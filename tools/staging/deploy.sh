#!/bin/bash
# Staging deploy agent: runs the newest main server and master images on this host, never during a game (ADR 0018).
# Scope: docs/roadmap/phase-t-testing-environment/step2-staging-deploy.md. Setup: tools/staging/README.md.
# Usage: deploy.sh               the newest main build whose `containers` run passed, once no one is playing (cron runs this)
#        deploy.sh --sha <sha>   that main build (also a manual rollback), once no one is playing
#        deploy.sh --force       don't wait for an empty server
set -u

# readonly: a line in the env file can't change them (the main-only filter depends on REPO)
readonly REPO=Hub-Tansky/RoboViolence2
readonly IMAGES="ghcr.io/hub-tansky/roboviolence2-server ghcr.io/hub-tansky/roboviolence2-master"
readonly PROJECT=roboviolence2 # `name:` in compose.example.yml
readonly ROOT=${RV2_STAGING_ROOT:-/var/lib/rv2-staging}
readonly ENV_FILE=${RV2_STAGING_ENV:-/etc/rv2-staging.env}
readonly MAX_ATTEMPTS=3 WAIT_NOTIFY_SECONDS=21600 KEEP=5
# Failures in these stages say nothing about the build: they notify once and don't use up an attempt.
# wait-empty fails only when the server runs but /status doesn't answer: never deploy over a game we can't see.
readonly INFRA_STAGES=" env select fetch pull wait-empty "

# --- Helpers (tests/test_staging_deploy.sh) ---

# pick_run [sha prefix]: workflow-runs JSON on stdin -> "<run id> <sha>" of the newest successful push to main of REPO.
# The same filter as tools/playtest/agent.sh: head_repository excludes fork pull requests from a branch named main.
pick_run() {
	jq -r --arg repo "$REPO" --arg sha "${1:-}" '[.workflow_runs[]
		| select(.event == "push" and .head_branch == "main" and .head_repository.full_name == $repo
			and .conclusion == "success" and (.head_sha | startswith($sha)))] | .[0] | select(.) | "\(.id) \(.head_sha)"'
}

players_in() { sed -n 's/^players \([0-9]*\)$/\1/p'; } # /status text on stdin -> player count

failures_of() { # failures_of <sha>: failed deploys recorded for it
	[ -f "$ROOT/state.tsv" ] && awk -F'\t' -v s="$1" '$1 == s && $2 == "fail"' "$ROOT/state.tsv" | wc -l | tr -d ' ' || echo 0
}

is_infra() { case $INFRA_STAGES in *" $1 "*) return 0 ;; esac; return 1; } # is_infra <stage>

waiting_update() { # waiting_update <now>: records that players hold back a deploy; true once, after WAIT_NOTIFY_SECONDS
	local since="" told=""
	read -r since told 2>/dev/null <"$ROOT/waiting" # "<since epoch> <notified 0|1>"; kept across new commits
	[ -n "$since" ] || { since=$1 told=0; }
	if [ $(($1 - since)) -ge $WAIT_NOTIFY_SECONDS ] && [ "$told" = 0 ]; then
		echo "$since 1" >"$ROOT/waiting"
		return 0
	fi
	echo "$since $told" >"$ROOT/waiting"
	return 1
}

prune_releases() { # prune_releases <keep>: delete the oldest release dirs beyond KEEP; prints the full SHA of each
	local d sha
	ls -1t "$ROOT/releases" 2>/dev/null | tail -n +$((KEEP + 1)) | while read -r d; do
		[ "$d" = "$1" ] && continue
		read -r sha 2>/dev/null <"$ROOT/releases/$d/SHA" && echo "$sha"
		rm -rf "${ROOT:?}/releases/$d"
	done
}

file_mode() { stat -c %a "$1" 2>/dev/null || stat -f %Lp "$1"; } # octal permission bits, GNU or BSD stat

load_env() { # sourced, not exported; compose reads the same file with --env-file
	[ -f "$ENV_FILE" ] || { echo "missing $ENV_FILE"; return 1; }
	[ "$(file_mode "$ENV_FILE")" = 600 ] || { echo "$ENV_FILE must be mode 600"; return 1; }
	# shellcheck source=/dev/null
	. "$ENV_FILE"
	[ -n "${BV2_ADMIN_PASS:-}" ] || { echo "BV2_ADMIN_PASS is not set in $ENV_FILE"; return 1; }
}

