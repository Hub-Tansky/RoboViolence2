#!/bin/bash
# Test for tools/cloud-setup.sh (ROBO-225): identity and hooksPath per repository, a bad argument only warns, exit 0.
# Stub gitleaks and pre-commit on PATH, so nothing is downloaded or installed.
# Usage: test_cloud_setup.sh <scratch dir>
set -u
script=$(cd "$(dirname "$0")/../tools" && pwd)/cloud-setup.sh
scratch=${1:?scratch dir}
rm -rf "$scratch" && mkdir -p "$scratch/bin" "$scratch/game/.githooks" "$scratch/docs"
printf '#!/bin/sh\necho v8.30.1\n' > "$scratch/bin/gitleaks"
printf '#!/bin/sh\necho pre-commit 4.3.0\n' > "$scratch/bin/pre-commit"
chmod +x "$scratch/bin/"*
git -C "$scratch/game" init -q && git -C "$scratch/docs" init -q

failures=0
expect() { # expect <name> <expected> <actual>
	if [ "$2" = "$3" ]; then echo "ok   $1"; else echo "FAIL $1: expected '$2', got '$3'"; failures=$((failures + 1)); fi
}

out=$(PATH="$scratch/bin:$PATH" bash "$script" "$scratch/game" "$scratch/docs" "$scratch/missing")
expect "exits 0 with a bad argument" 0 $?
expect "game: email" "337104978+Hub-Tansky@users.noreply.github.com" "$(git -C "$scratch/game" config user.email)"
expect "game: name" "Hub-Tansky" "$(git -C "$scratch/game" config user.name)"
expect "game: hooksPath" ".githooks" "$(git -C "$scratch/game" config core.hooksPath)"
expect "docs: email" "337104978+Hub-Tansky@users.noreply.github.com" "$(git -C "$scratch/docs" config user.email)"
expect "docs without .githooks: no hooksPath" "" "$(git -C "$scratch/docs" config core.hooksPath)"
expect "pinned gitleaks is kept" 1 "$(grep -c 'gitleaks 8.30.1 already installed' <<< "$out")"
expect "a bad argument warns" 1 "$(grep -c "WARNING: $scratch/missing is not a git repository" <<< "$out")"
expect "every line is a step line" 0 "$(grep -vc '^cloud-setup: ' <<< "$out")"
expect "gitleaks pin matches secret-scan.yml" 1 \
	"$(grep -c "gitleaks/releases/download/v$(sed -n 's/^GITLEAKS_VERSION=\([0-9.]*\).*/\1/p' "$script")/" "$(dirname "$script")/../.github/workflows/secret-scan.yml")"

[ "$failures" = 0 ] && echo "cloud_setup: ok" || echo "cloud_setup: $failures failed"
exit "$failures"
