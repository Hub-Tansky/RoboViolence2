#!/bin/bash
# Test for tools/cloud-setup.sh (ROBO-225): identity and hooksPath per repository, a bad argument only warns, exit 0;
# the Claude guard hook in the user settings, kept beside existing settings and added once (ROBO-256).
# Stub gitleaks and pre-commit on PATH, so nothing is downloaded or installed.
# Usage: test_cloud_setup.sh <scratch dir>
set -u
script=$(cd "$(dirname "$0")/../tools" && pwd)/cloud-setup.sh
scratch=${1:?scratch dir}
rm -rf "$scratch" && mkdir -p "$scratch/bin" "$scratch/game/.githooks" "$scratch/docs" "$scratch/home/.claude"
printf '{"model": "x", "permissions": {"deny": ["Read(//tmp/a)"]}}\n' > "$scratch/home/.claude/settings.json"
printf '#!/bin/sh\necho v8.30.1\n' > "$scratch/bin/gitleaks"
printf '#!/bin/sh\necho pre-commit 4.3.0\n' > "$scratch/bin/pre-commit"
chmod +x "$scratch/bin/"*
git -C "$scratch/game" init -q && git -C "$scratch/docs" init -q

failures=0
expect() { # expect <name> <expected> <actual>
	if [ "$2" = "$3" ]; then echo "ok   $1"; else echo "FAIL $1: expected '$2', got '$3'"; failures=$((failures + 1)); fi
}

out=$(HOME="$scratch/home" PATH="$scratch/bin:$PATH" bash "$script" "$scratch/game" "$scratch/docs" "$scratch/missing")
expect "exits 0 with a bad argument" 0 $?
expect "game: email" "337104978+Hub-Tansky@users.noreply.github.com" "$(git -C "$scratch/game" config user.email)"
expect "game: name" "Hub-Tansky" "$(git -C "$scratch/game" config user.name)"
expect "game: hooksPath" ".githooks" "$(git -C "$scratch/game" config core.hooksPath)"
expect "docs: email" "337104978+Hub-Tansky@users.noreply.github.com" "$(git -C "$scratch/docs" config user.email)"
expect "docs without .githooks: no hooksPath" "" "$(git -C "$scratch/docs" config core.hooksPath)"
expect "pinned gitleaks is kept" 1 "$(grep -c 'gitleaks 8.30.1 already installed' <<< "$out")"
expect "a bad argument warns" 1 "$(grep -c "WARNING: $scratch/missing is not a git repository" <<< "$out")"
settings="$scratch/home/.claude/settings.json"
guard_cmd=$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["hooks"]["PreToolUse"][0]["hooks"][0]["command"])' "$settings")
expect "guard hook with the absolute path" "python3 \"$(cd "$(dirname "$script")/.." && pwd)/.claude/hooks/guard.py\"" "$guard_cmd"
expect "existing settings kept" "x" "$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["model"])' "$settings")"
expect "deny rules added after the existing one" '["Read(//tmp/a)", "Read(**/.env*)", "Read(~/.ssh/**)", "Read(//etc/rv2-prod.env)"]' \
	"$(python3 -c 'import json,sys; print(json.dumps(json.load(open(sys.argv[1]))["permissions"]["deny"]))' "$settings")"
HOME="$scratch/home" PATH="$scratch/bin:$PATH" bash "$script" "$scratch/game" > /dev/null
expect "a second run adds nothing" '1 4' \
	"$(python3 -c 'import json,sys; d=json.load(open(sys.argv[1])); print(len(d["hooks"]["PreToolUse"]), len(d["permissions"]["deny"]))' "$settings")"
expect "every line is a step line" 0 "$(grep -vc '^cloud-setup: ' <<< "$out")"
expect "gitleaks pin matches secret-scan.yml" 1 \
	"$(grep -c "gitleaks/releases/download/v$(sed -n 's/^GITLEAKS_VERSION=\([0-9.]*\).*/\1/p' "$script")/" "$(dirname "$script")/../.github/workflows/secret-scan.yml")"

[ "$failures" = 0 ] && echo "cloud_setup: ok" || echo "cloud_setup: $failures failed"
exit "$failures"
