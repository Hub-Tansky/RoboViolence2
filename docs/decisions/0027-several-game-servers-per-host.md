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
- The staging deploy agent waits until both servers report 0 players and verifies both; it skips TDM where a release has no `tdm` service.

## Consequences
- A further server is a copied service, a launch script, a port, a volume, `BV2_HEALTH_PORT` and one Caddy route.
- Old CTF admin URLs keep working.
- Rejected: a second published admin port per server (one more firewall opening and certificate host per server); a healthcheck that accepts any listening socket (the admin API's socket would pass it while the game port is down); `BV2_HEALTH_PORT` also setting `sv_port` (the launch script runs after the environment and would override it).
