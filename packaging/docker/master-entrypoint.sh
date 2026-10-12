#!/bin/sh
# The master reads master.db and web.db from its working directory, the volume: seed them on first start.
set -e
for db in master.db web.db; do
	[ -f "$db" ] || cp "/opt/bv2/seed/$db" .
done
exec /opt/bv2/bv2master
