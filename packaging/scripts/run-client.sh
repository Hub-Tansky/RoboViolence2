#!/bin/sh
# Starts the client from the package folder. Arguments are one console command, e.g. ./run-client.sh connect 127.0.0.1
cd "$(dirname "$0")"
app="Robo Violence 2.app/Contents/MacOS/bv2"  # macOS package
if [ -x "$app" ]; then exec "$app" "$@"; fi
exec ./bv2 "$@"
