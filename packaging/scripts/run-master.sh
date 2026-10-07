#!/bin/sh
# Starts the master server (TCP 10207) from the package folder; it reads master.db and web.db here.
cd "$(dirname "$0")" && exec ./bv2master
