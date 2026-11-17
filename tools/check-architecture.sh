#!/bin/sh
# Fails when ARCHITECTURE.md's "## Modules" rows and the module directories disagree (ADR 0063):
# every directory under engine/, packaging/ or content/, and game/src, masterserver/src, tests and tools,
# that holds tracked files needs a row; every row must name a directory with tracked files.
# Usage: tools/check-architecture.sh [repo dir] (default: this repository)
set -e
cd "${1:-$(dirname "$0")/..}"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
git ls-files | awk -F/ '
	NF > 2 && ($1 == "engine" || $1 == "packaging" || $1 == "content") { print $1 "/" $2 }
	NF > 2 && (($1 == "game" && $2 == "src") || ($1 == "masterserver" && $2 == "src")) { print $1 "/" $2 }
	NF > 1 && ($1 == "tests" || $1 == "tools") { print $1 }
' | sort -u > "$tmp/needed"
awk '/^## /{in_mod = ($0 == "## Modules")} in_mod' ARCHITECTURE.md \
	| sed -n 's/^| `\([^`]*\)\/` | .*/\1/p' | sort -u > "$tmp/rows"
status=0
comm -23 "$tmp/needed" "$tmp/rows" | sed 's/^/no row in ARCHITECTURE.md "## Modules": /' > "$tmp/out"
while read -r d; do
	[ -n "$(git ls-files -- "$d/" | head -n 1)" ] || echo "row names a missing directory: $d/" >> "$tmp/out"
done < "$tmp/rows"
if [ -s "$tmp/out" ]; then cat "$tmp/out"; status=1; fi
[ -s "$tmp/rows" ] || { echo "ARCHITECTURE.md has no \"## Modules\" rows"; status=1; }
[ "$status" = 0 ] && echo "ok: ARCHITECTURE.md has a row for every module directory"
exit $status
