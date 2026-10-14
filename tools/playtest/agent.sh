#!/bin/bash
# Playtest pull agent: installs successful main builds on this Mac and tests them (ADR 0016).
# Scope: docs/roadmap/phase-t-testing-environment/step1-mac-pull-agent.md. Setup: tools/playtest/README.md.
# Usage: agent.sh                       local-smoke on the newest successful main build (the LaunchAgent runs this)
#        agent.sh --sha <sha>           local-smoke on that main build, also after 3 failed attempts
#        agent.sh --test staging-connect [--sha <sha>]   client from `current` (or <sha>) joins the staging server
#        agent.sh --play local|staging  start for manual play; agent.sh --stop ends it
#        agent.sh --preflight           check env file, tools and GitHub access only
set -u

REPO=Hub-Tansky/RoboViolence2
ARTIFACT=roboviolence2-macos-arm64.zip
PKG_NAME=roboviolence2-macos-arm64
ROOT=${RV2_PLAYTEST_ROOT:-$HOME/RV2-playtest}
ENV_FILE=${RV2_PLAYTEST_ENV:-$HOME/.config/rv2-playtest.env}
GAME_PORT=3333 MASTER_PORT=10207 ADMIN_PORT=18080
MAX_ATTEMPTS=3 KEEP=5 KEEP_REPORTS=30

# --- Helpers without side effects (tests/test_playtest_agent.sh) ---

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

digest_ok() { [ -n "$2" ] && [ "$(shasum -a 256 "$1" | cut -d' ' -f1)" = "$2" ]; } # digest_ok <file> <hex>

file_mode() { stat -c %a "$1" 2>/dev/null || stat -f %Lp "$1"; } # octal permission bits, GNU or BSD stat

last_state() { # last_state <sha> <test>: newest state.tsv line (sha run test target result stage attempts time)
	[ -f "$ROOT/state.tsv" ] && awk -F'\t' -v s="$1" -v t="$2" '$1 == s && $3 == t' "$ROOT/state.tsv" | tail -1
}

attempts_of() { local a; a=$(last_state "$1" "$2" | cut -f7); echo "${a:-0}"; }

