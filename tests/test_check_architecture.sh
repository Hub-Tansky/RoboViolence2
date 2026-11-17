#!/bin/bash
# Test for tools/check-architecture.sh (ROBO-235, ADR 0061): module rows, not file rows.
# Usage: test_check_architecture.sh <scratch dir>
set -u
script=$(cd "$(dirname "$0")/../tools" && pwd)/check-architecture.sh
scratch=${1:?scratch dir}
rm -rf "$scratch" && mkdir -p "$scratch" && cd "$scratch" || exit 1
git init -q . && git config user.email t@example.invalid && git config user.name t

failures=0
expect() { # expect <name> <expected exit> <expected output fragment>
	out=$(sh "$script" "$scratch" 2>&1); rc=$?
	if [ "$rc" = "$2" ] && printf '%s' "$out" | grep -qF -- "$3"; then echo "ok   $1"; else echo "FAIL $1: exit $rc, output: $out"; failures=$((failures + 1)); fi
}
add() { mkdir -p "$(dirname "$1")" && echo x > "$1" && git add "$1"; }
rows() { # rows <dir>...: ARCHITECTURE.md with one Modules row per argument, plus a file-looking row elsewhere
	{ echo "# Architecture"; echo; echo "| \`notes/\` | outside the Modules section |"; echo; echo "## Modules"; echo
	  echo "| directory | purpose |"; echo "|---|---|"
	  for d in "$@"; do echo "| \`$d/\` | purpose |"; done; echo; echo "## Later"; } > ARCHITECTURE.md
}

add game/src/a.cpp; add engine/zeven/src/b.cpp; add tests/test_a.cpp; add tools/x.py; add docs/y.md; add README.md
rows game/src engine/zeven tests tools
expect "every module directory has a row" 0 "ok:"

add tests/test_new.cpp; add tests/corpus/sample.bin
expect "a new test file needs no ARCHITECTURE.md change" 0 "ok:"

add engine/newmod/src/c.cpp
expect "a new module directory without a row fails" 1 "no row in ARCHITECTURE.md \"## Modules\": engine/newmod"

rows game/src engine/zeven engine/newmod tests tools packaging/gone
expect "a row naming a missing directory fails" 1 "row names a missing directory: packaging/gone/"

rows game/src engine/zeven engine/newmod tests tools
expect "rows outside the Modules section don't count" 0 "ok:"

add content/maps/m.bvm
expect "content subdirectories need a row" 1 "content/maps"

echo "$failures failures"
[ "$failures" = 0 ]
