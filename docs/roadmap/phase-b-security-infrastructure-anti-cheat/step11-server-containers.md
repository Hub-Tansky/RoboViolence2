# Step 11: Server container images

**Status:** IN PROGRESS

**Depends on:** [Step 1](step1-playtest-builds-and-build-guides.md). **Next:** [step12-remote-admin.md](step12-remote-admin.md). **Index:** [README.md](README.md)

## Scope

| Field | Value |
|---|---|
| Goal | CI builds Linux container images for `bv2dedicated` and `bv2master` that run with secrets injected from the environment, and a runbook explains how to host them |
| In scope | Tasks 11.1–11.4 below |
| Out of scope | Choosing or provisioning a host; real hostnames or IPs (never committed); the admin channel (step 12); account services (PNS-15) |
| Allowed paths | `packaging/docker/**`, `.github/workflows/**`, `docs/ops/**`, `.dockerignore`, `ARCHITECTURE.md`, `docs/roadmap/**`. Owner, 2026-10-12: `content/LaunchScript/CTF.cfg` (drop its empty secret `set` lines), `.github/dependabot.yml` (docker), `masterserver/src/cMSstruct.h` and `engine/babonet/include/cMSstruct.h` (packing fix) |
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

## Notes

- One Dockerfile with two targets (`server`, `master`) instead of two files: the C++ build runs once for both images. It builds the `linux-x64-release` preset (the packages' Release build) and declares no `BV2_DATA_DIR` volume: the runbook mounts one when you have the original data. Images are linux/amd64 (`x64-linux`).
- Verification (owner, 2026-10-12, no Docker on the Mac): CI builds both images, checks `id -u`, runs the compose example to healthy and waits for the server in the master's `web.db`; the owner runs the compose example on a Linux machine and joins from a client.
- The server keeps reading stdin: `stdin_open` and `tty` let `docker attach` reach its console and detach. Without stdin (`docker run` without `-i`) its console loop spins with a 1 ms sleep, so run it with `-i`.
- Healthchecks look for the listening socket in `/proc/net/tcp`: a TCP connect would join the game as a player.
- Found: the master's copy of the protocol structs (`masterserver/src/cMSstruct.h`) was not packed (and had a `long`), unlike the game's since step 4: the master read every server's port one byte off (3333 became 13), so its lists were unusable. Both copies now pack to 1 byte and assert the same sizes and `stBV2row::port` at offset 113; CI's listing check matches the port.
- Found: `CTF.cfg` set `sv_password`, `zsv_adminUser` and `zsv_adminPass` to `""` after the environment layer, wiping `BV2_SV_PASSWORD` and `BV2_ADMIN_PASS` (packages too). CI now sets a password and checks the master lists the game as passworded.
- The master lists a server at its connection's address (in compose, the container's): PNS-44.

## Critical files

- `CMakePresets.json`, `tools/setup-dev.sh --linux-packages` (package list)

## Acceptance checks

```bash
docker compose -f packaging/docker/compose.example.yml up -d && docker compose ps
```

- Both services are healthy. A local client connects to the containerised server, which appears in the containerised master's list.
- `docker run --rm --entrypoint id <image> -u` is not 0.
- gitleaks and `.gitleaks.toml` host rules find nothing in `packaging/docker/` or `docs/ops/`.
- `tools/review.sh --full <preset>` passes, and the fresh-context review record `reviews/step11.md` and the raw reviewer report `reviews/step11-report.md` (with the skill's load line) exist ([REVIEW.md](../../../REVIEW.md) section 5).
- `ARCHITECTURE.md` lists every file added, moved or removed in this step.
- The secret scan (`gitleaks git --staged` / CI) is clean.
