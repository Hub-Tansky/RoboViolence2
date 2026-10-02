#!/bin/sh
# Activates the repo hooks and checks required tools. Pass --linux-packages to print the apt line.
set -e
cd "$(dirname "$0")/.."

if [ "$1" = "--linux-packages" ]; then
  # Ubuntu 24.04 package names. vcpkg builds most libraries; these are the host packages
  # it and the SDL3/GL build need on Linux.
  echo "sudo apt install build-essential ninja-build pkg-config autoconf libtool \\"
  echo "  libgl1-mesa-dev libglu1-mesa-dev \\"
  echo "  libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev libxss-dev \\"
  echo "  libwayland-dev libxkbcommon-dev wayland-protocols \\"
  echo "  libasound2-dev libpulse-dev libpipewire-0.3-dev libudev-dev libdbus-1-dev"
  exit 0
fi

git config core.hooksPath .githooks
missing=0
for t in gitleaks cmake ninja python3; do
  command -v "$t" >/dev/null 2>&1 || { echo "missing: $t" >&2; missing=1; }
done
[ -n "$VCPKG_ROOT" ] || echo "note: VCPKG_ROOT is not set (the presets use \$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake)" >&2
[ "$missing" = 0 ] && echo "hooks active (.githooks); tools found" || exit 1
