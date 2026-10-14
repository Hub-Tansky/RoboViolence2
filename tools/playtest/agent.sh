#!/bin/bash
# Playtest pull agent: installs successful main builds on this Mac and tests them (ADR 0016).
# Scope: docs/roadmap/phase-t-testing-environment/step1-mac-pull-agent.md. Setup: tools/playtest/README.md.
# Usage: agent.sh                       local-smoke on the newest successful main build (the LaunchAgent runs this)
#        agent.sh --sha <sha>           local-smoke on that main build, also after 3 failed attempts
#        agent.sh --test staging-connect [--sha <sha>]   client from `current` (or <sha>) joins the staging server
#        agent.sh --play local|staging  start for manual play; agent.sh --stop ends it
#        agent.sh --preflight           check env file, tools and GitHub access only
set -u

# readonly: a line in the env file can't change them (the main-only filter depends on REPO)
readonly REPO=Hub-Tansky/RoboViolence2
readonly ARTIFACT=roboviolence2-macos-arm64.zip PKG_NAME=roboviolence2-macos-arm64
readonly ROOT=${RV2_PLAYTEST_ROOT:-$HOME/RV2-playtest}
readonly ENV_FILE=${RV2_PLAYTEST_ENV:-$HOME/.config/rv2-playtest.env}
readonly GAME_PORT=3333 MASTER_PORT=10207 ADMIN_PORT=18080
readonly MAX_ATTEMPTS=3 KEEP=5 KEEP_REPORTS=30
# Failures in these stages say nothing about the build: they notify once and don't use up an attempt
readonly INFRA_STAGES=" preflight select artifact download ports-busy gui-session "

# --- Helpers (tests/test_playtest_agent.sh) ---

# pick_run [sha prefix]: workflow-runs JSON on stdin -> "<run id> <sha>" of the newest successful push to main of REPO.
# head_repository excludes fork pull requests whose branch is also named main.
pick_run() {
	jq -r --arg repo "$REPO" --arg sha "${1:-}" '[.workflow_runs[]
		| select(.event == "push" and .head_branch == "main" and .head_repository.full_name == $repo
			and .conclusion == "success" and (.head_sha | startswith($sha)))] | .[0] | select(.) | "\(.id) \(.head_sha)"'
}

# artifact_info: artifacts JSON on stdin -> "<artifact id> <sha256 hex>" of ARTIFACT
artifact_info() {
	jq -r --arg n "$ARTIFACT" '[.artifacts[] | select(.name == $n and (.expired | not))] | .[0] | select(.)
		| "\(.id) \(.digest // "" | sub("^sha256:"; ""))"'
}

# digest_ok <file> <hex>: integrity of the transfer; the trust anchor is pick_run's main-only filter
digest_ok() { [ -n "$2" ] && [ "$(shasum -a 256 "$1" | cut -d' ' -f1)" = "$2" ]; }

file_mode() { stat -c %a "$1" 2>/dev/null || stat -f %Lp "$1"; } # octal permission bits, GNU or BSD stat

last_state() { # last_state <sha> <test>: newest state.tsv line (sha run test target result stage attempts time)
	[ -f "$ROOT/state.tsv" ] && awk -F'\t' -v s="$1" -v t="$2" '$1 == s && $3 == t' "$ROOT/state.tsv" | tail -1
}

attempts_of() { local a; a=$(last_state "$1" "$2" | cut -f7); echo "${a:-0}"; }

due() { # due <sha> <test>: true unless that SHA passed or failed MAX_ATTEMPTS times
	[ "$(last_state "$1" "$2" | cut -f5)" != pass ] && [ "$(attempts_of "$1" "$2")" -lt "$MAX_ATTEMPTS" ]
}

record_state() { # record_state <sha> <run> <test> <target> <result> <stage>: appends a state.tsv line, prints the attempt
	local n stage=$6
	n=$(($(attempts_of "$1" "$3") + 1))
	[ "$5" = pass ] && stage=-
	printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' "$1" "$2" "$3" "$4" "$5" "$stage" "$n" "$(date -u +%Y-%m-%dT%H:%M:%SZ)" >>"$ROOT/state.tsv"
	echo "$n"
}

is_infra() { case $INFRA_STAGES in *" $1 "*) return 0 ;; esac; return 1; } # is_infra <stage>

prune() { # prune <dir> <keep>: delete the oldest entries, never the one `current` points to
	local keep_target d
	keep_target=$(readlink "$ROOT/current" 2>/dev/null)
	ls -1t "$1" 2>/dev/null | tail -n +$(($2 + 1)) | while read -r d; do
		[ "$1/$d" = "$keep_target" ] || rm -rf "${1:?}/$d"
	done
}

