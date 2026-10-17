# Step 6: Server and master

**Status:** IN PROGRESS

**Depends on:** Phase B DONE. **Next:** step 7 (R28). **Index:** [README.md](README.md)

## Scope

| Field | Value |
|---|---|
| Goal | Admin logins can't be brute-forced per connection or use a short password, master bans need a configured secret, the master lists co-located servers at their public address, and the dead UDP remote admin is gone |
| In scope | Tasks 6.1–6.4 below (PNS-42, PNS-43 master ban password, PNS-44, PNS-46); defaults owner-approved 2026-10-17 |
| Out of scope | Join order and relayed IPs (PNS-43, Phase D); a PAKE or TLS for in-game logins; the admin HTTP API's own limits (step 12) |
| Allowed paths | `game/src/ServerRecv.cpp`, `game/src/Server.{h,cpp}`, `game/src/CMaster.{h,cpp}`, `game/src/Console.cpp`, `game/src/SceneNet.cpp`, `game/src/GameVar.{h,cpp}`, `game/src/Paths.cpp`, `game/src/RemoteAdminPackets.h` (removed), `game/CMakeLists.txt`, `masterserver/src/**`, `masterserver/README.md`, `config/**`, `packaging/docker/**`, `docs/ops/**`, `tests/**`, `docs/analysis/**`, `docs/decisions/**`, `AGENTS.md`, `ARCHITECTURE.md`, `docs/roadmap/**` |
| Inputs | `AGENTS.md`, `ARCHITECTURE.md`, PNS-42, PNS-43, PNS-44, PNS-46, [ADR 0014](../../decisions/0014-libsodium-hmac-for-credentials.md), [ADR 0015](../../decisions/0015-admin-http-api-behind-tls-proxy.md) |
| Deliverables | Attempt budget and length rule; master ban secret; advertised address; remote admin removed; tests; ADR 0022 |
| Definition of done | See "Acceptance checks". All must pass. `ARCHITECTURE.md` is updated |

## Context

- `NET_CLSV_ADMIN_REQUEST` (`game/src/ServerRecv.cpp:238`) is accepted without limit; one captured proof allows offline guessing of a short `zsv_adminPass` (PNS-42).
- The master authorises `CACHE_BAN`/`CACHE_UNBAN` with the literal `roxbabo` (`masterserver/src/cNetManager.cpp:224,239`).
- The master files a server under the heartbeat's source address (`cMasterServer::UpdateGame`, `c->IP`); behind Docker that is a private address remote players can't reach (PNS-44).
- `RA_LOGIN` is refused since Phase B step 12, but the `RA_*` handling and broadcasts remain (PNS-46).

## Tasks

### 6.1 Admin login budget and length (PNS-42)

- A `zsv_adminPass` shorter than 12 characters disables in-game admin login (logged). The 5th failed attempt on a connection disconnects it.

### 6.2 Master ban secret (PNS-43)

- The master reads `BV2_MASTER_BAN_PASS`; ban and unban messages need it and are refused when it is unset.

### 6.3 Advertised address (PNS-44)

- `sv_publicAddress` (env `BV2_PUBLIC_ADDRESS`) goes into the heartbeat's `ip`. The master uses it only when the heartbeat comes from a private or loopback address; otherwise the source address. ADR 0022.

### 6.4 Remove the UDP remote admin (PNS-46)

- Delete the `RA_*` messages, handlers and broadcasts; keep the peers used for the LAN browser and ping. List `BV2_ADMIN_API_*` in `AGENTS.md` and `config/README.md`.

## Critical files

- `game/src/ServerRecv.cpp`, `masterserver/src/cNetManager.cpp`, `masterserver/src/cMasterServer.cpp`, `game/src/CMaster.cpp`

## Acceptance checks

- `ctest --test-dir build/<preset> --output-on-failure` passes, including `admin_login` (short password, budget), `master` (ban secret, advertised address).
- `grep -rn "RA_\|roxbabo" game/src masterserver/src` finds nothing.
- `tools/review.sh --full <preset>` passes, and the fresh-context review record `reviews/step6.md` and the raw reviewer report `reviews/step6-report.md` (with the skill's load line) exist (`REVIEW.md` section 5).
- `ARCHITECTURE.md` lists every file added, moved or removed in this step.
- The secret scan (`gitleaks git --staged` / CI) is clean.
