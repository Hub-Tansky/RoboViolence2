# Step 12: Remote admin

**Status:** IN PROGRESS

**Depends on:** [Step 10](step10-server-credentials.md), [Step 11](step11-server-containers.md). **Next:** [step13-localisation.md](step13-localisation.md). **Index:** [README.md](README.md)

## Scope

| Field | Value |
|---|---|
| Goal | A server operator can run console commands on `bv2dedicated` remotely over an authenticated, encrypted, rate-limited and audited channel |
| In scope | Tasks 12.1–12.4 below |
| Out of scope | A graphical admin tool; the C.12 report review tool (PNS-13); plaintext UDP admin authorised by IP:port (never) |
| Allowed paths | `game/src/**` (admin channel and console glue), `engine/**` (only if the ADR needs it), `vcpkg.json`, `packaging/docker/**`, `docs/ops/**`, `docs/decisions/**`, `tests/**`, `ARCHITECTURE.md`, `docs/roadmap/**` Owner, 2026-10-12: `game/CMakeLists.txt` (AdminApi.cpp), `.github/workflows/containers.yml` (the TLS status check). |
| Inputs | `AGENTS.md`, `ARCHITECTURE.md`, earlier review records `reviews/step*.md`, [ADR 0002](../../decisions/0002-compile-out-libcurl.md), step 10 ADR, step 11 runbook |
| Deliverables | Admin channel in `bv2dedicated`; ADR on transport and TLS; audit log; runbook section |
| Definition of done | See "Acceptance checks". All must pass. `ARCHITECTURE.md` is updated |

## Context

- Bv2RemoteAdmin was deleted in Phase A step 1. Admin today is the in-game `admin` login (step 10) and the server console.
- OpenSSL was removed, and libcurl is compiled out (ADR 0002). TLS inside the binary adds a dependency. Ending TLS at a reverse proxy in the container stack avoids that but adds an ops dependency.

## Tasks

### 12.1 ADR: transport

- Compare: an HTTP API on localhost behind a TLS reverse proxy (Caddy) in the compose stack; RCON over TLS inside the binary (mbedtls); SSH to the container console. Recommend one, and the owner confirms.
- Owner, 2026-10-12: the HTTP API behind Caddy ([ADR 0015](../../decisions/0015-admin-http-api-behind-tls-proxy.md)).

### 12.2 Channel

- Token or HMAC authentication (reusing step 10), bound to localhost or the proxy network only. Commands map to existing console commands.

### 12.3 Limits and audit

- Rate limit per client; append-only audit log (time, source, command, result) in the pref dir. Secrets are masked as in the console.

### 12.4 Ops

- Compose example and runbook updated. Tests for auth failure, rate limiting and the audit line.

## Notes

- `game/src/AdminApi.{h,cpp}`: `POST /command` (body: one console command; the reply is the lines it added) and `GET /status`. Off unless `zsv_adminApiPort` (env `BV2_ADMIN_API_PORT`) is set; binds `zsv_adminApiBind` (`BV2_ADMIN_API_BIND`, default `127.0.0.1`). Token: `Authorization: Bearer <zsv_adminPass>`, compared in constant time.
- Limits: 5 requests per source, refilled 1 a second (429); at most 8 open connections, 1 s each; 4 KB per request (Caddy also caps the body and its read times). The source is the TCP peer, or the rightmost `X-Forwarded-For` address when the peer is `zsv_adminApiProxy` (`BV2_ADMIN_API_PROXY`, `admin-proxy` in compose).
- Refused over the API: `restart`, `disconnect`, `host`, `connect`; they rebuild the server under the request (the review reproduced a use-after-free). A command is audited before it runs.
- Audit: `admin-audit.log` in the pref dir, one line per request: UTC time, source, result (`ok`, `denied`, `limited`, `not found`, `too large`), command with a secret's value masked.
- Legacy `RA_LOGIN` (UDP, cleartext password, then trusted by IP:port) is refused (owner, 2026-10-12).
- 12.4: `packaging/docker/Caddyfile` and an `admin-proxy` service (Caddy 2.11.7) on 8443; the server's API on 8080 of the compose network only. CI calls `/status` over TLS with a wrong token (401) and the right one.
- Tests: `admin_api` (not on Windows: POSIX sockets in the test; port 10209). Fix proof: with the token check, the budget, the masking, the `X-Forwarded-For` check, the control-character cleaning or the timeout audit off, its checks fail; without the refusal, `restart` is a heap-use-after-free under ASan.
- Not done: the legacy `RA_*` code behind the refused login, and the new variables in AGENTS.md and config/README.md → PNS-46. Caddy runs as root and is pinned by tag.

## Critical files

- `game/src/Console.cpp`, `game/src/Server.cpp`, `packaging/docker/compose.example.yml`

## Acceptance checks

```bash
ctest --test-dir build/linux-x64-asan --output-on-failure -R admin
```

- Passes: a wrong token is rejected, a burst is limited, and every command leaves an audit line with secrets masked.
- With the compose stack, a remote `status` command over TLS returns the server state.
- `tools/review.sh --full <preset>` passes, and the fresh-context review record `reviews/step12.md` and the raw reviewer report `reviews/step12-report.md` (with the skill's load line) exist ([REVIEW.md](../../../REVIEW.md) section 5).
- `ARCHITECTURE.md` lists every file added, moved or removed in this step.
- The secret scan (`gitleaks git --staged` / CI) is clean.
