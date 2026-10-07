# Step 11: Server container images

**Status:** TODO

**Depends on:** [Step 1](step1-playtest-builds-and-build-guides.md). **Next:** [step12-remote-admin.md](step12-remote-admin.md). **Index:** [README.md](README.md)

## Scope

| Field | Value |
|---|---|
| Goal | CI builds Linux container images for `bv2dedicated` and `bv2master` that run with secrets injected from the environment, and a runbook explains how to host them |
| In scope | Tasks 11.1–11.4 below |
| Out of scope | Choosing or provisioning a host; real hostnames or IPs (never committed); the admin channel (step 12); account services (PNS-15) |
| Allowed paths | `packaging/docker/**`, `.github/workflows/**`, `docs/ops/**`, `.dockerignore`, `ARCHITECTURE.md`, `docs/roadmap/**` |
| Inputs | `AGENTS.md`, `ARCHITECTURE.md`, earlier review records `reviews/step*.md`, `config/README.md`, [ADR 0007](../../decisions/0007-data-root-pref-dir-config-layers.md), step 1 Linux package |
| Deliverables | Two Dockerfiles; CI image build (and push to GHCR on `main`); `docs/ops/runbook.md` |
| Definition of done | See "Acceptance checks". All must pass. `ARCHITECTURE.md` is updated |

## Context

- The servers read secrets from `BV2_SV_PASSWORD`, `BV2_ADMIN_PASS` and `BV2_MASTER_SERVERS`, and never save them (ADR 0007).
- The headless server still needs GLU at runtime (PNS-3); the image must include it until PNS-12 removes it.
- Map files and `bv2.db` live in the pref dir (`BV2_PREF_DIR`), which becomes a volume.

## Tasks

### 11.1 Dockerfiles

- Multi-stage: build with the `linux-x64` preset, run on a slim Ubuntu 24.04 base as a non-root user. One image per server, volumes for the pref dir and for `BV2_DATA_DIR`.

### 11.2 CI

- Build both images on PRs; push to `ghcr.io/hub-tansky/...` on `main` (`packages: write`).

### 11.3 Compose example

- `packaging/docker/compose.example.yml` with placeholder env values and no real hosts.

### 11.4 Runbook

- `docs/ops/runbook.md`: start, stop, upgrade, logs, backups of the pref dir, secret injection, ports to open.

## Critical files

- `CMakePresets.json`, `tools/setup-dev.sh --linux-packages` (package list)

## Acceptance checks

```bash
docker compose -f packaging/docker/compose.example.yml up -d && docker compose ps
```

- Both services are healthy. A local client connects to the containerised server, which appears in the containerised master's list.
- `docker run --rm <image> id -u` is not 0.
- gitleaks and `.gitleaks.toml` host rules find nothing in `packaging/docker/` or `docs/ops/`.
- `tools/review.sh --full <preset>` passes, and the fresh-context review record `reviews/step11.md` exists ([REVIEW.md](../../../REVIEW.md) section 5).
- `ARCHITECTURE.md` lists every file added, moved or removed in this step.
- The secret scan (`gitleaks git --staged` / CI) is clean.
