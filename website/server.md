# Your own game server

The Windows and Linux downloads include the game server (`bv2dedicated`). On macOS it is in `roboviolence2-macos-arm64.zip` on the [releases page](https://github.com/Hub-Tansky/RoboViolence2/releases), not in the `.dmg`. For an always-on host, use the Docker image.

A new server shows up in the in-game server list by itself.

## Ports

Open inbound **TCP 3333**. A team deathmatch server (launch script `TDM`) uses **TCP 3334**. All game traffic is TCP.

## Quick start from a download

In the unpacked folder (Linux and macOS shown; on Windows double-click `run-server.cmd`):

```
./run-server.sh
```

This starts capture the flag; `./run-server.sh TDM` starts team deathmatch. Players join from the server list or with the console command `connect <your address>`. The server reads your BaboViolent 2 maps from `BV2_DATA_DIR` (the folder that contains `main/`) and sends missing maps to players.

## Docker

Image for amd64 and arm64: `ghcr.io/hub-tansky/roboviolence2-server`. One capture the flag server with its saved state in a volume:

```
docker run -dit --name rv2-ctf --restart unless-stopped \
  -p 3333:3333 -v rv2-ctf:/var/lib/bv2 \
  --env-file ~/rv2-server.env \
  ghcr.io/hub-tansky/roboviolence2-server
```

Keep settings and secrets in that env file:

```
# join password, empty for none
BV2_SV_PASSWORD=
# admin login password
BV2_ADMIN_PASS=
# your public IPv4, so the server list shows a reachable address
BV2_PUBLIC_ADDRESS=
```

For team deathmatch, add `TDM` after the image name, publish `3334:3334` instead and add `-e BV2_HEALTH_PORT=3334` so the container health check watches that port. `docker attach rv2-ctf` opens the server console; `Ctrl-P` `Ctrl-Q` leaves it running. To use your BaboViolent 2 maps, mount the folder that holds `main/` read-only and point `BV2_DATA_DIR` at it:

```
-v /path/to/BaboViolent2/Content:/opt/bv2-data:ro -e BV2_DATA_DIR=/opt/bv2-data
```

The [server runbook](https://github.com/Hub-Tansky/RoboViolence2/blob/main/docs/ops/runbook.md) covers logs, the server console, upgrades, remote admin over HTTPS and backups.
