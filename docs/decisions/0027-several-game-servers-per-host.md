# 0027: Several game servers on one host share one image, one env file and one admin proxy

- Status: Accepted
- Date: 2026-10-20
- Affects: Project item "TDM server beside CTF in the server image and compose example" ([Project](https://github.com/users/Hub-Tansky/projects/1)), `packaging/docker/`, `content/LaunchScript/TDM.cfg`, `tools/staging/deploy.sh`

## Context
- Production (decided 2026-10-20) runs the master, a CTF server on 3333 and a TDM server on 3334 on one VPS.
- The server image's healthcheck watched port 3333 only, and the admin proxy, the staging deploy agent and CI knew one game server.

## Decision
- One server image; the launch script argument (`CTF`, `TDM`) picks the mode, and each launch script sets its own `sv_port`.
- `BV2_HEALTH_PORT` (default 3333) tells the image's healthcheck which port to watch; it doesn't change `sv_port`.
- The compose example runs `server` (CTF) and `tdm` with the same environment (one env file, one `BV2_PUBLIC_ADDRESS`, one admin token) and a volume each.
- One Caddy site serves both admin APIs: CTF at `/`, TDM at `/tdm/` (prefix stripped).
- Owner-approved [GUI] exception (2026-10-21), amending [ADR 0019](0019-in-game-name-and-fork-credits.md): the shipped launch scripts' `sv_gameName` say Robo Violence 2 (`Robo Violence 2 CTF Server`, `Robo Violence 2 TDM Server`); other visible names stay as ADR 0009 says.
- The staging deploy agent waits until every running game server in its `GAME_SERVERS` list reports 0 players and verifies every running one; `up --remove-orphans` keeps the running services equal to the release's, so a release from before TDM has no `tdm` container to check.

## Consequences
- A further server is a service on the compose file's `x-game-server` anchor (launch script, port, volume, `BV2_HEALTH_PORT`), a launch script, one Caddy route and one `GAME_SERVERS` entry in `tools/staging/deploy.sh`.
- Old CTF admin URLs keep working.
- Rejected: a second published admin port per server (one more firewall opening and certificate host per server); a healthcheck that accepts any listening socket (the admin API's socket would pass it while the game port is down); `BV2_HEALTH_PORT` also setting `sv_port` (the launch script runs after the environment and would override it).
