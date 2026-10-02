#!/bin/sh
# Activates the repo hooks and checks required tools.
set -e
cd "$(dirname "$0")/.."
git config core.hooksPath .githooks
missing=0
for t in gitleaks cmake python3; do
  command -v "$t" >/dev/null 2>&1 || { echo "missing: $t" >&2; missing=1; }
done
[ "$missing" = 0 ] && echo "hooks active (.githooks); tools found" || exit 1
