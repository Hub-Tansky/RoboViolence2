# Step 7: Medium and low defects

**Status:** TODO

**Depends on:** [Step 3](step3-test-harness.md). **Next:** [step8-server-validator.md](step8-server-validator.md). **Index:** [README.md](README.md)

## Scope

| Field | Value |
|---|---|
| Goal | The remaining register defects R6–R10, R12, R13, R15, R19, the remaining R18 sites and babonet R7 are fixed, each with a test |
| In scope | Tasks 7.1–7.3 below |
| Out of scope | R11 and R14 (fixed in Phase A step 3); gameplay balance changes beyond the defect; babonet UDP / `cPeer2Peer` (PNS-17) |
| Allowed paths | `game/src/**`, `engine/babonet/src/**`, `tests/**`, `docs/analysis/KEY_QUESTIONS.md`, `ARCHITECTURE.md`, `docs/roadmap/**` |
| Inputs | `AGENTS.md`, `ARCHITECTURE.md`, `docs/analysis/KEY_QUESTIONS.md` Part D |
| Deliverables | One commit per defect with its test |
| Definition of done | See "Acceptance checks". All must pass. `ARCHITECTURE.md` is updated |

## Context

Current locations (the `KEY_QUESTIONS.md` paths predate step 2):

- R6: `clientProjectiles.erase(projectiles.begin()+i)` uses the wrong vector's iterator (`game/src/Game.cpp:844`).
- R7: babonet's partial-body receive writes at offset 0, corrupting messages split over three or more reads (`engine/babonet/src/cClient.cpp` near 735).
- R8: auto-balance moves one player per cycle, miscounts, and can desync a team (`game/src/Server.cpp`).
- R9: `queryNextMap` (`game/src/Server.cpp:293`) reloads the current map and can index an empty vector.
- R10: `collisionClip` reads neighbour cells before clamping (`game/src/MapRender.cpp`).
- R12: the two CTF dropped-flag radii differ, and the red-return log text is wrong (`game/src/ServerCTF.cpp`).
- R13: the Pro SND spawn bound check uses `>` instead of `>=` (`game/src/GameSpawn.cpp`).
- R15: the ping average divides a 59-sample sum by 60 (`game/src/PlayerUpdate.cpp`).
- R18 (CodeQL, rest): `int` multiplication before widening in `game/src/CHost.cpp:167`, `game/src/CAStar.cpp:91, 95, 195`, and the vendored `game/src/tinyxmlparser.cpp:467–494`.
- R19 (CodeQL): `%i` for an `unsigned long` (`game/src/Console.cpp:2253`).

## Tasks

### 7.1 Network and memory: R6, R7, R10

- R7 gets a babonet test that splits one message over three reads.

### 7.2 Server logic: R8, R9, R13

- Tests through the step 3 harness. R8 is visible to players (team moves): describe the fixed behaviour in the PR.

### 7.3 Small fixes: R12, R15, R18 (rest), R19

- R12 makes both flag radii equal; ask the owner which value (0.5 or 0.25) before changing it, since it is gameplay.

## Critical files

- `game/src/Game.cpp`, `game/src/Server.cpp`, `engine/babonet/src/cClient.cpp`, `game/src/ServerCTF.cpp`

## Acceptance checks

```bash
ctest --test-dir build/linux-x64-asan --output-on-failure
```

- Passes with a test for each defect.
- Every listed defect is marked fixed in `KEY_QUESTIONS.md`, with the commit.
- `ARCHITECTURE.md` lists every file added, moved or removed in this step.
- The secret scan (`gitleaks git --staged` / CI) is clean.
