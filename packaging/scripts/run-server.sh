#!/bin/sh
# Starts the dedicated server from the package folder. Arguments go to bv2dedicated (default: launch script CTF).
cd "$(dirname "$0")" || exit 1
[ $# -eq 0 ] && set -- CTF
exec ./bv2dedicated "$@"
