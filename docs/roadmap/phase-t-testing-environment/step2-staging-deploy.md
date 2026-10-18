# Step 2: Staging host and pull deploy agent

**Status:** DONE (2026-10-18)

**Depends on:** step 1 (Mac agent's `staging-connect`), Phase B step 11 (containers), step 12 (admin API). **Next:** T2–T7 (candidates). **Index:** [README.md](README.md)

## Scope

| Field | Value |
|---|---|
| Goal | A long-lived staging server runs the newest `main` server and master images, updated by a pull agent on its own host, never during a game, and reports which commit it runs |
| In scope | Tasks 2.1 … 2.6 below; plan item T8 ([plan](../20261014_testing_environment_plan.md#t8-staging-host-and-pull-deploy-agent)) |
| Out of scope | Choosing the host and its account (owner; details go in the private wiki); a public DNS name and certificate; PR builds on staging (never) |
| Allowed paths | `tools/staging/**`, `tools/playtest/**`, `tests/test_staging_deploy.sh`, `tests/CMakeLists.txt`, `packaging/docker/compose.example.yml`, `packaging/docker/Caddyfile`, `.github/workflows/containers.yml`, `docs/ops/runbook.md`, `docs/roadmap/**`, `docs/decisions/**`, `ARCHITECTURE.md` |
| Inputs | `AGENTS.md`, `ARCHITECTURE.md`, the plan, [step 1 review](reviews/step1.md), `docs/ops/runbook.md`, `.github/workflows/containers.yml`, [ADR 0015](../../decisions/0015-admin-http-api-behind-tls-proxy.md), [ADR 0017](../../decisions/0017-playtest-pull-agent.md) |
| Deliverables | `tools/staging/deploy.sh`, `tools/staging/README.md`, `tests/test_staging_deploy.sh`, multi-arch images, `BV2_IMAGE_TAG`, `/deployed`, ADR 0018 |
| Definition of done | See "Acceptance checks". All must pass. `ARCHITECTURE.md` is updated |

## Context

- `containers.yml` pushes `:latest` and `:<sha>` for amd64 only; arm64 is built as a check. The owner chose (2026-10-14) to decide the host later, so both architectures are published.
- `compose.example.yml` pins `:latest`, so a deploy can't name a commit.
- `/status` reports players but not the build (`game/src/AdminApi.cpp:110`).
- The repo, its Actions API and its GHCR images are public: the host needs no GitHub token.

## Tasks

### 2.1 Scope

- This file; row in the phase README.

### 2.2 Images and compose

- `containers.yml`: push both architectures (`:<sha>-amd64`, `:<sha>-arm64`), then one multi-arch `:<sha>` and `:latest`.
- `compose.example.yml`: `image: …:${BV2_IMAGE_TAG:-latest}`.
- `Caddyfile`: `GET /deployed` serves `/srv/deployed` (commit and time, no auth); everything else goes to the admin API as before. CI checks the route.

### 2.3 Deploy agent

- `tools/staging/deploy.sh` (bash, `curl`, `jq`, `docker compose`): select the newest successful `containers` push run on `main` of this repo; fetch that commit's `compose.example.yml` and `Caddyfile`; pull both images at `:<sha>`; wait until `/status` shows `players 0` (notify once after 6 h; `--force` skips); `up --wait`; verify `/status`; write and publish `deployed`; on failure roll back to the previous commit. ntfy for every outcome; infrastructure failures (GitHub, raw files, pull) notify once and don't use up the 3 attempts.
- `--sha <sha>` deploys that `main` build (also a manual rollback).

### 2.4 Tests

- `tests/test_staging_deploy.sh` (ctest `staging_deploy`): run selection, player count parsing, attempt rule, env-file mode.

### 2.5 Docs, ADR, inventory

- `tools/staging/README.md` (host setup, env file, cron, rollback); runbook: `BV2_IMAGE_TAG` and `/deployed`.
- Mac agent: `RV2_STAGING_ADMIN_PORT` (default 8443), for a host that publishes 443 with a public certificate.

### 2.6 Host bootstrap

Added 2026-10-18 at the owner's request, after the host was chosen (commit 8ce1ff0, merged as PR #70 before its review round).

- `tools/staging/cloud-init.yml`: Docker, `deploy.sh`, and `rv2-staging-setup <public name>` (env file with a generated password and `BV2_PUBLIC_ADDRESS`, cron, first deploy).
- ADR 0018; `ARCHITECTURE.md`.

## Critical files

- `tools/staging/deploy.sh`, `.github/workflows/containers.yml`, `packaging/docker/compose.example.yml`, `packaging/docker/Caddyfile`

## Acceptance checks

- `ctest --test-dir build/macos-arm64 -R staging_deploy --output-on-failure` passes.
- CI `containers` passes on the PR, including the `/deployed` check.
- On the owner's Mac (Docker Desktop, images of a `main` commit): `deploy.sh --sha <sha>` brings the stack up healthy, `/deployed` shows the commit, a second run does nothing; with a client connected, a newer commit waits; a bad tag fails at `pull` without touching the running stack; a failed `up` rolls back.
- After this PR merges, the first `containers` run on `main` publishes multi-arch `:<sha>` (owner check: `docker buildx imagetools inspect`).
- With a host chosen: a new `main` build runs on staging within 30 minutes of its `containers` run when no one is connected, and the Mac agent's `staging-connect` passes (owner check).
- `tools/review.sh --full macos-arm64` passes; `reviews/step2.md` and `reviews/step2-report.md` exist.
- `ARCHITECTURE.md` lists every file added in this step.
