# Step 4: Packet hygiene and sender binding

**Status:** TODO

**Depends on:** [Step 3](step3-test-harness.md). **Next:** [step5-crash-fixes.md](step5-crash-fixes.md). **Index:** [README.md](README.md)

## Scope

| Field | Value |
|---|---|
| Goal | No client can act as another player, and no malformed client or server message can read out of bounds |
| In scope | Tasks 4.1–4.5 below |
| Out of scope | Anti-cheat rules (steps 8–9); crash fixes R1–R5 (step 5); a new serialisation format (PNS-17); anything visible to players (**[GUI]**) |
| Allowed paths | `game/src/ServerRecv.cpp`, `game/src/ClientRecv.cpp`, `game/src/Server.{h,cpp}`, `game/src/Client.h`, `game/src/netPacket.h`, `game/src/Console.cpp` (`svChange` only), `tests/**`, `docs/analysis/KEY_QUESTIONS.md`, `ARCHITECTURE.md`, `docs/roadmap/**` |
| Inputs | `AGENTS.md`, `ARCHITECTURE.md`, `docs/analysis/KEY_QUESTIONS.md` Q-S1, Q-S2, Q-S4, Q-S6, Q-S7, step 3 harness |
| Deliverables | One slot-binding helper used by every handler; range and size checks; tests per finding |
| Definition of done | See "Acceptance checks". All must pass. `ARCHITECTURE.md` is updated |

## Context

- Q-S1: most handlers index `game->players[packet.playerID]` without checking that the slot belongs to `bbnetID`. Only `COORD_FRAME` compares `babonetID`. Affected: `TEAM_REQUEST`, `PLAYER_CHANGE_NAME`, `VOTE`, `SPAWN_REQUEST`, `PLAYER_SHOOT`, `PLAYER_PROJECTILE`, `SHOOT_MELEE`, `PONG`. The handlers start at `game/src/ServerRecv.cpp:38`.
- Q-S2: `playerID` is a signed `char` indexing 32 slots; `weaponID` indexes `gameVar.weapons[20]`. Neither is range-checked.
- Q-S6: `MAP_REQUEST` copies `mapName` (`ServerRecv.cpp:50`) without NUL-termination or path checks.
- Q-S7: `PLAYER_UPDATE_SKIN` (`ServerRecv.cpp:1145`) rebroadcasts once per player (N²); `PLAY_SOUND` (`ServerRecv.cpp:500`) relays unvalidated.
- Q-S4: the client applies any variable from `SV_CHANGE` (`game/src/ClientRecv.cpp:544`), not only `sv_*`.
- This binding is the slot ownership of former C.3 and the base of `ServerValidator` (step 8).

## Tasks

### 4.1 Resolve the sender's slot once

- At the top of `recvPacket`, map `bbnetID` to the player slot. Handlers use it and ignore the packet's `playerID`; unknown senders get only the handshake messages.

### 4.2 Range and size checks

- Check `typeID` against the expected struct size. Range-check `playerID`, `weaponID`, `projectileID` and team/vote indices before use. Drop and log invalid packets.

### 4.3 Strings

- NUL-terminate every fixed-size `char[]` from the wire. `mapName` allows `[A-Za-z0-9_-]` only, with no path separators.

### 4.4 Skin and sound relay

- `UPDATE_SKIN`: one broadcast per update. `PLAY_SOUND`: validate the sound ID and position; relay only for the sender's own player.

### 4.5 Client `SV_CHANGE` filter

- Accept only variables whose name starts with `sv_`; ignore and log the rest.

## Critical files

- `game/src/ServerRecv.cpp`, `game/src/ClientRecv.cpp`, `game/src/netPacket.h`

## Acceptance checks

```bash
ctest --test-dir build/linux-x64-asan --output-on-failure
```

- Passes, including one test per Q-S item above. The step 3 expected-failure inputs now pass.
- The CI `fuzz` job finds no crash in 2 min per message type.
- The manual test DM round passes on one OS (owner).
- Q-S1, Q-S2, Q-S4, Q-S6 and Q-S7 in `KEY_QUESTIONS.md` are marked fixed, with the commit.
- [REVIEW.md](../../../REVIEW.md) checklist and `/anthropic-skills:thermo-nuclear-code-quality-review` done on the step diff; every finding fixed or confirmed with the owner.
- `ARCHITECTURE.md` lists every file added, moved or removed in this step.
- The secret scan (`gitleaks git --staged` / CI) is clean.