due() { # due <sha> <test>: true unless that SHA passed or failed MAX_ATTEMPTS times
	[ "$(last_state "$1" "$2" | cut -f5)" != pass ] && [ "$(attempts_of "$1" "$2")" -lt "$MAX_ATTEMPTS" ]
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

MODE=smoke TEST=local-smoke TARGET=local WANT_SHA="" PLAY=""
SHA="" RUN_ID="" STAGE="" RESULT="" REPORT="" LOCKED=""
PKG="" RUN_DIR="" CPREF="" CLIENT_PID="" SERVER_PID="" PIDS="" FIFO=""

say() { printf '%s %s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "$*" | tee -a "${REPORT:-/dev/null}"; }
stage() { STAGE=$1; say "stage $1"; }
fail() { say "FAIL $STAGE: $*"; RESULT=fail; exit 1; }

notify() { # notify <title> <message>
	osascript -e "display notification \"$2\" with title \"$1\"" >/dev/null 2>&1
	[ -n "${NTFY_TOPIC:-}" ] && curl -fsS --max-time 10 -H "Title: $1" -d "$2" "https://ntfy.sh/$NTFY_TOPIC" >/dev/null 2>&1
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

lock() {
	mkdir -p "$ROOT" && chmod 700 "$ROOT"
	if ! mkdir "$ROOT/lock" 2>/dev/null; then
		alive "$(cat "$ROOT/lock/pid" 2>/dev/null)" && exit 0
		rm -rf "$ROOT/lock"
		mkdir "$ROOT/lock" || exit 1
	fi
	echo $$ >"$ROOT/lock/pid"
	LOCKED=1
}

prune() { # prune <dir> <keep>: delete the oldest entries, never the one `current` points to
	local keep_target; keep_target=$(readlink "$ROOT/current" 2>/dev/null)
	ls -1t "$1" 2>/dev/null | tail -n +$(($2 + 1)) | while read -r d; do
		[ "$1/$d" = "$keep_target" ] || rm -rf "${1:?}/$d"
	done
}

preflight() { # tools and GitHub access; prints problems, true when there are none
	local bad=0 t
	for t in gh jq curl ditto shasum nc osascript; do command -v "$t" >/dev/null || { echo "missing tool: $t"; bad=1; }; done
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

fetch_install() { # sets PKG
	local info id hex dl vdir
	stage artifact
	info=$(gh_api "repos/$REPO/actions/runs/$RUN_ID/artifacts" | artifact_info)
	[ -n "$info" ] || fail "run $RUN_ID has no $ARTIFACT"
	id=${info% *} hex=${info#* }
	stage download
	dl=$(mktemp -d "$ROOT/download.XXXXXX") || fail "temp dir"
	gh_api "repos/$REPO/actions/artifacts/$id/zip" >"$dl/pkg.zip" || { rm -rf "$dl"; fail "artifact $id"; }
	stage verify
	digest_ok "$dl/pkg.zip" "$hex" || { rm -rf "$dl"; fail "sha256 differs from the artifact digest ${hex:-(none)}"; }
	stage install
	vdir="$ROOT/versions/$(date +%Y%m%d)-${SHA:0:7}"
	rm -rf "$vdir" && mkdir -p "$vdir" && ditto -x -k "$dl/pkg.zip" "$vdir" || { rm -rf "$dl"; fail "unzip"; }
	rm -rf "$dl"
	xattr -dr com.apple.quarantine "$vdir" 2>/dev/null
	PKG="$vdir/$PKG_NAME"
	[ -x "$PKG/bv2dedicated" ] && [ -x "$PKG/bv2master" ] && [ -x "$PKG/run-client.sh" ] || fail "unexpected package layout"
	printf '%s %s\n' "$SHA" "$RUN_ID" >"$vdir/BUILD"
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
	sleep 2147483647 </dev/null >"$FIFO" 2>/dev/null & # holds the server's stdin open; `quit` is written beside it
	PIDS="$PIDS $!"
	(cd "$PKG" && BV2_PREF_DIR="$RUN_DIR/pref" BV2_MASTER_SERVERS="127.0.0.1:$MASTER_PORT" BV2_ADMIN_API_PORT=$ADMIN_PORT \
		BV2_ADMIN_PASS="$BV2_ADMIN_PASS" exec ./bv2dedicated CTF <"$FIFO" >"$RUN_DIR/server.log" 2>&1) &
	SERVER_PID=$!
	PIDS="$PIDS $!"
	stage server-start
	wait_for 30 grep -q "Server Created on port $GAME_PORT" "$RUN_DIR/server.log" || fail "no 'Server Created' within 30 s"
	stage master-port
	wait_for 10 port_busy $MASTER_PORT || fail "master not listening on $MASTER_PORT within 10 s"
}

start_client() { # start_client <pref dir> <host> [cfg]: packaged client, its console log in the pref dir
	stage gui-session
	[ "$(stat -f %Su /dev/console)" = "$(id -un)" ] || { STAGE=no-gui-session; fail "user not logged in at the console"; }
	stage client-start
	CPREF=$1
	mkdir -p "$CPREF"
	(cd "$PKG" && BV2_PREF_DIR="$CPREF" exec ./run-client.sh ${3:+--config "$3"} connect "$2" </dev/null >"$CPREF/client.log" 2>&1) &
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
	alive "$SERVER_PID" && echo quit >"$FIFO" # a FIFO without a reader would block the write
	wait_gone 10 $SERVER_PID $CLIENT_PID
	for p in $PIDS; do alive "$p" && kill "$p" 2>/dev/null; done
	wait_gone 5 $PIDS || for p in $PIDS $CLIENT_PID; do alive "$p" && kill -9 "$p" 2>/dev/null; done
	wait $PIDS $CLIENT_PID 2>/dev/null # reap, so bash prints no job notices
	CLIENT_PID="" SERVER_PID="" PIDS=""
}

finish() {
	trap - EXIT
	[ -z "$PLAY" ] || [ "$RESULT" != pass ] && stop_all
	if [ -n "$SHA" ] && [ -n "$REPORT" ]; then
		[ "$RESULT" = pass ] || RESULT=fail
		local attempts=$(($(attempts_of "$SHA" "$TEST") + 1)) f
		printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' "$SHA" "$RUN_ID" "$TEST" "$TARGET" "$RESULT" \
			"$([ "$RESULT" = pass ] && echo - || echo "$STAGE")" "$attempts" "$(date -u +%Y-%m-%dT%H:%M:%SZ)" >>"$ROOT/state.tsv"
		for f in "$RUN_DIR/server.log" "$RUN_DIR/master.log" "$CPREF/client.log" "$CPREF/console.log"; do
			[ -n "$f" ] && [ -f "$f" ] && { printf -- '--- last 50 lines of %s\n' "$f"; tail -50 "$f"; } >>"$REPORT"
		done
		say "result $RESULT (attempt $attempts)"
		if [ "$RESULT" = pass ]; then
			[ "$TEST" = local-smoke ] && ln -sfn "$(dirname "$PKG")" "$ROOT/current"
			notify "RV2 ${SHA:0:7} READY TO TEST" "$TEST passed. Play: agent.sh --play local. Report: $REPORT"
		else
			notify "RV2 ${SHA:0:7} FAILED at $STAGE" "$TEST, attempt $attempts. Report: $REPORT"
		fi
		rm -f "$ROOT/infra-failure"
		prune "$ROOT/versions" $KEEP; prune "$ROOT/pref" $KEEP; prune "$ROOT/run" $KEEP; prune "$ROOT/reports" $KEEP_REPORTS
	elif [ "$RESULT" = fail ] && [ "$(cat "$ROOT/infra-failure" 2>/dev/null)" != "$STAGE" ]; then
		echo "$STAGE" >"$ROOT/infra-failure" # notify once per distinct failure, not every 10 minutes
		notify "RV2 agent FAILED at $STAGE" "No build tested. Log: $ROOT/agent.log"
	fi
	[ -n "$LOCKED" ] && rm -rf "$ROOT/lock"
}

begin_report() {
	mkdir -p "$ROOT/reports"
	REPORT="$ROOT/reports/${SHA:0:7}.txt"
	say "=== $TEST on $TARGET: $SHA, run https://github.com/$REPO/actions/runs/$RUN_ID"
}

play_running() { local p; for p in $(cat "$ROOT/play.pids" 2>/dev/null); do alive "$p" && return 0; done; return 1; }

# --- Modes ---

while [ $# -gt 0 ]; do
	case "$1" in
		--sha) WANT_SHA=$2; shift 2 ;;
		--test) MODE=test TEST=$2; shift 2 ;;
		--play) MODE=play TARGET=$2; shift 2 ;;
		--stop) MODE=stop; shift ;;
		--preflight) MODE=preflight; shift ;;
		*) sed -n '4,9p' "$0" >&2; exit 2 ;;
	esac
