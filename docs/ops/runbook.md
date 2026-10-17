# Server runbook

Hosting the game server and the master server as containers (Phase B step 11). Images: `packaging/docker/Dockerfile` (targets `server` and `master`); example: `packaging/docker/compose.example.yml`.

## Images

| Image | Process | Port | Volume |
|---|---|---|---|
| `ghcr.io/hub-tansky/roboviolence2-server` | `bv2dedicated CTF` | TCP 3333 | `/var/lib/bv2`: saved config, `bv2.db`, logs, downloaded maps |
| `ghcr.io/hub-tansky/roboviolence2-master` | `bv2master` | TCP 10207 | `/var/lib/bv2`: `master.db` (bans), `web.db` (game list), seeded on first start |

- CI pushes multi-arch (amd64, arm64) `latest` and the commit SHA from `main`. GHCR packages start private: make them public, or `docker login ghcr.io` with a token that can read packages.
- Both run as uid 10001. A local build follows the host (amd64 or arm64). The game data in the image is the generated placeholder set; for the original data see below.
- Build locally instead: `docker compose -f packaging/docker/compose.example.yml build`.

## Secrets

- Never in the compose file, the image or the repository ([config/README.md](../../config/README.md)). Keep them in an env file outside the repository, readable only by you, and pass it with `--env-file` (in every command below, after `-f ...`):

  ```
  BV2_SV_PASSWORD=
  BV2_ADMIN_PASS=
  BV2_MASTER_BAN_PASS=
  BV2_PUBLIC_ADDRESS=
  ```

- The server reads `BV2_SV_PASSWORD` (join password), `BV2_ADMIN_PASS` (`zsv_adminPass`; in-game admin login needs 12 characters or more, and five failures disconnect) and `BV2_MASTER_SERVERS` and never saves them (ADR 0007). Empty means none.
- `BV2_PUBLIC_ADDRESS`: the server's public IPv4, listed by the master instead of the compose network address (ADR 0022).
- `BV2_MASTER_BAN_PASS` (master, at most 7 characters): needed for `cacheban`/`cacheunban`; empty refuses them. It crosses the network in cleartext.
- The admin user and other settings: `name value` lines (no `set`), e.g. `zsv_adminUser "boss"`, in `bv2.cfg` in the server volume, or in a file passed with `--config <file> CTF` (name the launch script too). The launch script sets no secrets, so these and the environment values stay.

## Run

From the repository root (or the folder holding the compose file):

| Task | Command |
|---|---|
| Start | `docker compose -f packaging/docker/compose.example.yml --env-file ~/bv2-secrets.env up -d` |
| Status, health | `docker compose -f packaging/docker/compose.example.yml ps` |
| Logs | `docker compose -f packaging/docker/compose.example.yml logs -f server` |
| Server console | `docker attach roboviolence2-server-1`; detach with Ctrl-P Ctrl-Q; `quit` stops the server |
| Stop | `docker compose -f packaging/docker/compose.example.yml stop` (`init: true` forwards the signal) |
| Upgrade | `docker compose -f packaging/docker/compose.example.yml pull`, then Start again |
| One commit | `BV2_IMAGE_TAG=<sha>` before any command above pins both images to that `main` commit (multi-arch); default `latest` |
| Deployed commit | `curl -k https://localhost:8443/deployed`: `srv/deployed` beside the compose file, written by the staging deploy agent ([tools/staging/](../../tools/staging/README.md)); 404 otherwise |

- A client connects with `connect <host> 3333 <password>`. The server browser needs the client's `BV2_MASTER_SERVERS=<host>:10207`.
- Upgrades across a protocol version (`GAME_VERSION_SV/CL`) need new clients too. Master and game servers share the master protocol, which has no version: upgrade them together.

## Ports

Open inbound TCP 3333 (game) and TCP 10207 (master) on the host firewall; TCP `BV2_ADMIN_PORT` only for remote admins (8443; 443 for a public name). All game traffic is TCP. Never publish the admin API port (8080).

## Remote admin

The game server's admin API ([ADR 0015](../decisions/0015-admin-http-api-behind-tls-proxy.md)) is reachable only through the `admin-proxy` service (Caddy), which terminates TLS on `BV2_ADMIN_PORT` (default 8443).

- Token: the admin password, `BV2_ADMIN_PASS` in the env file. Empty refuses every request.
- Certificate: for `localhost`, Caddy's own CA (`curl -k`, or trust the root in the `caddy-data` volume). For a public name, set `BV2_ADMIN_HOST` and `BV2_ADMIN_PORT=443` in the env file; Caddy fetches a certificate (TLS-ALPN challenge on 443).

| Task | Command |
|---|---|
| Status | `curl -k -H "Authorization: Bearer $BV2_ADMIN_PASS" https://localhost:8443/status` |
| A console command | `curl -k -H "Authorization: Bearer $BV2_ADMIN_PASS" --data 'kick Tester' https://localhost:8443/command` |
| Audit log | `docker compose -f packaging/docker/compose.example.yml exec server tail /var/lib/bv2/admin-audit.log` |

- The reply to a command is the console lines it printed. `restart`, `disconnect`, `host` and `connect` are refused (they rebuild the server). Each source may send 5 requests at once, then 1 a second (429 beyond).
- `admin-audit.log` (in the server volume): one line per request (also refused, limited, too large and timed-out ones) with UTC time, source, result and command; secrets are masked. A command is logged before it runs.
- The source is the client address Caddy reports; the server believes `X-Forwarded-For` only from `BV2_ADMIN_API_PROXY` (`admin-proxy`).
- The old UDP remote admin (`RA_*`) was removed in Phase C step 6.

## Master address

The master lists each game server at the address its connection comes from. In compose that is the container's internal address: set `BV2_PUBLIC_ADDRESS` so remote clients can join from the browser ([ADR 0022](../decisions/0022-advertised-server-address.md)).

## Original game data

Keep your own copy outside the repository. Mount the folder that holds its `main/` read-only and point `BV2_DATA_DIR` at it:

```yaml
    environment:
      BV2_DATA_DIR: /opt/bv2-data
    volumes: ["/path/to/your/data:/opt/bv2-data:ro"]
```

## Backups

The volumes hold all state: back up `master-data` (bans) and `server-data` (config, `bv2.db`). For example:

```
docker run --rm -v roboviolence2_master-data:/data -v "$PWD":/backup ubuntu:24.04 tar -czf /backup/master-data.tgz -C /data .
```

Restore by extracting into the volume before `up`.

## Without compose

`docker run` needs `--init` (stop signals) and `-it` (the console): `docker run -d -it --init -p 3333:3333 -v bv2-server:/var/lib/bv2 ghcr.io/hub-tansky/roboviolence2-server`.
