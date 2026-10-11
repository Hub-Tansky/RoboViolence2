# Step 9: First anti-cheat checks

**Status:** DONE (2026-10-12)

**Depends on:** [Step 8](step8-server-validator.md). **Next:** [step10-server-credentials.md](step10-server-credentials.md). **Index:** [README.md](README.md)

## Scope

| Field | Value |
|---|---|
| Goal | The server detects impossible fire rates, ammo use, reloads, weapon switches, movement and message floods, logging by default and enforcing on demand |
| In scope | Tasks 9.1–9.4 below |
| Out of scope | Aimbot and wallhack detection, telemetry, replays, information hiding, reports and sanctions (PNS-13); server-authoritative movement (PNS-13); bans by account (PNS-14) |
| Allowed paths | `game/src/ServerValidator.{h,cpp}`, `game/src/ServerRecv.cpp`, `game/src/Server.{h,cpp}`, `game/src/GameVar.{h,cpp}` (new `sv_*` only), `config/bv2.example.cfg` (`sv_antiCheat*`, owner 2026-10-12), `tests/**`, `docs/analysis/**`, `ARCHITECTURE.md`, `docs/roadmap/**` |
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
- Owner, 2026-10-11: rules that drop or kick today (fire rate, rocket without bazooka, grenade ammo, spawn weapon whitelist) keep enforcing at every level and also score at `1` and `2`. The step 8 speed/frame kick stays at `1` and `2`, off at `0`. The rules below are new.
- At `1`, every packet is applied unchanged. At `2`, a weapon-rule violation drops the packet (the server decides hits, so nothing desyncs). A movement-rule violation never drops `COORD_FRAME`s: there is no server-to-client position correction, so dropped frames freeze the player for everyone else. It scores, and the score's action (warn, kick) applies.

### 9.2 Weapon rules

- Shots faster than `fireDelay` (with tolerance), a shot whose `weaponID` is not the held weapon, shots and projectiles long after death, grenade ammo, rockets without a bazooka, spawn requests while alive or before `sv_timeToSpawn`.
- Owner, 2026-10-11: guns have no ammo or reload, and weapons change only by a server-decided pickup, so "reload" and "weapon-switch timing" are dropped.

### 9.3 Movement rules (interim until server-authoritative movement)

- Per-tick maximum displacement against the speed rules, a wall-crossing check against `Map`, and teleport detection. Spawns, deaths and map changes reset state. Runs beside the 8.3 speed rule (owner: it keeps its kick).

### 9.4 Rate limits

- Per-message-type budget per player (chat, votes, name changes, skin updates, pings, team requests). Excess scores, and at `2` is dropped.

## Notes

- Rules, limits and tuning: [ANTI_CHEAT_TUNING.md](../../analysis/ANTI_CHEAT_TUNING.md). Tests: `validator`.
- Each rule has a positive and a negative test. Fix proof: with each rule, budget, tolerance or allowance (send interval, pickup grace, stuck-in-wall respawn, inner half of a wall cell, short chords, once per window) removed, a test fails.
- No test tells apart: the path-speed window timed by client frames (a stall long enough to show it also trips the step 8 frame check, PNS-41), and the reset in `onSpawned` (the `timeAlive` fallback covers the same case).
- Not caught: a speed up to about 4.25 units/s (path slack).
- Play-test (owner, 2026-10-12, CI package of 5b70702, `sv_antiCheat 1`): macOS server, a macOS client on the server and a Linux client over the LAN, CTF. Spawns, team switches, chat, a spectator idle. The log has no `anti-cheat:` line; it is on PR #39 (LAN address masked).

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
