# Staging deploy agent

Keeps a staging host on the newest `main` server and master images: pulls only successful `containers` runs of `main` from this repo, deploys only when no one is connected, rolls back on failure and publishes the running commit at `/deployed`. Scope: [step 2](../../docs/roadmap/phase-t-testing-environment/step2-staging-deploy.md). Decision: [ADR 0018](../../docs/decisions/0018-staging-pull-deploy.md). Container operation: [runbook](../../docs/ops/runbook.md).

The host's address, provider and access go in the private wiki (`docs/prv_wiki/docs_restricted/testing-environment/`), never in the repo.

## Needs

Linux (amd64 or arm64) with Docker and the compose plugin, `curl`, `jq`; inbound TCP 3333, 10207 and the admin port (8443, or 443 and 80 with a public certificate). No GitHub token: the repo, its Actions API and the images are public.

## Install

As the user that runs Docker (root, or a member of the `docker` group):

```bash
sudo mkdir -p /opt/rv2-staging /var/lib/rv2-staging
```

```bash
sudo curl -fsSL https://raw.githubusercontent.com/Hub-Tansky/RoboViolence2/main/tools/staging/deploy.sh -o /opt/rv2-staging/deploy.sh && sudo chmod 755 /opt/rv2-staging/deploy.sh
```

`/etc/rv2-staging.env`, mode 600, owned by that user. Compose reads it too (`--env-file`):

```
BV2_ADMIN_PASS=<random, 16+ characters>
BV2_ADMIN_HOST=<public name, or localhost>
NTFY_TOPIC=<optional>
RV2_STAGING_ADMIN_PORT=<optional, default 8443>
```

No join password: the client takes it as a command-line argument, which other local users could read (`ps`).

Cron, every 10 minutes (`crontab -e` for that user):

```
*/10 * * * * /opt/rv2-staging/deploy.sh >>/var/lib/rv2-staging/deploy.log 2>&1
```

The first run deploys the newest build at once (nothing is running to protect).

## Use

| Command | Does |
|---|---|
| `deploy.sh` | newest successful `main` `containers` run, once `/status` shows `players 0` (what cron runs) |
| `deploy.sh --sha <sha>` | that `main` build; also a manual rollback |
| `deploy.sh --force` | don't wait for players to leave |

Notifications (ntfy): `RV2 staging now on <sha7>`; `FAILED at <stage>` with the rollback result; `waiting` once after 6 hours of connected players. GitHub, download and pull failures notify once per stage and don't count; 3 failed deploys of a commit stop retries until `--sha`.

Files under `/var/lib/rv2-staging/`: `deployed` (running commit), `state.tsv` (commit, result, stage, time), `releases/<sha7>/` (that commit's compose file and Caddyfile, last 5), `waiting`, `deploy.log`.

## Uninstall

Remove the cron line, then `docker compose -f /var/lib/rv2-staging/releases/<sha7>/compose.example.yml --env-file /etc/rv2-staging.env down` (add `-v` to delete the master database and certificates).