[ -n "${RV2_DEPLOY_SOURCE_ONLY:-}" ] && return 0

# --- Run state ---

WANT_SHA="" FORCE="" SHA="" PREV="" STAGE="" RESULT="" LOCKED=""

say() { printf '%s %s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "$*"; }
stage() { STAGE=$1; say "stage $1"; }
fail() { say "FAIL $STAGE: $*"; RESULT=fail; exit 1; }

notify() { # notify <title> <message>: ntfy only (a server has no desktop); the topic goes to curl on stdin
	say "notify: $1 - $2"
	if [ -n "${NTFY_TOPIC:-}" ]; then
		printf 'url = "https://ntfy.sh/%s"\n' "$NTFY_TOPIC" | curl -fsS --max-time 10 -K - -H "Title: $1" -d "$2" >/dev/null 2>&1
	fi
	return 0
}

compose() { # compose <sha> <args...>: that commit's compose file, images pinned to it
	local sha=$1; shift
	BV2_IMAGE_TAG=$sha docker compose -f "$ROOT/releases/${sha:0:7}/compose.example.yml" --env-file "$ENV_FILE" "$@"
}

server_running() { [ -n "$(docker ps -q -f "label=com.docker.compose.project=$PROJECT" -f label=com.docker.compose.service=server -f status=running)" ]; }

admin_status() { # the running server's /status through its own proxy; -k: loopback to our own Caddy
	local host=${BV2_ADMIN_HOST:-localhost} port=${BV2_ADMIN_PORT:-8443}
	printf 'Authorization: Bearer %s\n' "$BV2_ADMIN_PASS" |
		curl -fsS -k --max-time 5 -H @- --resolve "$host:$port:127.0.0.1" "https://$host:$port/status"
}

wait_for() { # wait_for <seconds> <command...>: true once the command succeeds
	local end=$(($(date +%s) + $1)); shift
	until "$@" >/dev/null 2>&1; do
		[ "$(date +%s)" -ge "$end" ] && return 1
		sleep 2
	done
}

lock() { # one run at a time; a lock whose pid is not a running deploy.sh is stale
	{ mkdir -p "$ROOT" && chmod 700 "$ROOT" && [ -w "$ROOT" ]; } || { say "FAIL: $ROOT is not writable by $(id -un)"; exit 1; }
	local p; p=$(cat "$ROOT/lock" 2>/dev/null)
	ps -p "$p" -o command= 2>/dev/null | grep -qE 'bash ([^ ]*/)?deploy\.sh( |$)' || rm -f "$ROOT/lock"
	(set -C; echo $$ >"$ROOT/lock") 2>/dev/null || { say "locked by pid $(cat "$ROOT/lock" 2>/dev/null); skipped"; exit 0; }
	LOCKED=1
}

finish() {
	trap - EXIT
	local infra=""
	[ "$RESULT" = fail ] && is_infra "$STAGE" && infra=1
	if [ -n "$infra" ]; then
		if [ "$(cat "$ROOT/infra-failure" 2>/dev/null)" != "$STAGE" ]; then
			echo "$STAGE" >"$ROOT/infra-failure" # once per distinct failure, not every 10 minutes
			notify "RV2 staging deploy FAILED at $STAGE" "No attempt counted${PREV:+; staging still on ${PREV:0:7}}"
		fi
	else
		rm -f "$ROOT/infra-failure"
	fi
	if [ "$RESULT" = fail ] && [ -z "$infra" ] && [ -n "$SHA" ]; then
		printf '%s\tfail\t%s\t%s\n' "$SHA" "$STAGE" "$(date -u +%Y-%m-%dT%H:%M:%SZ)" >>"$ROOT/state.tsv"
		if [ -n "$PREV" ] && [ "$PREV" != "$SHA" ] && compose "$PREV" up -d --no-build --wait --wait-timeout 120 >/dev/null 2>&1 &&
			wait_for 60 admin_status; then
			notify "RV2 staging deploy ${SHA:0:7} FAILED at $STAGE" "Rolled back to ${PREV:0:7}"
		else
			notify "RV2 staging deploy ${SHA:0:7} FAILED at $STAGE" "No rollback${PREV:+: ${PREV:0:7} did not come back}; staging may be down"
		fi
	fi
	[ -n "$LOCKED" ] && rm -f "$ROOT/lock"
}

usage() { sed -n '4,6p' "$0" >&2; exit 2; }

while [ $# -gt 0 ]; do
	case "$1" in
		--sha) [ -n "${2:-}" ] || usage; WANT_SHA=$2; shift 2 ;;
		--force) FORCE=1; shift ;;
		*) usage ;;
	esac
