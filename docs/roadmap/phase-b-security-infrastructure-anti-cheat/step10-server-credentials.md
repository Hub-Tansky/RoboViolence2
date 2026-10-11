# Step 10: Server credentials and identity

**Status:** IN PROGRESS

**Depends on:** [Step 4](step4-packet-hygiene.md). **Next:** [step11-server-containers.md](step11-server-containers.md). **Index:** [README.md](README.md)

## Scope

| Field | Value |
|---|---|
| Goal | Server and admin passwords never cross the wire in cleartext or unsalted MD5, and no identity or ban depends on a client-reported MAC address |
| In scope | Tasks 10.1–10.4 below |
| Out of scope | Account login, session tokens and KDF (PNS-14, blocked on PNS-15); the remote admin channel (step 12); new password prompts or fields (**[GUI]**) |
| Allowed paths | `game/src/ServerRecv.cpp`, `game/src/ClientRecv.cpp`, `game/src/Client*.{h,cpp}`, `game/src/Server.{h,cpp}`, `game/src/Console.cpp`, `game/src/netPacket.h`, `game/src/CMaster.cpp`, `engine/babonet/src/MD5.h`, `masterserver/src/**`, `content-seed/**`, `tests/**`, `docs/analysis/KEY_QUESTIONS.md`, `docs/decisions/**`, `ARCHITECTURE.md`, `docs/roadmap/**`. Owner, 2026-10-12: `vcpkg.json`, `CMakeLists.txt`, `game/CMakeLists.txt` (libsodium), `game/src/Credentials.{h,cpp}` (new), `engine/babonet/include/cMSstruct.h`, `game/src/PacketCheck.cpp`, `game/src/Game.cpp` (GAMEVERSION send moves to `Server`), `game/src/GameVar.cpp` (`sv_password` no longer pushed) |
| Inputs | `AGENTS.md`, `ARCHITECTURE.md`, earlier review records `reviews/step*.md`, `docs/analysis/KEY_QUESTIONS.md` Q-S3, Q-S5, `config/README.md` |
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

## Notes

- Scheme: `NET_SVCL_JOIN_CHALLENGE` (134, sent just before `GAMEVERSION`) carries a 16-byte nonce per connection; the client answers `HMAC-SHA256(password, label + '\0' + nonce)` with label `join`, `admin user` or `admin password` (`game/src/Credentials.h`). The admin login reuses the join nonce. `GAMEVERSION` stays 4 bytes, so a mismatched build still reports "Wrong game version". Version 22100.
- Found and fixed: `set sv_password` was broadcast in cleartext to every client (`Server::sendSVChange`); the master heartbeat and the LAN browser reply carried the password (now `*` when set: they only need the flag); `PLAYER_INFO` relayed each player's account username and MD5 password hash to all players. The join push (`GameVar::sendSVVar`) sent `***` (masked by `dksvarGetFormatedVar`); secrets now stay off the wire in `sendOne` and `sendSVChange` (`dksvarIsSecret`).
- MAC: gone from `PLAYER_INFO`, the server's player cache (now matched by IP; the old code overwrote the wrong entry on a match), the master's ban and cache messages and `BanList` (the seed; an existing `master.db` keeps an unused `MAC` column). `bb_getMyMAC` stays in babonet, unused.
- Tests: `credentials` (RFC 4231), `join_password`, `admin_login`; fix proofs: with the server's check skipped, the wrong-password and replay cases fail (`test_join_password.cpp:71,75`, `test_admin_login.cpp:60`); without the blanking, the relay check fails (`test_join_password.cpp:82`); with the push restored, the capture check fails.
- Not tested by the harness (it runs the server only): the client side (`Client::sendVersionAccepted`, the `admin` command) and the master heartbeat and LAN reply; the owner's join and admin test covers the client.
- Master protocol: `stCacheBan`, `stCacheBanned` and `stCachePlayer` lose 20 bytes and the master has no protocol version; deploy master and game servers together. An existing `master.db` keeps an unused `MAC` column; bans that matched only by MAC stop applying.
- Open: a sniffed proof allows offline guessing of a weak password, and admin attempts are not limited → PNS-42; pre-existing gaps → PNS-43.

## Critical files

- `game/src/ServerRecv.cpp`, `game/src/ClientRecv.cpp`, `game/src/netPacket.h`, `game/src/Console.cpp`

## Acceptance checks

```bash
ctest --test-dir build/linux-x64-asan --output-on-failure
```

- Passes, including a handshake test that captures the traffic and finds no password or MD5 of it.
- `grep -rn macAddr game/src masterserver/src` finds nothing.
- The corpus is regenerated for the new `GAMEVERSION_ACCEPTED` and `PLAYER_INFO` layouts (`tests/gen_corpus.cpp`), and the replay test and CI `fuzz` job pass on it.
- The owner joins a password-protected server and logs in as admin on one OS.
- Q-S5 is marked fixed in `KEY_QUESTIONS.md`.
- `tools/review.sh --full <preset>` passes, and the fresh-context review record `reviews/step10.md` and the raw reviewer report `reviews/step10-report.md` (with the skill's load line) exist ([REVIEW.md](../../../REVIEW.md) section 5).
- `ARCHITECTURE.md` lists every file added, moved or removed in this step.
- The secret scan (`gitleaks git --staged` / CI) is clean.