done

case $MODE in
	preflight)
		if out=$({ load_env && preflight; } 2>&1); then notify "RV2 agent preflight passed" "Ready for unattended runs"; echo ok; exit 0; fi
		echo "$out"; notify "RV2 agent preflight FAILED" "$(echo "$out" | head -1)"; exit 1 ;;
	stop)
		for p in $(cat "$ROOT/play.pids" 2>/dev/null); do alive "$p" && kill "$p" 2>/dev/null; done
		rm -f "$ROOT/play.pids"; exit 0 ;;
esac

lock
trap finish EXIT
stage preflight
out=$(load_env 2>&1) || fail "$out"
load_env
out=$(preflight 2>&1) || fail "$out"

case $MODE in
	smoke)
		play_running && { say "manual play running; skipped"; exit 0; }
		select_run
		if [ -z "$WANT_SHA" ] && ! due "$SHA" "$TEST"; then SHA=""; exit 0; fi
		begin_report
		fetch_install
		start_local "$ROOT/run/${SHA:0:7}-local-$(date +%Y%m%d%H%M%S)"
		CFG_DIR="$ROOT/pref/${SHA:0:7}"
		rm -rf "$CFG_DIR" && mkdir -p "$CFG_DIR"
		printf 'r_fullScreen false\ncl_playerName "rv2-agent-%s"\n' "${SHA:0:7}" >"$CFG_DIR/playtest.cfg"
		start_client "$CFG_DIR" 127.0.0.1 "$CFG_DIR/playtest.cfg"
		stage client-join
		wait_for 30 has_player "http://127.0.0.1:$ADMIN_PORT" "$BV2_ADMIN_PASS" "rv2-agent-${SHA:0:7}" || fail "/status does not list rv2-agent-${SHA:0:7} within 30 s"
		RESULT=pass ;;
	test)
		[ "$TEST" = staging-connect ] || { echo "unknown test: $TEST" >&2; exit 2; }
		TARGET=staging
		if [ -z "${RV2_STAGING_HOST:-}" ] || [ -z "${RV2_STAGING_ADMIN_PASS:-}" ]; then
			notify "RV2 staging-connect not-configured" "Set RV2_STAGING_HOST and RV2_STAGING_ADMIN_PASS in $ENV_FILE"; exit 0
		fi
		if [ -n "$WANT_SHA" ]; then select_run; begin_report; fetch_install; else use_current; begin_report; fi
		stage staging-version
		deployed=$(curl -fsS --max-time 5 "https://$RV2_STAGING_HOST:8443/deployed" 2>/dev/null | cut -d' ' -f1)
		say "staging runs ${deployed:-an unknown build (no /deployed)}"
		CFG_DIR="$ROOT/pref/${SHA:0:7}-staging"
		rm -rf "$CFG_DIR" && mkdir -p "$CFG_DIR"
		printf 'r_fullScreen false\ncl_playerName "rv2-agent-%s"\n' "${SHA:0:7}" >"$CFG_DIR/playtest.cfg"
		start_client "$CFG_DIR" "$RV2_STAGING_HOST" "$CFG_DIR/playtest.cfg"
		stage staging-join
		if ! wait_for 30 has_player "https://$RV2_STAGING_HOST:8443" "$RV2_STAGING_ADMIN_PASS" "rv2-agent-${SHA:0:7}"; then
			[ -n "$deployed" ] && [ "$deployed" != "$SHA" ] && STAGE=staging-version-mismatch
			fail "staging /status does not list rv2-agent-${SHA:0:7} within 30 s"
		fi
		RESULT=pass ;;
	play)
		PLAY=1
		play_running && { echo "already running; agent.sh --stop first" >&2; exit 1; }
		use_current
		SHA="" # manual play writes no pass/fail
		case $TARGET in
			local)
				start_local "$ROOT/run/play-$(date +%Y%m%d%H%M%S)"
				start_client "$ROOT/play-pref" 127.0.0.1 ;;
			staging)
				[ -n "${RV2_STAGING_HOST:-}" ] || fail "RV2_STAGING_HOST is not set"
				start_client "$ROOT/play-pref" "$RV2_STAGING_HOST" ;;
			*) echo "unknown target: $TARGET" >&2; exit 2 ;;
		esac
		echo "$PIDS $CLIENT_PID" >"$ROOT/play.pids"
		RESULT=pass
		echo "playing $TARGET; stop with: agent.sh --stop" ;;
esac
