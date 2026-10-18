#!/bin/bash
# Unit test for tools/playtest/agent.sh helpers: run selection, artifact digest, attempt rule, env-file mode, launch script.
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
record() { record_state cccc03 3 local-smoke local "$@"; }
expect "new SHA is due" yes "$(due cccc03 local-smoke && echo yes || echo no)"
expect "first failure is attempt 1" 1 "$(record fail client-join)"
expect "second failure is attempt 2" 2 "$(record fail server-port)"
expect "state line columns" "cccc03 3 local-smoke local fail server-port 2" "$(last_state cccc03 local-smoke | cut -f1-7 | tr '\t' ' ')"
expect "due after 2 failed attempts" yes "$(due cccc03 local-smoke && echo yes || echo no)"
record fail verify >/dev/null
expect "not due after 3 failed attempts" no "$(due cccc03 local-smoke && echo yes || echo no)"
expect "other test of the same SHA still due" yes "$(due cccc03 staging-connect && echo yes || echo no)"
: >"$ROOT/state.tsv"
expect "a pass records stage -" "pass -" "$(record pass client-join >/dev/null; last_state cccc03 local-smoke | cut -f5,6 | tr '\t' ' ')"
expect "not due after a pass" no "$(due cccc03 local-smoke && echo yes || echo no)"

expect "download failure is infrastructure" yes "$(is_infra download && echo yes || echo no)"
expect "busy port is infrastructure" yes "$(is_infra ports-busy && echo yes || echo no)"
expect "no console session is infrastructure" yes "$(is_infra gui-session && echo yes || echo no)"
expect "failed join is a build failure" no "$(is_infra client-join && echo yes || echo no)"
expect "digest mismatch is a build failure" no "$(is_infra verify && echo yes || echo no)"

mkdir -p "$ROOT/versions"
n=0
for v in a b c d; do n=$((n + 1)); mkdir "$ROOT/versions/$v"; touch -t "20260101000$n" "$ROOT/versions/$v"; done # a oldest
ln -sfn "$ROOT/versions/a" "$ROOT/current"
prune "$ROOT/versions" 2
expect "prune keeps the newest and the current one" "a c d" "$(ls "$ROOT/versions" | tr '\n' ' ' | sed 's/ $//')"

printf 'BV2_ADMIN_PASS=test-only-value\n' >"$RV2_PLAYTEST_ENV"
chmod 644 "$RV2_PLAYTEST_ENV"
expect "env file readable by others is refused" no "$(load_env >/dev/null && echo yes || echo no)"
chmod 600 "$RV2_PLAYTEST_ENV"
expect "env file mode 600 is loaded" test-only-value "$(load_env >/dev/null && echo "$BV2_ADMIN_PASS")"
expect "loaded secrets are not exported" "" "$(load_env >/dev/null && sh -c 'printf %s "${BV2_ADMIN_PASS:-}"')"

printf '//--- comment\n\nset sv_port 3333\ndedicate CTF-Placeholder\naddmap CTF-Placeholder\nset sv_enableVote true\nendscript\nafter end\n' >"$scratch/CTF.cfg"
expect "launch script with the map swapped, no comments, blanks or endscript" \
	"set sv_port 3333|dedicate CTF-Alert|addmap CTF-Alert|set sv_enableVote true|" \
	"$(launch_script "$scratch/CTF.cfg" CTF-Alert | tr '\n' '|')"
expect "map name with other characters refused" no "$(launch_script "$scratch/CTF.cfg" 'x/;e' >/dev/null && echo yes || echo no)"

[ $failures -eq 0 ] && echo "all passed"
exit $failures