load_env() { # sourced, not exported: each secret is passed only to the process that needs it
	[ -f "$ENV_FILE" ] || { echo "missing $ENV_FILE"; return 1; }
	[ "$(file_mode "$ENV_FILE")" = 600 ] || { echo "$ENV_FILE must be mode 600"; return 1; }
	# shellcheck source=/dev/null
	. "$ENV_FILE"
	[ -n "${BV2_ADMIN_PASS:-}" ] || { echo "BV2_ADMIN_PASS is not set in $ENV_FILE"; return 1; }
}

[ -n "${RV2_AGENT_SOURCE_ONLY:-}" ] && return 0

# --- Run state ---

MODE=smoke TEST=local-smoke TARGET=local WANT_SHA=""
SHA="" RUN_ID="" NAME="" STAGE="" RESULT="" REPORT="" LOCKED="" KEEP_RUNNING=""
PKG="" RUN_DIR="" CPREF="" CLIENT_PID="" SERVER_PID="" PIDS="" FIFO=""

say() { printf '%s %s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "$*" | tee -a "${REPORT:-/dev/null}"; }
stage() { STAGE=$1; say "stage $1"; }
fail() { say "FAIL $STAGE: $*"; RESULT=fail; exit 1; }

notify() { # notify <title> <message>: text goes in as arguments, never as AppleScript source or argv URL
	osascript - "$1" "$2" >/dev/null 2>&1 <<-'EOF'
		on run argv
			display notification (item 2 of argv) with title (item 1 of argv)
		end run
	EOF
	if [ -n "${NTFY_TOPIC:-}" ]; then # the topic is a secret: it goes to curl as config on stdin
		printf 'url = "https://ntfy.sh/%s"\n' "$NTFY_TOPIC" | curl -fsS --max-time 10 -K - -H "Title: $1" -d "$2" >/dev/null 2>&1
	fi
	return 0
}

gh_api() { GH_TOKEN=$(gh auth token --user Hub-Tansky 2>/dev/null) gh api "$@"; }

wait_for() { # wait_for <seconds> <command...>: true once the command succeeds
	local end=$(($(date +%s) + $1)); shift
	until "$@" >/dev/null 2>&1; do
		[ "$(date +%s)" -ge "$end" ] && return 1
		sleep 1
	done
}

has_player() { # has_player <base url> <admin pass> <name>: /status lists that player
	printf 'Authorization: Bearer %s\n' "$2" | curl -fsS --max-time 5 -H @- "$1/status" | grep -q "^[0-9]* $3 team "
}

port_busy() { nc -z 127.0.0.1 "$1" >/dev/null 2>&1; }

alive() { [ -n "$1" ] && kill -0 "$1" 2>/dev/null; }

ours() { ps -p "$1" -o comm= 2>/dev/null | grep -qE 'bv2|sleep'; } # pid is one of our processes, not a reused one

lock() {
	mkdir -p "$ROOT" && chmod 700 "$ROOT"
	local p; p=$(cat "$ROOT/lock" 2>/dev/null)
	# macOS shlock never takes over a dead pid's lock: drop it when its pid is not a running agent
	ps -p "$p" -o command= 2>/dev/null | grep -qE 'bash ([^ ]*/)?agent\.sh( |$)' || rm -f "$ROOT/lock" # also an empty or missing file
	shlock -p $$ -f "$ROOT/lock" || { say "locked by pid $(cat "$ROOT/lock" 2>/dev/null); skipped"; exit 0; }
	LOCKED=1
}

play_pids() { local p; for p in $(cat "$ROOT/play.pids" 2>/dev/null); do ours "$p" && echo "$p"; done; }

play_running() { # true while a --play session has live processes; forgets a stale pid file
	[ -n "$(play_pids)" ] && return 0
	rm -f "$ROOT/play.pids"
	return 1
}

preflight() { # tools and GitHub access; prints problems, true when there are none
	local bad=0 t
	for t in gh jq curl ditto shasum nc osascript shlock mkfifo xattr; do
		command -v "$t" >/dev/null || { echo "missing tool: $t"; bad=1; }
	done
	gh auth token --user Hub-Tansky >/dev/null 2>&1 || { echo "gh has no token for Hub-Tansky (keychain locked?)"; bad=1; }
	gh_api "repos/$REPO/actions/workflows/build.yml/runs?per_page=1" >/dev/null 2>&1 || { echo "GitHub API not reachable"; bad=1; }
	return $bad
}

# --- Steps ---

select_run() {
	stage select
	local runs pick
	runs=$(gh_api "repos/$REPO/actions/workflows/build.yml/runs?branch=main&event=push&status=success&per_page=50") || fail "runs query"
	pick=$(printf '%s' "$runs" | pick_run "$WANT_SHA")
	[ -n "$pick" ] || fail "no successful main push build${WANT_SHA:+ for $WANT_SHA}"
	RUN_ID=${pick% *} SHA=${pick#* }
}

fetch_install() { # sets PKG; reuses an installed copy of the same build
	local vdir="$ROOT/versions/${SHA:0:7}" json info id hex dl installed=""
	PKG="$vdir/$PKG_NAME"
	read -r installed _ 2>/dev/null <"$vdir/BUILD"
	if [ "$installed" = "$SHA" ]; then # any successful main run of a commit has the same package
		say "already installed: $vdir"
		return
	fi
	stage artifact
	json=$(gh_api "repos/$REPO/actions/runs/$RUN_ID/artifacts?per_page=100") || fail "artifacts query"
	info=$(printf '%s' "$json" | artifact_info)
	[ -n "$info" ] || fail "run $RUN_ID has no $ARTIFACT"
	id=${info% *} hex=${info#* }
	stage download
	dl=$(mktemp -d "$ROOT/download.XXXXXX") || fail "temp dir"
	if ! gh_api "repos/$REPO/actions/artifacts/$id/zip" >"$dl/pkg.zip"; then rm -rf "$dl"; fail "artifact $id"; fi
	stage verify
	if ! digest_ok "$dl/pkg.zip" "$hex"; then rm -rf "$dl"; fail "sha256 differs from the artifact digest ${hex:-(none)}"; fi
	stage install # unzip beside, then swap: a failed unzip never leaves a half-installed version
	if ! ditto -x -k "$dl/pkg.zip" "$dl/new"; then rm -rf "$dl"; fail "unzip"; fi
	xattr -dr com.apple.quarantine "$dl/new" 2>/dev/null
	if ! [ -x "$dl/new/$PKG_NAME/bv2dedicated" ] || ! [ -x "$dl/new/$PKG_NAME/bv2master" ] || ! [ -x "$dl/new/$PKG_NAME/run-client.sh" ]; then
		rm -rf "$dl"; fail "unexpected package layout"
	fi
	printf '%s %s\n' "$SHA" "$RUN_ID" >"$dl/new/BUILD" # written last: BUILD means a complete package
	mkdir -p "$ROOT/versions" && rm -rf "$vdir" && mv "$dl/new" "$vdir"
	rm -rf "$dl"
}

use_current() { # sets PKG, SHA, RUN_ID from the last version that passed local-smoke
	local vdir; vdir=$(readlink "$ROOT/current" 2>/dev/null)
	[ -n "$vdir" ] && [ -f "$vdir/BUILD" ] || fail "no passing version yet (run local-smoke first)"
	read -r SHA RUN_ID <"$vdir/BUILD"
	PKG="$vdir/$PKG_NAME"
}

start_local() { # start_local <run dir>: master and server from PKG, on fixed ports
	local p
	stage ports-busy
	for p in $GAME_PORT $MASTER_PORT $ADMIN_PORT; do port_busy "$p" && fail "port $p is in use"; done
	stage local-start
	RUN_DIR=$1
	mkdir -p "$RUN_DIR/pref" && cp "$PKG/master.db" "$PKG/web.db" "$RUN_DIR/" || fail "run dir"
	(cd "$RUN_DIR" && exec "$PKG/bv2master" </dev/null >"$RUN_DIR/master.log" 2>&1) &
	PIDS="$PIDS $!"
	FIFO="$RUN_DIR/server.in"
	mkfifo "$FIFO" || fail "fifo"
	sleep 2147483647 >"$FIFO" 2>/dev/null & # holds the server's stdin open; `quit` is written beside it
	PIDS="$PIDS $!"
	(cd "$PKG" && BV2_PREF_DIR="$RUN_DIR/pref" BV2_MASTER_SERVERS="127.0.0.1:$MASTER_PORT" BV2_ADMIN_API_PORT=$ADMIN_PORT \
		BV2_ADMIN_PASS="$BV2_ADMIN_PASS" exec ./bv2dedicated CTF <"$FIFO" >"$RUN_DIR/server.log" 2>&1) &
	SERVER_PID=$!
	PIDS="$PIDS $!"
	stage server-port # the port, not the log: the server's stdout is block-buffered in a file
	wait_for 30 port_busy $GAME_PORT || fail "server not listening on $GAME_PORT within 30 s"
	stage master-port
	wait_for 10 port_busy $MASTER_PORT || fail "master not listening on $MASTER_PORT within 10 s"
}

start_client() { # start_client <pref dir> <host> [player name]: packaged client; a name also sets windowed mode
	local cfg=""
	stage gui-session
	[ "$(stat -f %Su /dev/console)" = "$(id -un)" ] || fail "user not logged in at the console"
	stage client-start
	CPREF=$1
	mkdir -p "$CPREF"
	if [ -n "${3:-}" ]; then
		cfg="$CPREF/playtest.cfg" # absolute: run-client.sh changes to the package dir
		printf 'r_fullScreen false\ncl_playerName "%s"\n' "$3" >"$cfg"
	fi
	(cd "$PKG" && BV2_PREF_DIR="$CPREF" exec ./run-client.sh ${cfg:+--config "$cfg"} connect "$2" </dev/null >"$CPREF/client.log" 2>&1) &
	CLIENT_PID=$!
	sleep 10
	alive "$CLIENT_PID" || fail "client exited within 10 s"
}

any_alive() { local p; for p in "$@"; do alive "$p" && return 0; done; return 1; }

wait_gone() { # wait_gone <seconds> <pid...>: true once none is running
	local end=$(($(date +%s) + $1)); shift
	while any_alive "$@"; do
		[ "$(date +%s)" -ge "$end" ] && return 1
		sleep 1
	done
}

stop_all() { # client, then `quit` to the server, then TERM, then KILL
	local p
	alive "$CLIENT_PID" && kill "$CLIENT_PID" 2>/dev/null
	[ -p "$FIFO" ] && echo quit 1<>"$FIFO" # read-write open never blocks, even without a reader
	wait_gone 10 $SERVER_PID $CLIENT_PID
	for p in $PIDS; do alive "$p" && kill "$p" 2>/dev/null; done
	wait_gone 5 $PIDS || for p in $PIDS $CLIENT_PID; do alive "$p" && kill -9 "$p" 2>/dev/null; done
	wait $PIDS $CLIENT_PID 2>/dev/null # reap, so bash prints no job notices
	CLIENT_PID="" SERVER_PID="" PIDS=""
}

finish() {
	trap - EXIT
	trap '' TERM INT # a second signal must not cut the cleanup short
	[ -n "$KEEP_RUNNING" ] || stop_all
	[ -n "$REPORT" ] && [ -z "$RESULT" ] && RESULT=fail # an unexpected exit while testing a build
	local infra="" # this run failed before testing the build
	[ "$RESULT" = fail ] && is_infra "$STAGE" && infra=1
	if [ "$RESULT" = interrupted ]; then
		say "interrupted (logout, shutdown or bootout); nothing recorded"
	elif [ -n "$REPORT" ] && [ -z "$infra" ]; then
		local attempt f
		attempt=$(record_state "$SHA" "$RUN_ID" "$TEST" "$TARGET" "$RESULT" "$STAGE")
		for f in "$RUN_DIR/server.log" "$RUN_DIR/master.log" "$CPREF/client.log" "$CPREF/console.log"; do
			[ -n "$f" ] && [ -f "$f" ] && { printf -- '--- last 50 lines of %s\n' "$f"; tail -50 "$f"; } >>"$REPORT"
		done
		say "result $RESULT (attempt $attempt)"
		if [ "$RESULT" = pass ]; then
			[ "$TEST" = local-smoke ] && ln -sfn "$(dirname "$PKG")" "$ROOT/current"
			notify "RV2 ${SHA:0:7} READY TO TEST" "$TEST passed. Play: agent.sh --play local. Report: $REPORT"
		else
			notify "RV2 ${SHA:0:7} FAILED at $STAGE" "$TEST, attempt $attempt. Report: $REPORT"
		fi
		prune "$ROOT/versions" $KEEP; prune "$ROOT/pref" $KEEP; prune "$ROOT/run" $KEEP; prune "$ROOT/reports" $KEEP_REPORTS
	elif [ -n "$infra" ] && [ "$MODE" = smoke ] && [ "$(cat "$ROOT/infra-failure" 2>/dev/null)" != "$STAGE" ]; then
		echo "$STAGE" >"$ROOT/infra-failure" # once per distinct failure, not every 10 minutes
		notify "RV2 agent FAILED at $STAGE" "No attempt counted. Log: $ROOT/agent.log"
	fi
	# The unattended marker: cleared by any scheduled run that got past the infrastructure, so the next incident notifies again
	[ "$MODE" = smoke ] && [ -z "$infra" ] && rm -f "$ROOT/infra-failure"
	[ -n "$LOCKED" ] && rm -f "$ROOT/lock"
}

begin_report() {
	mkdir -p "$ROOT/reports"
	REPORT="$ROOT/reports/${SHA:0:7}.txt"
	NAME="rv2-agent-${SHA:0:7}"
	say "=== $TEST on $TARGET: $SHA, run https://github.com/$REPO/actions/runs/$RUN_ID"
}

usage() { sed -n '4,8p' "$0" >&2; exit 2; }

# --- Modes ---

while [ $# -gt 0 ]; do
	case "$1" in
		--sha) [ -n "${2:-}" ] || usage; WANT_SHA=$2; shift 2 ;;
		--test) [ "${2:-}" = staging-connect ] || usage; MODE=test TEST=$2 TARGET=staging; shift 2 ;;
		--play) case "${2:-}" in local | staging) ;; *) usage ;; esac; MODE=play TARGET=$2; shift 2 ;;
		--stop) MODE=stop; shift ;;
		--preflight) MODE=preflight; shift ;;
		*) usage ;;
	esac
