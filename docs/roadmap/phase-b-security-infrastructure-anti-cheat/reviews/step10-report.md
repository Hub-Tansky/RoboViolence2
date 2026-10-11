SKILL: Launching skill: thermo-nuclear-code-quality-review

Review of `git diff 0ff5b65...7a48dba` (Phase B step 10, 6 commits). Built on macos-arm64 at 7a48dba (ninja up to date) and ran ctest: 16/16 pass. `tools/check-architecture.sh` passes. Nothing was edited.

## Findings

**1. Critical. `sv_password` still goes out in cleartext, to the master server and to every LAN browser.**
- `game/src/CMaster.cpp:896` copies the password into `bv2Row.password` for the master heartbeat.
- The master then sends the whole `stBV2row` to every client that lists games (`masterserver/src/cMasterServer.cpp:331`, `BV2_ROW`).
- `game/src/CMaster.cpp:362` puts it in the LAN broadcast reply, which goes to anyone who sends the hard-coded key `3333113333`.
- The only consumers need a flag, not the value: `CBrowser.cpp:169` and `cMasterServer.cpp:134` both test `password[0] != 0`.
- So the step goal ("server passwords never cross the wire in cleartext") is not met, and Q-S5's "fixed" claim in KEY_QUESTIONS is wrong. The tests miss it because the harness only captures game-connection traffic.
- Fix (CMaster.cpp is in Allowed paths): fill both fields with a marker such as `"*"` when `sv_password != ""`, else `""`. Add a check that the `BV2_ROW` sent to the master and the broadcast reply never contain the password. Correct the Q-S5 line.

**2. Medium. Growing `GAMEVERSION` breaks the "wrong version" message between old and new builds.**
- The client's pre-dispatch check needs an exact size (`PacketCheck.cpp:186`; rule at `:94`, now 20 bytes).
- A 22100 client joining a 22000 server gets a 4-byte `GAMEVERSION`. It drops it ("Dropped server message 116 (size 4)") and hangs until timeout instead of saying "Wrong game version".
- An older step-4-era client joining a new server fails the same way. This defeats the one message whose job is to report a version mismatch, which matters for the friends-on-mixed-builds playtests.
- Cleaner fix: keep `net_svcl_gameversion` at 4 bytes for good and send the nonce in a new message (e.g. `NET_SVCL_JOIN_CHALLENGE`, sent just before `GAMEVERSION`).
- Minimal fix: a "first 4 bytes are the version" rule for `GAMEVERSION` (accept `size >= 4`), reading the nonce only when the size is 20.

**3. Medium (design). The `sv_password` push guard is a one-off `if` at the wrong layer.**
- `Console.cpp:856` adds `if (svar != "sv_password")` before `sendSVChange`; `GameVar.cpp` removes `sendOne("sv_password")` by hand in two lists.
- The codebase already has the canonical predicate `dksvarIsSecret(name)` (`engine/zeven/include/dksvar.h:84`: "callers must not echo their values").
- Put one `if (dksvarIsSecret(name)) return;` in `Server::sendSVChange` (`Server.cpp:1397`, parse the name) and in `GameVar::sendOne`. That deletes the Console special case and also covers `zsv_adminPass` and any future secret. The varCom passed in is the full command line, so parse the variable name once there.
- Related, pre-existing: `Console.cpp:847` echoes the truncated password into the console log, against that same contract.

**4. Medium. The tests prove the server side only; the client side and the "nothing on the wire" claim are partly circular.**
- `test_join_password.cpp` and `test_admin_login.cpp` build the client packets themselves with `credentials::prove`. Scanning `delivered` for the password therefore checks only the test's own code.
- `Client::sendVersionAccepted` (`ClientRecv.cpp:58-64`) is never run, and neither is the admin command (`Console.cpp:763-764`), the deferred GAMEVERSION-before-NEWPLAYER path (`ClientRecv.cpp:286/296`) or the map-download re-query (`:145`).
- A regression that put `m_password` into `proof`, or hashed with the wrong nonce, would pass every test. Only the server-to-client half of the capture check, which catches the SV push, is meaningful.
- Fix: add a test that drives the real client path, even just `Client::sendVersionAccepted` with a fake `bb_clientSend` capture. Or state the limit in the step notes.

**5. Low. libsodium is not initialised on the client.**
- `sodium_init()` is only called inside `credentials::nonce()` (`Credentials.cpp:21`), which only the server calls.
- The client reaches `crypto_auth_hmacsha256_*` (`hmac`) and the test reaches `sodium_memcmp` without it. libsodium documents `sodium_init` as required before any other call.
- It works today because SHA-256 has no CPU dispatch, but it is a latent contract break. Fix: call `sodium_init()` once in `hmac()`, or in a function-local static used by all three.

**6. Low. The nonce size is defined in two places with no link between them.**
- `netPacket.h:293` hard-codes `nonce[16]`; proofs and `admin_request` hard-code `[32]`; `Credentials.h:9` declares `NONCE_SIZE`/`MAC_SIZE` separately.
- `Server.cpp:404-405` writes `joinNonce[playerID]` (`NONCE_SIZE`) but copies `sizeof(gameVersion.nonce)` bytes; `ClientRecv.cpp:286` copies `sizeof(serverNonce)`. If the two ever diverge, that is a silent over-read.
- Fix: add `static_assert(sizeof(net_svcl_gameversion::nonce) == credentials::NONCE_SIZE)` (and the same for proof/login/password against `MAC_SIZE`) in `netPacket.h`, or use the constants there directly.
- Also: `Server::joinNonce` (`Server.h:59`) is uninitialised. It is harmless, since it is written before the slot's first accepted packet, but `= {}` costs nothing.

