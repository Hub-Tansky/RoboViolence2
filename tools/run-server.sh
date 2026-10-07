#!/bin/sh
# Starts the dedicated server from the package folder. Usage: ./run-server.sh [launch script, default CTF]
cd "$(dirname "$0")" && exec ./bv2dedicated "${1:-CTF}"
