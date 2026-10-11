# Step 4 review

Reviewer: fresh-context subagent, /thermo-nuclear-code-quality-review, 2026-10-11
Diff: origin/main...52d51fb (ecf939b..52d51fb)
Skill evidence: `SKILL: Launching skill: thermo-nuclear-code-quality-review` (raw report: [step4-report.md](step4-report.md))

## review.sh

`tools/review.sh --full macos-arm64` and `--full macos-arm64-asan` at d8282bd: all checks PASS. After the fixes, both presets build and pass all 8 ctests locally; CI run 38115553398 on 1fc6700 is green on all three OSes (Debug and Release ctest, Linux ASan smoke, package-check).

## Findings

| # | Finding | Outcome |
|---|---|---|
| 1 | Client table allowed `fromID` -1, hit `weaponID` 100, flag `playerID` -1, which handlers use as indices | fixed in fdaa0dc (`PacketCheck`: `Slot` for those fields; the minibot gun only for shots) |
| 2 | `itemFlag` and flag states unchecked; weapon range 0..19 includes empty slots | fixed in fdaa0dc (`Weapon` kind = `knownWeapon`; `FlagState` -2..31) |
| 3 | Regression: in-flight flames (-2) and life packs/dropped weapons (-1) dropped for late joiners | fixed in fdaa0dc (`ProjectileWeapon` kind) |
| 4 | Client `SV_CHANGE` and `TimedMessage(chat)` still run server text as a format | fixed in fdaa0dc; AGENTS.md rule extended (1fc6700); `explicit` constructor deferred → PNS-34 |
| 5 | Clients past TCP connect but before the password handshake can send everything | fixed in fdaa0dc (`Server::joined`; only handshake and pings before it) |
| 6 | `PLAY_SOUND` not tied to the sender's own player | fixed in fdaa0dc (living sender; position replaced by the sender's) |
| 7 | Handlers re-look-up the sender; null checks after the dereference | fixed in fdaa0dc (`sender` from `checkPacket`, used by the touched handlers) |
| 8 | Hidden numeric special case in the client field encoding | fixed in fdaa0dc (explicit field kinds in `PacketCheck.cpp`) |
| 9 | Tests weaker than claimed; client check untested | fixed in f02dea3 (`checkPacket` per type and size, controls for spawn and shoot, handshake, sound position, client check on server-sent values) |
| 10 | Handler drops not logged | fixed in fdaa0dc |
| 11 | ARCHITECTURE.md rows stale | fixed in fdaa0dc |
| 12 | KEY_QUESTIONS wording; R1 commit missing | fixed in 1fc6700 |
| 13 | R1/R25 pulled in from step 5 without owner confirmation | confirmed with owner (2026-10-11): they stay in step 4 |
| 14 | Commit subjects without `.M` (repeat) | rejected: pushed history not rewritten. Also: 5bb7221 and d8282bd have each other's subjects (5bb7221 holds the docs, d8282bd the client `nuzzleID` check) |
| 15 | `sv_password`, account hashes and MAC addresses broadcast to clients | deferred → PNS-35 |

## Fix proof

Each fix reverted, `test_packet_hygiene`:
- Q-S1 (playerID binding off): `FAIL tests/test_packet_hygiene.cpp:75: game()->players[slotA]->teamID == teamA` (and the rename checks).
- #5 handshake gate off: `FAIL tests/test_packet_hygiene.cpp:147: sentOfType(NET_CLSV_SVCL_CHAT) == 0`.
- #3 projectile -1/-2 refused: `FAIL tests/test_packet_hygiene.cpp:176: packet::checkServerMessage(... NET_CLSV_SVCL_PLAYER_PROJECTILE, out)`.
- #2 weapon range without the null check: `FAIL tests/test_packet_hygiene.cpp:193: !packet::checkServerMessage(... NET_SVCL_PICKUP_ITEM, out)`.
- #6 sound position kept: `FAIL tests/test_packet_hygiene.cpp:139: ...position[0] == (unsigned char)game()->players[slotB]->currentCF.position[0]`.
- R1: Linux CI run before ee6e0f0: `AddressSanitizer: SEGV ... in fclose` (replay); green after.
- Step 3 crash inputs: replayed as `fixed-*.bin`, no ASan report.

## Acceptance

- Fuzzing, no masks, macOS ASan, 15 s per message type: no crash on all 20 types. The CI `fuzz` job runs on `main` after the merge (owner decision in step 3).
- Owner DM round on macOS with the CI package of 1fc6700 (2026-10-11): works.

## Not done / unsure

- The client's handlers beyond the pre-dispatch check (e.g. `projectileID` indices) are not fuzzed: the fuzzer drives the server only.
