#!/bin/sh
# Starts the client from the package folder. Arguments are one console command, e.g. ./run-client.sh connect 127.0.0.1
cd "$(dirname "$0")"
if [ -x bv2.app/Contents/MacOS/bv2 ]; then exec bv2.app/Contents/MacOS/bv2 "$@"; fi
exec ./bv2 "$@"
