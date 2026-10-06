# Step 10: Server credentials and identity

**Status:** TODO

**Depends on:** [Step 4](step4-packet-hygiene.md). **Next:** [step11-server-containers.md](step11-server-containers.md). **Index:** [README.md](README.md)

## Scope

| Field | Value |
|---|---|
| Goal | Server and admin passwords never cross the wire in cleartext or unsalted MD5, and no identity or ban depends on a client-reported MAC address |
| In scope | Tasks 10.1–10.4 below |
| Out of scope | Account login, session tokens and KDF (PNS-14, blocked on PNS-15); the remote admin channel (step 12); new password prompts or fields (**[GUI]**) |
| Allowed paths | `game/src/ServerRecv.cpp`, `game/src/ClientRecv.cpp`, `game/src/Client*.{h,cpp}`, `game/src/Server.{h,cpp}`, `game/src/Console.cpp`, `game/src/netPacket.h`, `game/src/CMaster.cpp`, `engine/babonet/src/MD5.h`, `masterserver/src/**`, `content-seed/**`, `tests/**`, `docs/analysis/KEY_QUESTIONS.md`, `docs/decisions/**`, `ARCHITECTURE.md`, `docs/roadmap/**` |
| Inputs | `AGENTS.md`, `ARCHITECTURE.md`, `docs/analysis/KEY_QUESTIONS.md` Q-S3, Q-S5, `config/README.md` |
| Deliverables | Challenge-response handshake; protocol bump; MAC removed; ADR for the HMAC choice |
| Definition of done | See "Acceptance checks". All must pass. `ARCHITECTURE.md` is updated |

## Context

- Q-S5: `sv_password` arrives in cleartext in `GAMEVERSION_ACCEPTED` (`game/src/ServerRecv.cpp:273`). Admin login compares MD5 of user and password (`ServerRecv.cpp:193–214`, `Console.cpp:754–757`).
- `PLAYER_INFO` carries `macAddr[20]` (`game/src/netPacket.h:428`), used for bans and the player cache (`ServerRecv.cpp:466–491`) and forwarded to the master (Q-S3).
- OpenSSL was removed in Phase A. An HMAC needs a small vetted implementation or a vcpkg port; decide in an ADR.

## Tasks

### 10.1 HMAC primitive (ADR)

- Choose the HMAC-SHA256 source (a vcpkg port such as `mbedtls` or `libsodium`, or a vendored-free alternative). Record it in an ADR.

### 10.2 Server password challenge-response

- The server sends a random nonce in its version reply; the client answers `HMAC(password, nonce)`. Bump `GAME_VERSION_SV/CL` and update `tests/test_netpacket.cpp`. The existing password prompt is reused.

### 10.3 Admin login challenge-response

- Same scheme for `admin` login. The console shows the same messages as before.

### 10.4 Remove MAC identity

- Drop `macAddr` from `PLAYER_INFO` (leave the message ID; don't renumber), from bans, the player cache and master reports. Bans stay IP-based until accounts exist (PNS-14).

## Critical files

- `game/src/ServerRecv.cpp`, `game/src/ClientRecv.cpp`, `game/src/netPacket.h`, `game/src/Console.cpp`

## Acceptance checks

```bash
ctest --test-dir build/linux-x64-asan --output-on-failure
```

- Passes, including a handshake test that captures the traffic and finds no password or MD5 of it.
- `grep -rn macAddr game/src masterserver/src` finds nothing.
- The owner joins a password-protected server and logs in as admin on one OS.
- `/thermo-nuclear-code-quality-review` has run on the step diff.
- Q-S5 is marked fixed in `KEY_QUESTIONS.md`.
- `ARCHITECTURE.md` lists every file added, moved or removed in this step.
- The secret scan (`gitleaks git --staged` / CI) is clean.