done

lock
trap finish EXIT
read -r PREV _ 2>/dev/null <"$ROOT/deployed"

stage env # without the env file there is no ntfy topic: this failure is in deploy.log only
load_env || fail "env file"
for t in curl jq docker; do command -v "$t" >/dev/null || fail "missing tool: $t"; done

stage select
query="branch=main&event=push&status=success&per_page=30"
[ ${#WANT_SHA} = 40 ] && query="$query&head_sha=$WANT_SHA" # a full SHA is found however old it is
runs=$(curl -fsS --max-time 20 "https://api.github.com/repos/$REPO/actions/workflows/containers.yml/runs?$query") || fail "runs query"
pick=$(printf '%s' "$runs" | pick_run "$WANT_SHA")
[ -n "$pick" ] || fail "no successful main push run of containers${WANT_SHA:+ for $WANT_SHA}"
SHA=${pick#* }
# Already running. The weekly rebuild re-tags the same commit; it reaches staging with the next commit (ADR 0018).
[ "$SHA" = "$PREV" ] && { rm -f "$ROOT/infra-failure" "$ROOT/waiting"; exit 0; }
if [ -z "$WANT_SHA" ] && [ "$(failures_of "$SHA")" -ge $MAX_ATTEMPTS ]; then exit 0; fi

stage fetch # this commit's compose file and Caddyfile, so files and images match
rel="$ROOT/releases/${SHA:0:7}"
mkdir -p "$rel/srv" || fail "release dir"
for f in compose.example.yml Caddyfile; do
	curl -fsS --max-time 20 "https://raw.githubusercontent.com/$REPO/$SHA/packaging/docker/$f" -o "$rel/$f" || fail "$f"
done
grep -q 'BV2_IMAGE_TAG' "$rel/compose.example.yml" || fail "commit predates BV2_IMAGE_TAG: it would run :latest"
echo "$SHA" >"$rel/SHA"

stage pull
for image in $IMAGES; do docker pull -q "$image:$SHA" >/dev/null || fail "$image:$SHA"; done

stage wait-empty # decided by the container, not the API: an unreadable /status must not look like an empty server
if [ -z "$FORCE" ] && server_running; then
	status=$(admin_status) || fail "server runs but /status doesn't answer (password changed? proxy down?); --force to deploy"
	players=$(printf '%s\n' "$status" | players_in)
	if [ "${players:-0}" -gt 0 ]; then
		say "$players players connected; ${SHA:0:7} waits"
		waiting_update "$(date +%s)" &&
			notify "RV2 staging deploy waiting" "Players held deploys back for 6 h; newest ${SHA:0:7}; deploy.sh --force to override"
		exit 0
	fi
fi
rm -f "$ROOT/waiting"

stage deploy
compose "$SHA" up -d --no-build --wait --wait-timeout 120 || fail "compose up"

stage verify
wait_for 60 admin_status || fail "/status did not answer within 60 s"
printf '%s %s\n' "$SHA" "$(date -u +%Y-%m-%dT%H:%M:%SZ)" | tee "$ROOT/deployed" >"$rel/srv/deployed" # served at /deployed
printf '%s\tpass\t-\t%s\n' "$SHA" "$(date -u +%Y-%m-%dT%H:%M:%SZ)" >>"$ROOT/state.tsv"
RESULT=pass
notify "RV2 staging now on ${SHA:0:7}" "Deployed${PREV:+ (was ${PREV:0:7})}"
for old in $(prune_releases "${SHA:0:7}"); do # the images of dropped releases; the rollback target stays
	for image in $IMAGES; do docker rmi "$image:$old" >/dev/null 2>&1; done
done