done
[ "$MODE" = play ] && [ -n "$WANT_SHA" ] && usage # play always uses `current`

case $MODE in
	preflight)
		if out=$({ load_env && preflight; } 2>&1); then notify "RV2 agent preflight passed" "Ready for unattended runs"; echo ok; exit 0; fi
		echo "$out"; notify "RV2 agent preflight FAILED" "$(echo "$out" | head -1)"; exit 1 ;;
	stop)
		for p in $(play_pids); do kill "$p" 2>/dev/null; done
		rm -f "$ROOT/play.pids"; exit 0 ;;
esac

lock
trap finish EXIT
trap 'RESULT=interrupted; exit 143' TERM INT
stage preflight
load_env || fail "env file"
out=$(preflight 2>&1) || fail "$out"

case $MODE in
	smoke)
		play_running && { say "manual play running; skipped"; exit 0; }
		select_run
		due "$SHA" "$TEST" || [ -n "$WANT_SHA" ] || exit 0
		begin_report
		fetch_install
		start_local "$ROOT/run/${SHA:0:7}-local-$(date +%Y%m%d%H%M%S)"
		start_client "$ROOT/pref/${SHA:0:7}" 127.0.0.1 "$NAME"
		stage client-join
		wait_for 30 has_player "http://127.0.0.1:$ADMIN_PORT" "$BV2_ADMIN_PASS" "$NAME" || fail "/status does not list $NAME within 30 s"
		RESULT=pass ;;
	test)
		if [ -z "${RV2_STAGING_HOST:-}" ] || [ -z "${RV2_STAGING_ADMIN_PASS:-}" ]; then
			notify "RV2 staging-connect not-configured" "Set RV2_STAGING_HOST and RV2_STAGING_ADMIN_PASS in $ENV_FILE"; exit 0
		fi
		if [ -n "$WANT_SHA" ]; then select_run; begin_report; fetch_install; else use_current; begin_report; fi
		stage staging-version
		deployed=$(curl -fsS --max-time 5 "https://$RV2_STAGING_HOST:8443/deployed" 2>/dev/null | cut -d' ' -f1)
		say "staging runs ${deployed:-an unknown build (no /deployed)}"
		start_client "$ROOT/pref/${SHA:0:7}-staging" "$RV2_STAGING_HOST" "$NAME"
		stage staging-join
		if ! wait_for 30 has_player "https://$RV2_STAGING_HOST:8443" "$RV2_STAGING_ADMIN_PASS" "$NAME"; then
			[ -n "$deployed" ] && [ "$deployed" != "$SHA" ] && STAGE=staging-version-mismatch
			fail "staging /status does not list $NAME within 30 s"
		fi
		RESULT=pass ;;
	play) # manual: no report, no pass/fail
		play_running && { echo "already running; agent.sh --stop first" >&2; exit 1; }
		use_current
		if [ "$TARGET" = local ]; then
			start_local "$ROOT/run/play-$(date +%Y%m%d%H%M%S)"
			start_client "$ROOT/play-pref" 127.0.0.1
		else
			[ -n "${RV2_STAGING_HOST:-}" ] || fail "RV2_STAGING_HOST is not set"
			start_client "$ROOT/play-pref" "$RV2_STAGING_HOST"
		fi
		echo "$PIDS $CLIENT_PID" >"$ROOT/play.pids"
		KEEP_RUNNING=1
		echo "playing $TARGET; stop with: agent.sh --stop" ;;
esac
