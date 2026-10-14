#!/bin/bash
# Unit test for tools/staging/deploy.sh helpers: run selection, player count, attempt rule, infra stages, env-file mode.
# Usage: test_staging_deploy.sh <scratch dir>
set -u
here=$(cd "$(dirname "$0")" && pwd)
scratch=${1:?scratch dir}
rm -rf "$scratch" && mkdir -p "$scratch"
export RV2_DEPLOY_SOURCE_ONLY=1 RV2_STAGING_ROOT="$scratch/root" RV2_STAGING_ENV="$scratch/env"
# shellcheck source=../tools/staging/deploy.sh
. "$here/../tools/staging/deploy.sh"

failures=0
expect() { # expect <name> <expected> <actual>
	if [ "$2" = "$3" ]; then echo "ok   $1"; else echo "FAIL $1: expected '$2', got '$3'"; failures=$((failures + 1)); fi
}

run() { # run <id> <sha> <event> <branch> <head repo> <conclusion>: one workflow run as JSON
	printf '{"id":%s,"head_sha":"%s","event":"%s","head_branch":"%s","head_repository":{"full_name":"%s"},"conclusion":"%s"}' "$@"
}
runs="{\"workflow_runs\":[
$(run 6 ffff06 pull_request main someone/RoboViolence2 success),
$(run 5 eeee05 push feature Hub-Tansky/RoboViolence2 success),
$(run 4 dddd04 push main Hub-Tansky/RoboViolence2 failure),
$(run 3 cccc03 push main Hub-Tansky/RoboViolence2 success)]}"
expect "newest successful main push; fork PR, other branch and failed run skipped" "3 cccc03" "$(printf '%s' "$runs" | pick_run)"
expect "sha of a fork PR is never selected" "" "$(printf '%s' "$runs" | pick_run ffff)"

status='map CTF-Placeholder
game type 2
score blue 0 red 0
players 2
0 Alice team 1 ping 20
1 Bob team 2 ping 30'
expect "player count from /status" 2 "$(printf '%s\n' "$status" | players_in)"
expect "a player named 'players 9' is not the count" 0 "$(printf 'players 0\n3 players 9 team 1 ping 5\n' | players_in)"

mkdir -p "$ROOT"
expect "no failures recorded" 0 "$(failures_of cccc03)"
printf 'cccc03\tfail\tdeploy\tt\nbbbb02\tfail\tverify\tt\ncccc03\tpass\t-\tt\ncccc03\tfail\tverify\tt\n' >"$ROOT/state.tsv"
expect "failures counted per SHA, passes ignored" 2 "$(failures_of cccc03)"

expect "unreadable /status of a running server is infrastructure" yes "$(is_infra wait-empty && echo yes || echo no)"
expect "pull failure is infrastructure" yes "$(is_infra pull && echo yes || echo no)"
expect "fetch failure is infrastructure" yes "$(is_infra fetch && echo yes || echo no)"
expect "failed compose up is a build failure" no "$(is_infra deploy && echo yes || echo no)"
expect "failed verify is a build failure" no "$(is_infra verify && echo yes || echo no)"

expect "first wait doesn't notify" no "$(waiting_update 1000 && echo yes || echo no)"
expect "waiting under 6 h doesn't notify" no "$(waiting_update 21000 && echo yes || echo no)"
expect "waiting 6 h notifies" yes "$(waiting_update 22600 && echo yes || echo no)"
expect "and only once" no "$(waiting_update 40000 && echo yes || echo no)"
expect "a new commit keeps the start time" "1000 1" "$(cat "$ROOT/waiting")"

mkdir -p "$ROOT/releases"
n=0
for r in r1 r2 r3 r4 r5 r6 r7; do # r1 oldest
	n=$((n + 1)); mkdir "$ROOT/releases/$r"; echo "sha-$r" >"$ROOT/releases/$r/SHA"; touch -t "20260101000$n" "$ROOT/releases/$r"
done
expect "prune drops the oldest beyond 5, never the kept one, and names their SHAs" "sha-r2" "$(prune_releases r1 | tr '\n' ' ' | sed 's/ $//')"
expect "releases left" "r1 r3 r4 r5 r6 r7" "$(ls "$ROOT/releases" | tr '\n' ' ' | sed 's/ $//')"

printf 'BV2_ADMIN_PASS=test-only-value\n' >"$RV2_STAGING_ENV"
chmod 644 "$RV2_STAGING_ENV"
expect "env file readable by others is refused" no "$(load_env >/dev/null && echo yes || echo no)"
chmod 600 "$RV2_STAGING_ENV"
expect "env file mode 600 is loaded" test-only-value "$(load_env >/dev/null && echo "$BV2_ADMIN_PASS")"
printf 'REPO=someone/fork\n' >>"$RV2_STAGING_ENV"
# bash refuses to assign a readonly variable: REPO keeps its value whatever the env file says
expect "env file can't change the repository filter" yes "$([ "$(load_env >/dev/null 2>&1; echo "$REPO")" != someone/fork ] && echo yes || echo no)"

[ $failures -eq 0 ] && echo "all passed"
exit $failures
