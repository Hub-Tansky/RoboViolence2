#!/bin/sh
# Fails if ARCHITECTURE.md's file inventory and `git ls-files` disagree.
set -e
cd "$(dirname "$0")/.."
git ls-files | sort > /tmp/arch-tracked.$$
sed -n 's/^| `\([^`]*\)` | .*/\1/p' ARCHITECTURE.md | sort > /tmp/arch-listed.$$
status=0
if ! comm -23 /tmp/arch-tracked.$$ /tmp/arch-listed.$$ | sed 's/^/unlisted: /' | grep .; then :; else status=1; fi
if ! comm -13 /tmp/arch-tracked.$$ /tmp/arch-listed.$$ | sed 's/^/stale: /' | grep .; then :; else status=1; fi
rm -f /tmp/arch-tracked.$$ /tmp/arch-listed.$$
[ "$status" = 0 ] && echo "ok: ARCHITECTURE.md matches git ls-files"
exit $status
