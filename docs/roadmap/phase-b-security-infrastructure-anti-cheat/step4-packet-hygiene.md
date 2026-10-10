# Step 4: Packet hygiene and sender binding

**Status:** IN PROGRESS

**Depends on:** [Step 3](step3-test-harness.md). **Next:** [step5-crash-fixes.md](step5-crash-fixes.md). **Index:** [README.md](README.md)

## Scope

| Field | Value |
|---|---|
| Goal | No client can act as another player, and no malformed client or server message can read out of bounds |
| In scope | Tasks 4.1–4.6 below |
| Out of scope | Anti-cheat rules (steps 8–9); crash fixes R1–R5 (step 5); a new serialisation format (PNS-17); anything visible to players (**[GUI]**) |
| Allowed paths | `game/src/ServerRecv.cpp`, `game/src/ClientRecv.cpp`, `game/src/Server.{h,cpp}`, `game/src/Client.{h,cpp}` (receive loop only), `game/src/netPacket.h`, `engine/babonet/include/baboNet.h`, `engine/babonet/src/baboNet.cpp` (`bb_clientReceive` size only), `game/src/Console.cpp` (`svChange` only), `tests/**`, `docs/analysis/KEY_QUESTIONS.md`, `ARCHITECTURE.md`, `AGENTS.md` (untrusted-data line only), `docs/roadmap/**` |
| Inputs | `AGENTS.md`, `ARCHITECTURE.md`, earlier review records `reviews/step*.md`, `docs/analysis/KEY_QUESTIONS.md` Q-S1, Q-S2, Q-S4, Q-S6, Q-S7, step 3 harness |
| Deliverables | One pre-dispatch check (size and sender slot) before the `recvPacket` switch; range checks; tests per finding |
| Definition of done | See "Acceptance checks". All must pass. `ARCHITECTURE.md` is updated |

## Context

- Q-S1: most handlers index `game->players[packet.playerID]` without checking that the slot belongs to `bbnetID`. Only `COORD_FRAME` compares `babonetID`. Affected: `TEAM_REQUEST`, `PLAYER_CHANGE_NAME`, `VOTE`, `SPAWN_REQUEST`, `PLAYER_SHOOT`, `PLAYER_PROJECTILE`, `SHOOT_MELEE`, `PONG`. The handlers start at `game/src/ServerRecv.cpp:38`.
- Q-S2: `playerID` is a signed `char` indexing 32 slots; `weaponID` indexes `gameVar.weapons[20]`. Neither is range-checked.
- Q-S6: `MAP_REQUEST` copies `mapName` (`ServerRecv.cpp:50`) without NUL-termination or path checks.
- Q-S7: `PLAYER_UPDATE_SKIN` (`ServerRecv.cpp:1145`) rebroadcasts once per player (N²); `PLAY_SOUND` (`ServerRecv.cpp:500`) relays unvalidated.
- Q-S4: the client applies any variable from `SV_CHANGE` (`game/src/ClientRecv.cpp:544`), not only `sv_*`.
- The received size is discarded: `bb_serverReceive` has an `int* size` out-parameter (`engine/babonet/include/baboNet.h:121`), but `Server.cpp:921` doesn't pass it, and `bb_clientReceive` (`baboNet.h:135`) has none, so `Client.cpp:425` can't get it. Both `recvPacket`s `memcpy` `sizeof(struct)` from a shorter buffer.
- This binding is the slot ownership of former C.3 and the base of `ServerValidator` (step 8).

## Tasks

### 4.1 Pre-dispatch check

- Pass the received `size` from `bb_serverReceive` into `Server::recvPacket`. Add the same `int* size = NULL` out-parameter to `bb_clientReceive` and pass it into `Client::recvPacket`.
- One function runs before the `switch`: a `typeID → sizeof` table rejects unknown types and wrong sizes, then it maps `bbnetID` to the player slot. Step 8 moves this function into `ServerValidator`.
- Handlers use the resolved slot and ignore the packet's `playerID`; unknown senders get only the handshake messages.

### 4.2 Range checks and rejection

- Range-check `playerID`, `weaponID`, `projectileID` and team/vote indices before use.
- A rejected packet is dropped with no reply and nothing applied. It is logged once per sender and message type, so a flood cannot fill the log; scoring and rate limits are steps 8–9.

### 4.3 Strings

- NUL-terminate every fixed-size `char[]` from the wire. `mapName` allows `[A-Za-z0-9_-]` only, with no path separators.

### 4.4 Skin and sound relay

- `UPDATE_SKIN`: one broadcast per update. `PLAY_SOUND`: validate the sound ID and position; relay only for the sender's own player.

### 4.5 Client `SV_CHANGE` filter

- Accept only variables whose name starts with `sv_`; ignore and log the rest.

### 4.6 Untrusted-data rule

- Add to `AGENTS.md` "Conventions and gotchas": wire data is untrusted until the `recvPacket` pre-dispatch check passes; new message types get a size-table entry.

## Notes

- 4.1: `Server::checkPacket` and `checkServerPacket` (client) also NUL-terminate text payloads and every `char[]` field the client reads; the server terminates its fields in the handlers.
- 4.2: the server can't bound `nuzzleID` (nuzzle lists exist only in the client build); `checkServerPacket` can't either, so the client indexes them only after its weapon is known. Covered by the fuzzer, not a range table.
- 4.3: wire text also reached `CString(char* fmt, ...)` as a format in places the plan didn't list (R21–R23, the client's chat, map list and console text, `textColorLess(playerName)`); all now use `"%s"`.
- R25 (shots from a player without a weapon) is fixed here, not in step 5: the fuzz acceptance check needs it.
- `Server::sendSVChange` allowed 255 characters into the 80-byte `svChange`; now 79.
- Commits: 4.1–4.4 share one commit (the same handlers); 4.5 and the tests are separate.

## Critical files

- `game/src/ServerRecv.cpp`, `game/src/ClientRecv.cpp`, `game/src/netPacket.h`

## Acceptance checks

```bash
ctest --test-dir build/linux-x64-asan --output-on-failure
```

- Passes, including one test per Q-S item above and a truncated-packet test per message type. The step 3 expected-failure inputs now pass.
- The CI `fuzz` job finds no crash in 2 min per message type.
- The manual test DM round passes on one OS (owner).
- Q-S1, Q-S2, Q-S4, Q-S6 and Q-S7 in `KEY_QUESTIONS.md` are marked fixed, with the commit.
- `tools/review.sh --full <preset>` passes, and the fresh-context review record `reviews/step4.md` and the raw reviewer report `reviews/step4-report.md` (with the skill's load line) exist ([REVIEW.md](../../../REVIEW.md) section 5).
- `ARCHITECTURE.md` lists every file added, moved or removed in this step.
- The secret scan (`gitleaks git --staged` / CI) is clean.
