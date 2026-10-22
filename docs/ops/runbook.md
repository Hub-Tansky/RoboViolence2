# Server runbook

Hosting the game server and the master server as containers (Phase B step 11). Images: `packaging/docker/Dockerfile` (targets `server` and `master`); example: `packaging/docker/compose.example.yml`.

## Images

| Image | Process | Port | Volume |
|---|---|---|---|
| `ghcr.io/hub-tansky/roboviolence2-server` | `bv2dedicated CTF` (service `server`), `bv2dedicated TDM` (service `tdm`) | TCP 3333 (CTF), 3334 (TDM) | `/var/lib/bv2`: saved config, `bv2.db`, logs, downloaded maps; one volume per server (`server-data`, `tdm-data`) |
| `ghcr.io/hub-tansky/roboviolence2-master` | `bv2master` | TCP 10207 | `/var/lib/bv2`: `master.db` (bans), `web.db` (game list), seeded on first start |

- CI pushes multi-arch (amd64, arm64) `latest` and the commit SHA from `main`. GHCR packages start private: make them public, or `docker login ghcr.io` with a token that can read packages.
- Both run as uid 10001. A local build follows the host (amd64 or arm64). The game data in the image is the generated placeholder set; for the original data see below.
- Build locally instead: `docker compose -f packaging/docker/compose.example.yml build`.

## Game servers

The compose example runs two game servers from the same image and the same env file: `server` (CTF, `content/LaunchScript/CTF.cfg`) and `tdm` (TDM, `content/LaunchScript/TDM.cfg`). They differ only in the launch script, the port and the volume. Both register with the master; with `BV2_PUBLIC_ADDRESS` set, the master lists both under that address, told apart by port.

- Another mode or a second server of one mode: copy the `tdm` service (it builds on the `x-game-server` anchor), give it its own launch script (with its own `sv_port`), port, volume and `BV2_HEALTH_PORT`, add a `handle_path` route for its admin API to the Caddyfile and an entry to `GAME_SERVERS` in `tools/staging/deploy.sh`.
- `BV2_HEALTH_PORT` (default 3333) only tells the image's healthcheck which port to watch: set it to the launch script's `sv_port`, or the container stays unhealthy.

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
| Logs | `docker compose -f packaging/docker/compose.example.yml logs -f server` (CTF) or `... logs -f tdm` |
| Server console | `docker attach roboviolence2-server-1` (CTF) or `roboviolence2-tdm-1`; detach with Ctrl-P Ctrl-Q; `quit` stops the server |
| Stop | `docker compose -f packaging/docker/compose.example.yml stop` (`init: true` forwards the signal) |
| Upgrade | `docker compose -f packaging/docker/compose.example.yml pull`, then Start again |
| One commit | `BV2_IMAGE_TAG=<sha>` before any command above pins both images to that `main` commit (multi-arch); default `latest` |
| Deployed commit | `curl -k https://localhost:8443/deployed`: `srv/deployed` beside the compose file, written by the staging deploy agent ([tools/staging/](../../tools/staging/README.md)); 404 otherwise |

- A client connects with `connect <host> 3333 <password>` (CTF) or `connect <host> 3334 <password>` (TDM). The server browser needs the client's `BV2_MASTER_SERVERS=<host>:10207`.
- Upgrades across a protocol version (`GAME_VERSION_SV/CL`) need new clients too. Master and game servers share the master protocol, which has no version: upgrade them together.

## Ports

Open inbound TCP 3333 (CTF), TCP 3334 (TDM) and TCP 10207 (master) on the host firewall; TCP `BV2_ADMIN_PORT` only for remote admins (8443; 443 for a public name). All game traffic is TCP. Never publish the admin API port (8080).

## Remote admin

Each game server's admin API ([ADR 0015](../decisions/0015-admin-http-api-behind-tls-proxy.md)) is reachable only through the `admin-proxy` service (Caddy), which terminates TLS on `BV2_ADMIN_PORT` (default 8443). CTF answers at `/`, TDM at `/tdm/` (`/tdm/status`, `/tdm/command`); both take the same token.

- Token: the admin password, `BV2_ADMIN_PASS` in the env file. Empty refuses every request.
- Certificate: for `localhost`, Caddy's own CA (`curl -k`, or trust the root in the `caddy-data` volume). For a public name, set `BV2_ADMIN_HOST` and `BV2_ADMIN_PORT=443` in the env file; Caddy fetches a certificate (TLS-ALPN challenge on 443).

