# Step 12: Remote admin

**Status:** TODO

**Depends on:** [Step 10](step10-server-credentials.md), [Step 11](step11-server-containers.md). **Next:** [step13-localisation.md](step13-localisation.md). **Index:** [README.md](README.md)

## Scope

| Field | Value |
|---|---|
| Goal | A server operator can run console commands on `bv2dedicated` remotely over an authenticated, encrypted, rate-limited and audited channel |
| In scope | Tasks 12.1–12.4 below |
| Out of scope | A graphical admin tool; the C.12 report review tool (PNS-13); plaintext UDP admin authorised by IP:port (never) |
| Allowed paths | `game/src/**` (admin channel and console glue), `engine/**` (only if the ADR needs it), `vcpkg.json`, `packaging/docker/**`, `docs/ops/**`, `docs/decisions/**`, `tests/**`, `ARCHITECTURE.md`, `docs/roadmap/**` |
| Inputs | `AGENTS.md`, `ARCHITECTURE.md`, [ADR 0002](../../decisions/0002-compile-out-libcurl.md), step 10 ADR, step 11 runbook |
| Deliverables | Admin channel in `bv2dedicated`; ADR on transport and TLS; audit log; runbook section |
| Definition of done | See "Acceptance checks". All must pass. `ARCHITECTURE.md` is updated |

## Context

- Bv2RemoteAdmin was deleted in Phase A step 1. Admin today is the in-game `admin` login (step 10) and the server console.
- OpenSSL was removed, and libcurl is compiled out (ADR 0002). TLS inside the binary adds a dependency. Ending TLS at a reverse proxy in the container stack avoids that but adds an ops dependency.

## Tasks

### 12.1 ADR: transport

- Compare: an HTTP API on localhost behind a TLS reverse proxy (Caddy) in the compose stack; RCON over TLS inside the binary (mbedtls); SSH to the container console. Recommend one, and the owner confirms.

### 12.2 Channel

- Token or HMAC authentication (reusing step 10), bound to localhost or the proxy network only. Commands map to existing console commands.

### 12.3 Limits and audit

- Rate limit per client; append-only audit log (time, source, command, result) in the pref dir. Secrets are masked as in the console.

### 12.4 Ops

- Compose example and runbook updated. Tests for auth failure, rate limiting and the audit line.

## Critical files

- `game/src/Console.cpp`, `game/src/Server.cpp`, `packaging/docker/compose.example.yml`

## Acceptance checks

```bash
ctest --test-dir build/linux-x64-asan --output-on-failure -R admin
```

- Passes: a wrong token is rejected, a burst is limited, and every command leaves an audit line with secrets masked.
- With the compose stack, a remote `status` command over TLS returns the server state.
- `tools/review.sh --full <preset>` passes, and the fresh-context review record `reviews/step12.md` exists ([REVIEW.md](../../../REVIEW.md) section 5).
- `ARCHITECTURE.md` lists every file added, moved or removed in this step.
- The secret scan (`gitleaks git --staged` / CI) is clean.
