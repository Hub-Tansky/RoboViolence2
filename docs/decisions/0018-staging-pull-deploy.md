# 0018: Staging deploys itself by pull, pinned to a commit, only when empty

- Status: Accepted
- Date: 2026-10-14
- Affects: [phase T step 2](https://github.com/Hub-Tansky/RoboViolence2/blob/58c91b2a660db2b290b0b5f504381980a6aedba2/docs/roadmap/phase-t-testing-environment/step2-staging-deploy.md)

## Context
- The Mac agent's `staging-connect` ([ADR 0017](0017-playtest-pull-agent.md)) needs a staging server on the same build, and must know which build it runs: `/status` doesn't say (`game/src/AdminApi.cpp:110`).
- `containers.yml` pushed amd64 only; the host is not chosen yet (owner, 2026-10-14) and may be arm64.
- `compose.example.yml` pinned `:latest`, so images and compose file could come from different commits.

## Decision
A cron job on the staging host (`tools/staging/deploy.sh`) selects the newest successful `containers` push run on `main` of this repo, fetches that commit's compose file and Caddyfile, pins both images to `:<sha>` (`BV2_IMAGE_TAG`), deploys only when the server is not running or `/status` shows no players, and rolls back to the previous commit on failure. Caddy serves the running commit at `GET /deployed` from a file in the release folder. CI publishes multi-arch `:<sha>` and `:latest`.

## Consequences
- No inbound deploy path and no secret in GitHub; no GitHub token on the host (public API and images).
- A game in progress is never interrupted, also when `/status` can't be read; a busy server can delay a deploy indefinitely (notified after 6 h; `--force`).
- `/deployed` is public: it holds only the commit and time.
- Commits before this decision have a compose file without `BV2_IMAGE_TAG`; the agent refuses them.
- The weekly rebuild re-tags the running commit; staging picks it up with the next commit, not by itself.
- Rejected: Watchtower or `:latest` auto-pull (restarts mid-game, unknown commit); GitHub Actions deploying over SSH (key in GitHub, open SSH port, push-based); a build line in `/status` (game code change for an ops need).

## Amendment 2026-10-22: production runs the same agent by hand
- Production (owner decision 2026-10-20: pinned to a release, deployed by hand) runs `tools/staging/deploy.sh` with `RV2_DEPLOY_LABEL=prod` and `--sha <full sha>`, never from cron. The label gives it its own state folder, env file and notification name.
- A busy server makes a run exit without deploying; nothing retries. `/deployed` is the success signal ([runbook: Production](../ops/runbook.md#production)).
- Failover is a new host from the same steps, the latest off-host backup restored, a new `BV2_PUBLIC_ADDRESS` and a DNS change.
- Rejected: cron on production (deploys every `main` commit, not a release); a separate production tool (two agents to keep in step); a floating IP for failover (provider-specific, costs more).
