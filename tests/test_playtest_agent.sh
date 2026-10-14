#!/bin/bash
# Unit test for tools/playtest/agent.sh helpers: run selection, artifact digest, attempt rule, env-file mode.
# Usage: test_playtest_agent.sh <scratch dir>
set -u
here=$(cd "$(dirname "$0")" && pwd)
scratch=${1:?scratch dir}
rm -rf "$scratch" && mkdir -p "$scratch"
export RV2_AGENT_SOURCE_ONLY=1 RV2_PLAYTEST_ROOT="$scratch/root" RV2_PLAYTEST_ENV="$scratch/env"
# shellcheck source=../tools/playtest/agent.sh
. "$here/../tools/playtest/agent.sh"

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
$(run 3 cccc03 push main Hub-Tansky/RoboViolence2 success),
$(run 2 bbbb02 push main Hub-Tansky/RoboViolence2 success)]}"

expect "newest successful main push; fork PR, other branch and failed run skipped" "3 cccc03" "$(printf '%s' "$runs" | pick_run)"
expect "sha prefix selects an older main build" "2 bbbb02" "$(printf '%s' "$runs" | pick_run bbbb)"
expect "sha of a fork PR is never selected" "" "$(printf '%s' "$runs" | pick_run ffff)"
expect "no runs" "" "$(printf '{"workflow_runs":[]}' | pick_run)"

printf 'package bytes' >"$scratch/pkg.zip"
hex=$(shasum -a 256 "$scratch/pkg.zip" | cut -d' ' -f1)
artifacts="{\"artifacts\":[{\"id\":7,\"name\":\"clang-tidy-log\",\"expired\":false,\"digest\":\"sha256:00\"},
{\"id\":8,\"name\":\"$ARTIFACT\",\"expired\":true,\"digest\":\"sha256:11\"},
{\"id\":9,\"name\":\"$ARTIFACT\",\"expired\":false,\"digest\":\"sha256:$hex\"}]}"
expect "artifact id and digest without the sha256: prefix" "9 $hex" "$(printf '%s' "$artifacts" | artifact_info)"
expect "matching digest accepted" yes "$(digest_ok "$scratch/pkg.zip" "$hex" && echo yes || echo no)"
expect "different digest rejected" no "$(digest_ok "$scratch/pkg.zip" "${hex%?}0" && echo yes || echo no)"
expect "missing digest rejected" no "$(digest_ok "$scratch/pkg.zip" "" && echo yes || echo no)"

mkdir -p "$ROOT"
line() { printf 'cccc03\t3\tlocal-smoke\tlocal\t%s\t%s\t%s\t2026-10-14T00:00:00Z\n' "$@" >>"$ROOT/state.tsv"; }
expect "new SHA is due" yes "$(due cccc03 local-smoke && echo yes || echo no)"
line fail download 1
line fail download 2
expect "due after 2 failed attempts" yes "$(due cccc03 local-smoke && echo yes || echo no)"
line fail download 3
expect "not due after 3 failed attempts" no "$(due cccc03 local-smoke && echo yes || echo no)"
expect "other test of the same SHA still due" yes "$(due cccc03 staging-connect && echo yes || echo no)"
: >"$ROOT/state.tsv"
line pass - 1
expect "not due after a pass" no "$(due cccc03 local-smoke && echo yes || echo no)"

printf 'BV2_ADMIN_PASS=test-only-value\n' >"$RV2_PLAYTEST_ENV"
chmod 644 "$RV2_PLAYTEST_ENV"
expect "env file readable by others is refused" no "$(load_env >/dev/null && echo yes || echo no)"
chmod 600 "$RV2_PLAYTEST_ENV"
expect "env file mode 600 is loaded" test-only-value "$(load_env >/dev/null && echo "$BV2_ADMIN_PASS")"
expect "loaded secrets are not exported" "" "$(load_env >/dev/null && sh -c 'printf %s "${BV2_ADMIN_PASS:-}"')"

[ $failures -eq 0 ] && echo "all passed"
exit $failures