| Task | Command |
|---|---|
| Status | `curl -k -H "Authorization: Bearer $BV2_ADMIN_PASS" https://localhost:8443/status` |
| A console command | `curl -k -H "Authorization: Bearer $BV2_ADMIN_PASS" --data 'kick Tester' https://localhost:8443/command` |
| TDM status | `curl -k -H "Authorization: Bearer $BV2_ADMIN_PASS" https://localhost:8443/tdm/status` |
| Audit log | `docker compose -f packaging/docker/compose.example.yml exec server tail /var/lib/bv2/admin-audit.log` (`exec tdm` for TDM) |

- The reply to a command is the console lines it printed. `restart`, `disconnect`, `host` and `connect` are refused (they rebuild the server). Each source may send 5 requests at once, then 1 a second (429 beyond).
- `admin-audit.log` (in the server volume): one line per request (also refused, limited, too large and timed-out ones) with UTC time, source, result and command; secrets are masked. A command is logged before it runs.
- The source is the client address Caddy reports; the server believes `X-Forwarded-For` only from `BV2_ADMIN_API_PROXY` (`admin-proxy`).
- The old UDP remote admin (`RA_*`) was removed in Phase C step 6.

## Master address

The master lists each game server at the address its connection comes from. In compose that is the container's internal address: set `BV2_PUBLIC_ADDRESS` so remote clients can join from the browser. Both servers take it from the same env file ([ADR 0022](../decisions/0022-advertised-server-address.md)).

## Original game data

Keep your own copy outside the repository. Mount the folder that holds its `main/` read-only and point `BV2_DATA_DIR` at it:

```yaml
    environment:
      BV2_DATA_DIR: /opt/bv2-data
    volumes: ["/path/to/your/data:/opt/bv2-data:ro"]
```

## Backups

The volumes hold all state: back up `master-data` (bans), `server-data` and `tdm-data` (config, `bv2.db`). For example:

```
docker run --rm -v roboviolence2_master-data:/data -v "$PWD":/backup ubuntu:24.04 tar -czf /backup/master-data.tgz -C /data .
```

Restore by extracting into the volume before `up`.

## Production

Production runs the same compose file and the same deploy agent as staging ([tools/staging/](../../tools/staging/README.md)), with three differences. The host's address, provider and access stay in the owner's private wiki, never in the repo.

- **Pinned, by hand.** No cron. The owner deploys one `main` commit (a release commit once releases carry server images) with `deploy.sh --sha <full sha>`. It still waits for 0 players on every game server and rolls back on failure.
- **Own paths and name.** A wrapper sets `RV2_STAGING_ROOT=/var/lib/rv2-prod`, `RV2_STAGING_ENV=/etc/rv2-prod.env` and `RV2_DEPLOY_LABEL=prod`, so notifications say `RV2 prod ...` and staging's state is never touched.
- **Env file.** As for staging, plus `BV2_MASTER_BAN_PASS`; `BV2_ADMIN_HOST` is the public admin name and `BV2_ADMIN_PORT=443`; `BV2_PUBLIC_ADDRESS` is the host's IPv4. Mode 600, only on the host.

| Task | Do |
|---|---|
| Deploy a commit | `<wrapper> --sha <full sha>`; check `https://<admin host>/deployed` and that the master lists every game server |
| Roll back | the same with the previous SHA (`deployed` and `state.tsv` under the prod root name it) |
| Add a game server | as in [Game servers](#game-servers); open its port in the host firewall and add an external monitor for it |
| Fail over to another host | new host from the same steps, restore the latest backup into the volumes and env file, set the new `BV2_PUBLIC_ADDRESS`, deploy the same SHA, then point the `master` and `admin` DNS records at it |

Backups: a nightly job copies the env file and every `roboviolence2_*` volume off the host (see [Backups](#backups) for what they hold); restore one snapshot to a scratch folder now and then to prove it works.

## Without compose

`docker run` needs `--init` (stop signals) and `-it` (the console): `docker run -d -it --init -p 3333:3333 -v bv2-server:/var/lib/bv2 ghcr.io/hub-tansky/roboviolence2-server`.