**7. Low (simplification). `credentials::prove` allocates a `std::vector` per call (`Credentials.cpp:383-385`).**
- Feeding the HMAC state directly is shorter, needs no allocation and needs no separate public `hmac()` beyond the test: `init(key)`, `update(label, strlen+1)`, `update(nonce, NONCE_SIZE)`, `final`.
- The public `hmac()` exists only for the RFC 4231 test. That is acceptable, but say so in the header.

**8. Low. Docs are inaccurate or stale.**
- `KEY_QUESTIONS.md:110` cites `f53b3bb`, which is not on this branch (`git merge-base --is-ancestor` fails; the branch commit is `4c02cba`). It also claims the server password is fixed, which finding 1 contradicts.
- `ARCHITECTURE.md:16` still says protocol `GAME_VERSION_SV/CL = 22000` at `Server.h:32`.
- `ARCHITECTURE.md:119` (the `test_master.cpp` row) still lists "IPs and MACs".
- Inventory order: the `Credentials.{cpp,h}` rows sit between `Console.cpp` and `Console.h`, and `test_join_password.cpp` comes after `test_validator.cpp`.
- `ClientRecv.cpp:61`: the stale comment "We are also server, retreive password" now sits above a ternary.

**9. Low. Master protocol compatibility is undocumented.**
- `stCacheBan` goes from 88 to 68 bytes, `stCacheBanned` from 38 to 18 and `stCachePlayer` from 76 to 56. The master protocol has no version (`DBVersion` only triggers the account-URL reply), and `cNetManager.cpp:220/251` `memcpy`s `sizeof` with no size check.
- A new game server talking to an old master makes the old master over-read its receive buffer. An old server talking to a new master corrupts `Duration`/`Date`/`Pass`, so its bans are silently dropped.
- Correct only if master and servers are always deployed together. Say that in the step notes or ADR, or add a size check in `cNetManager::ReceiveServerPacket`.
- Existing `master.db`: verified fine. Every query names its columns, so the leftover `MAC` column receives NULL. Old bans that matched by MAC alone lose effect, as the notes say.

**10. Low (commit hygiene).**
- `49940c5` ("step10.4: no MAC identity") also carries two unrelated changes: the account-login blanking in relayed `PLAYER_INFO` (`ServerRecv.cpp:425-429` plus its test, which is Q-S5/10.2 work) and the player-cache wrong-index fix.
- The project rule is one commit per task item. Split them, or at least name them in the commit subject.

## Verified as correct
- **HMAC construction:** domain separation by NUL-terminated label plus a fixed 16-byte nonce. Join and admin labels cannot be swapped. Constant-time `sodium_memcmp`. Nonce from `randombytes_buf`.
- **Nonce lifetime:** a new nonce per accepted TCP connection (`Server.cpp:404`), written before any packet from that slot is accepted. A reconnect into the same slot gets a fresh nonce.
- **Replay:** a proof replayed on another connection is rejected (tested). Admin requests are gated on `joined` (`ServerRecv.cpp:61`), so `joinNonce[slot]` is always set.
- **Listen-server host:** uses `gameVar.sv_password` over its own loopback nonce.
- **Client paths:** the client stores the nonce before the version branch, so the deferred handshake works. The re-query path now goes through the same `sendVersionAccepted`.
- **MAC removal:** gone from `game/src` and `masterserver/src` (only `bb_getMyMAC` stays in babonet; `main.cpp:310` is commented out).
- **Wire layout:** `static_assert`s, `PacketCheck` tables and the corpus sizes (33/64/102) agree. The masterserver's own unpacked `cMSstruct.h` copies still come out at 68/18/56 bytes.

## Not checked / unsure
- I did not mutate the code to confirm the step notes' "test fails without the fix" claims. By reading, the server-side replay and wrong-password checks clearly depend on `credentials::check`. The relay-blank and SV-push checks would also fail without their fixes.
- Did not run `tools/review.sh --full`, the ASan preset, Windows, or the CI fuzz job. The PR #40 lookup with `gh` failed (the PR number did not resolve).
- The `macos-universal` preset (triplet `universal-osx`): libsodium's vcpkg port builds with autotools there and may not build for two architectures. Not tried.
- Pre-existing, out of scope but relevant to Q-S5:
  - `PLAYER_INFO` is accepted before the password handshake (`ServerRecv.cpp:61`), so an unauthenticated connection on a passworded server triggers the name broadcast, the account auth with its MD5 and the master ban query.
  - Every relayed `PLAYER_INFO` still carries each player's IP to all clients.
  - The hard-coded ban password `roxbabo` goes to the master in cleartext.
  - Consider logging these as PNS items.
- Whether `Server` can be recreated while babonet connections survive, which would leave `joinNonce` unset, was not traced.
