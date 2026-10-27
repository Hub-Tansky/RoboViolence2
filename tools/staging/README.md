# Staging deploy agent

Production runs the same agent by hand ([runbook: Production](../../docs/ops/runbook.md#production)), so a change here changes production too.

Keeps a staging host on the newest `main` server and master images: pulls only successful `containers` runs of `main` from this repo, deploys only when no one is connected, rolls back on failure and publishes the running commit at `/deployed`. Scope: [step 2](https://github.com/Hub-Tansky/RoboViolence2/blob/58c91b2a660db2b290b0b5f504381980a6aedba2/docs/roadmap/phase-t-testing-environment/step2-staging-deploy.md). Decision: [ADR 0018](../../docs/decisions/0018-staging-pull-deploy.md). Container operation: [runbook](../../docs/ops/runbook.md).

The host's address, provider and access go in the owner's private Obsidian vault (`RoboViolence2/prv_wiki/docs_restricted/testing-environment/`), never in the repo.

## Needs

Linux (amd64 or arm64) with Docker and the compose plugin, `curl`, `jq`; inbound TCP 3333, 3334, 10207 and the admin port, and UDP 3333, 3334 (server browser ping). For the Mac agent's `staging-connect` the host needs a public DNS name (a free dynamic-DNS name works) and `BV2_ADMIN_PORT=443`, so Caddy gets a real certificate. No GitHub token: the repo, its Actions API and the images are public.

## Install

On a fresh Ubuntu 24.04 VM, paste [cloud-init.yml](cloud-init.yml) as the user data. The VM needs a public IPv4 at first boot, or the install has no network. It does the steps below except the env file and cron; once the DNS name points at the host, `sudo rv2-staging-setup <public name>` writes the env file (generated password, the host's public IP as `BV2_PUBLIC_ADDRESS`), installs the cron line and runs the first deploy. Open TCP 443, 3333, 3334, 10207 and UDP 3333, 3334 in the provider's firewall.

By hand:

As the user that runs Docker (root, or a member of the `docker` group):

```bash
sudo mkdir -p /opt/rv2-staging && sudo install -d -o "$USER" -m 700 /var/lib/rv2-staging
```

```bash
sudo curl -fsSL https://raw.githubusercontent.com/Hub-Tansky/RoboViolence2/main/tools/staging/deploy.sh -o /opt/rv2-staging/deploy.sh && sudo chmod 755 /opt/rv2-staging/deploy.sh
```

`/etc/rv2-staging.env`, mode 600, owned by that user. Compose reads it too (`--env-file`):

```
BV2_ADMIN_PASS=<random, 16+ characters>
BV2_ADMIN_HOST=<public name, or localhost>
BV2_ADMIN_PORT=<443 with a public name; default 8443>
BV2_PUBLIC_ADDRESS=<public IPv4; the master lists it, ADR 0022>
NTFY_TOPIC=<optional>
```

No join password: the client takes it as a command-line argument, which other local users could read (`ps`).

Cron, every 10 minutes (`crontab -e` for that user):

```
*/10 * * * * /opt/rv2-staging/deploy.sh >>/var/lib/rv2-staging/deploy.log 2>&1
```

The first run deploys the newest build at once (nothing is running to protect). With a public name, Caddy must get its certificate within the 60 s verify window; if not, the deploy counts as failed and `--sha` retries it.

## Use

| Command | Does |
|---|---|
| `deploy.sh` | newest successful `main` `containers` run, once `/status` of CTF and `/tdm/status` of TDM both show `players 0` (what cron runs). If a game server runs but its `/status` doesn't answer (changed password, proxy down), it doesn't deploy |
| `deploy.sh --sha <sha>` | that `main` build; also a manual rollback. Use the full SHA for builds older than the last 30 runs |
| `deploy.sh --force` | don't wait for players to leave |

Notifications (ntfy): `RV2 staging now on <sha7>` (`RV2_DEPLOY_LABEL`, default `staging`, names the host there and in the state and env file paths; production sets `prod`); `FAILED at <stage>` with the rollback result; `waiting` once when players have held deploys back for 6 hours. Commits from before `BV2_IMAGE_TAG` existed are refused. A missing or unreadable env file is logged only (no ntfy topic yet). GitHub, download and pull failures notify once per stage and don't count; 3 failed deploys of a commit stop retries until `--sha`.

Files under `/var/lib/rv2-staging/`: `deployed` (running commit), `state.tsv` (commit, result, stage, time), `releases/<sha7>/` (that commit's compose file, Caddyfile and `srv/deployed`, last 5; older releases' images are removed), `waiting`, `deploy.log`.

## Health watcher

[health-watch.sh](health-watch.sh) sends `RV2 <label> unhealthy - <service> <state>` (unhealthy, exited, restarting, missing, or Docker not answering) once a problem holds for two runs, and `RV2 <label> healthy again` once all are well for two runs. It skips while `deploy.sh` runs and writes `health-seen` and `health-notified` to the state folder. Install it beside `deploy.sh` and run it every minute:

```bash
sudo curl -fsSL https://raw.githubusercontent.com/Hub-Tansky/RoboViolence2/main/tools/staging/health-watch.sh -o /opt/rv2-staging/health-watch.sh && sudo chmod 755 /opt/rv2-staging/health-watch.sh
```

```
* * * * * /opt/rv2-staging/health-watch.sh >>/var/lib/rv2-staging/health.log 2>&1
```

## Uninstall

Remove the cron lines, then `docker compose -f /var/lib/rv2-staging/releases/<sha7>/compose.example.yml --env-file /etc/rv2-staging.env down` (add `-v` to delete the master database and certificates).
