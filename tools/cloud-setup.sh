#!/usr/bin/env bash
# Cloud session setup (ROBO-225): the commit hooks a Mac clone gets from tools/setup-dev.sh, for a fresh Claude Code
# cloud container. The environment's setup script runs it (owner action O3), for example:
#   bash /home/user/RoboViolence2/tools/cloud-setup.sh /home/user/RoboViolence2 /home/user/rv2-docs /home/user/rv2-website
# Arguments: repository directories (default: the repository holding this script). Each gets the Hub-Tansky commit
# identity; the ones with a .githooks/ directory also get core.hooksPath. Installs gitleaks (pinned, checksum checked)
# and pre-commit. Prints one line per step. A failed step prints a WARNING and the script still exits 0, so a session
# always starts; the hooks then refuse commits that need the missing tool, which is the safe direction.
set -u

GITLEAKS_VERSION=8.30.1  # same as .github/workflows/secret-scan.yml
GITLEAKS_SHA256_x64=551f6fc83ea457d62a0d98237cbad105af8d557003051f41f3e7ca7b3f2470eb
GITLEAKS_SHA256_arm64=e4a487ee7ccd7d3a7f7ec08657610aa3606637dab924210b3aee62570fb4b080
PRE_COMMIT_VERSION=4.3.0
EMAIL=337104978+Hub-Tansky@users.noreply.github.com
NAME=Hub-Tansky

ok() { echo "cloud-setup: ok: $*"; }
warn() { echo "cloud-setup: WARNING: $*"; }

install_gitleaks() {
	if command -v gitleaks >/dev/null 2>&1 && gitleaks version 2>/dev/null | grep -q "$GITLEAKS_VERSION"; then
		ok "gitleaks $GITLEAKS_VERSION already installed"
		return
	fi
	local arch sum tmp bin
	case "$(uname -m)" in
		x86_64) arch=x64 sum=$GITLEAKS_SHA256_x64 ;;
		aarch64 | arm64) arch=arm64 sum=$GITLEAKS_SHA256_arm64 ;;
		*) warn "gitleaks: no pinned build for $(uname -m)"; return ;;
	esac
	tmp=$(mktemp -d)
	if ! curl -sSfL --retry 3 -o "$tmp/gitleaks.tgz" \
		"https://github.com/gitleaks/gitleaks/releases/download/v$GITLEAKS_VERSION/gitleaks_${GITLEAKS_VERSION}_linux_$arch.tar.gz"; then
		warn "gitleaks: download failed (is github.com allowed by the environment's network policy?)"
	elif ! echo "$sum  $tmp/gitleaks.tgz" | sha256sum -c --quiet - >/dev/null 2>&1; then
		warn "gitleaks: checksum mismatch, not installed"
	elif ! tar -xzf "$tmp/gitleaks.tgz" -C "$tmp" gitleaks; then
		warn "gitleaks: could not unpack"
	else
		bin=/usr/local/bin
		[ -w "$bin" ] || { bin=$HOME/.local/bin; mkdir -p "$bin"; }
		if install -m 755 "$tmp/gitleaks" "$bin/gitleaks"; then ok "gitleaks $GITLEAKS_VERSION in $bin"; else warn "gitleaks: could not install to $bin"; fi
	fi
	rm -rf "$tmp"
}

install_pre_commit() {
	if command -v pre-commit >/dev/null 2>&1; then
		ok "pre-commit already installed ($(pre-commit --version 2>/dev/null))"
		return
	fi
	# The container's Python is externally managed (PEP 668), and pipx is not installed.
	if python3 -m pip install --quiet --disable-pip-version-check --break-system-packages "pre-commit==$PRE_COMMIT_VERSION" >/dev/null 2>&1 \
		|| python3 -m pip install --quiet --disable-pip-version-check --user "pre-commit==$PRE_COMMIT_VERSION" >/dev/null 2>&1; then
		ok "pre-commit $PRE_COMMIT_VERSION"
	else
		warn "pre-commit: pip install failed"
	fi
}

setup_repo() {
	local dir=$1
	if ! git -C "$dir" rev-parse --git-dir >/dev/null 2>&1; then
		warn "$dir is not a git repository"
		return
	fi
	if git -C "$dir" config user.email "$EMAIL" && git -C "$dir" config user.name "$NAME"; then
		ok "$dir: commit identity $NAME <$EMAIL>"
	else
		warn "$dir: could not set the commit identity"
	fi
	if [ -d "$dir/.githooks" ]; then
		if git -C "$dir" config core.hooksPath .githooks; then ok "$dir: core.hooksPath .githooks"; else warn "$dir: could not set core.hooksPath"; fi
	fi
}

install_gitleaks
install_pre_commit
[ $# -gt 0 ] || set -- "$(cd "$(dirname "$0")/.." && pwd)"
for dir in "$@"; do setup_repo "$dir"; done
exit 0
