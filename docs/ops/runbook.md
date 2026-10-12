# Server runbook

Hosting the game server and the master server as containers (Phase B step 11). Images: `packaging/docker/Dockerfile` (targets `server` and `master`); example: `packaging/docker/compose.example.yml`.

## Images

| Image | Process | Port | Volume |
|---|---|---|---|
| `ghcr.io/hub-tansky/roboviolence2-server` | `bv2dedicated CTF` | TCP 3333 | `/var/lib/bv2`: saved config, `bv2.db`, logs, downloaded maps |
| `ghcr.io/hub-tansky/roboviolence2-master` | `bv2master` | TCP 10207 | `/var/lib/bv2`: `master.db` (bans), `web.db` (game list), seeded on first start |

- CI pushes `latest` and the commit SHA from `main`. GHCR packages start private: make them public, or `docker login ghcr.io` with a token that can read packages.
- Both run as uid 10001, linux/amd64 only (ARM hosts need emulation). The game data in the image is the generated placeholder set; for the original data see below.
- Build locally instead: `docker compose -f packaging/docker/compose.example.yml build`.

## Secrets

- Never in the compose file, the image or the repository ([config/README.md](../../config/README.md)). Keep them in an env file outside the repository, readable only by you, and pass it with `--env-file` (in every command below, after `-f ...`):

  ```
  BV2_SV_PASSWORD=
  BV2_ADMIN_PASS=
  ```

- The server reads `BV2_SV_PASSWORD` (join password), `BV2_ADMIN_PASS` (`zsv_adminPass`) and `BV2_MASTER_SERVERS` and never saves them (ADR 0007). Empty means none.
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

- A client connects with `connect <host> 3333 <password>`. The server browser needs the client's `BV2_MASTER_SERVERS=<host>:10207`.
- Upgrades across a protocol version (`GAME_VERSION_SV/CL`) need new clients too. Master and game servers share the master protocol, which has no version: upgrade them together.

## Ports

Open inbound TCP 3333 (game) and TCP 10207 (master) on the host firewall; nothing else. All game traffic is TCP.

## Master address limitation

The master lists each game server at the address its connection comes from. With master and server on one host in compose, that is the container's internal address: clients on the host can join from the browser, remote clients cannot (they join by `connect <public host>`). PNS-44.

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
