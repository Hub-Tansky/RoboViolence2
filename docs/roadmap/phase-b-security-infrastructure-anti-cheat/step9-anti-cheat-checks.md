# Step 9: First anti-cheat checks

**Status:** TODO

**Depends on:** [Step 8](step8-server-validator.md). **Next:** [step10-server-credentials.md](step10-server-credentials.md). **Index:** [README.md](README.md)

## Scope

| Field | Value |
|---|---|
| Goal | The server detects impossible fire rates, ammo use, reloads, weapon switches, movement and message floods, logging by default and enforcing on demand |
| In scope | Tasks 9.1–9.4 below |
| Out of scope | Aimbot and wallhack detection, telemetry, replays, information hiding, reports and sanctions (PNS-13); server-authoritative movement (PNS-13); bans by account (PNS-14) |
| Allowed paths | `game/src/ServerValidator.{h,cpp}`, `game/src/ServerRecv.cpp`, `game/src/Server.{h,cpp}`, `game/src/GameVar.{h,cpp}` (new `sv_*` only), `tests/**`, `docs/analysis/**`, `ARCHITECTURE.md`, `docs/roadmap/**` |
| Inputs | `AGENTS.md`, `ARCHITECTURE.md`, earlier review records `reviews/step*.md`, `docs/analysis/ALGORITHM_02-Movement-Collision-Interpolation.md`, `docs/analysis/ALGORITHM_03-Weapons-Hitscan-Projectiles.md`, step 8 validator |
| Deliverables | Rules in `ServerValidator`; `sv_antiCheat`; tests per rule; a tuning note in `docs/analysis/` |
| Definition of done | See "Acceptance checks". All must pass. `ARCHITECTURE.md` is updated |

## Context

- Weapon timing lives in `gameVar.weapons[]` (`fireDelay`, ammo, reload). The server already rolls Pro spread (ALGORITHM_03).
- Coord frames carry position and velocity (`short` ×100, `char` ×10) at a ping-dependent rate. The map grid offers `collisionClip` and the ray march for a wall check.
- Lag makes movement bursty. Thresholds need tolerance and tuning on real games.

## Tasks

### 9.1 `sv_antiCheat`

- `0` off, `1` log only (default), `2` enforce. Violations always add to the score; actions apply only at `2`.

### 9.2 Weapon rules

- Shots faster than `fireDelay` (with tolerance), shots with no ammo or during reload, weapon-switch timing, spawn and team rules.

### 9.3 Movement rules (interim until server-authoritative movement)

- Per-tick maximum displacement against the speed rules, a wall-crossing check against `Map`, and teleport detection. Spawns, deaths and map changes reset state. Replaces the 8.3 speed rule.

### 9.4 Rate limits

- Per-message-type budget per player (chat, votes, name changes, skin updates, pings). Excess is dropped and scored.

## Critical files

- `game/src/ServerValidator.cpp`, `game/src/ServerRecv.cpp`, `game/src/Map.h`

## Acceptance checks

```bash
ctest --test-dir build/linux-x64-asan --output-on-failure -R validator
```

- Each rule has a positive test (cheat detected) and a negative test (normal play, jittered packets, not flagged).
- A 10-minute owner play-test at `sv_antiCheat 1` logs no violations for honest players; the log is attached to the PR.
- `tools/review.sh --full <preset>` passes, and the fresh-context review record `reviews/step9.md` and the raw reviewer report `reviews/step9-report.md` (with the skill's load line) exist ([REVIEW.md](../../../REVIEW.md) section 5).
- `ARCHITECTURE.md` lists every file added, moved or removed in this step.
- The secret scan (`gitleaks git --staged` / CI) is clean.
