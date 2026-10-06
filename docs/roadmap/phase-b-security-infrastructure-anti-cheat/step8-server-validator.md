# Step 8: ServerValidator (refactor, no behaviour change)

**Status:** TODO

**Depends on:** [Step 4](step4-packet-hygiene.md). **Next:** [step9-anti-cheat-checks.md](step9-anti-cheat-checks.md). **Index:** [README.md](README.md)

## Scope

| Field | Value |
|---|---|
| Goal | All server-side cheat checks live in one `ServerValidator` with a per-player violation score, and behaviour is identical to before |
| In scope | Tasks 8.1–8.4 below |
| Out of scope | New checks and rate limits (step 9); telemetry and replays (PNS-13); server-authoritative movement (PNS-13); client-side changes |
| Allowed paths | `game/src/ServerRecv.cpp`, `game/src/Server.{h,cpp}`, `game/src/ServerValidator.{h,cpp}` (new), `game/src/Player.{h,cpp}` (`speedHackCount` only), `game/src/GameVar.{h,cpp}` (new `sv_*` only), `game/CMakeLists.txt`, `tests/**`, `ARCHITECTURE.md`, `docs/roadmap/**` |
| Inputs | `AGENTS.md`, `ARCHITECTURE.md`, `docs/analysis/ALGORITHM_02-Movement-Collision-Interpolation.md` §5, `docs/analysis/KEY_QUESTIONS.md` Q2, step 4 slot binding |
| Deliverables | `ServerValidator` class; tests pinning today's kick behaviour; `ARCHITECTURE.md` note |
| Definition of done | See "Acceptance checks". All must pass. `ARCHITECTURE.md` is updated |

## Context

- The client is open source, so it can't be trusted. Defences are server-side only; no client-integrity checks.
- Today's checks are ad-hoc inside handlers:
  - spawn weapon whitelist with a kick, `game/src/ServerRecv.cpp:645`;
  - speed-hack counter `Player::speedHackCount` (`game/src/Player.h:211`), one velocity sample per 3 s window, kick at 3, `game/src/ServerRecv.cpp:795–821`.
- Movement is client-authoritative (Q2). Server-authoritative movement needs the sim core and UDP (PNS-12, PNS-13). This validator is the layer those later checks plug into, so the game isn't rewritten twice.

## Tasks

### 8.1 Pin today's behaviour

- Harness tests: an invalid spawn weapon kicks; three speed violations in a row kick; a valid player is never kicked. They pass before the refactor.

### 8.2 `ServerValidator`

- Owned by `Server`, it receives the resolved slot from step 4. API: `onSpawnRequest`, `onCoordFrame`, `onShoot`, and so on, each returning accept/reject.
- A per-player violation score with configurable thresholds and actions (log, warn, kick, temp-ban). Actions reuse the existing kick and disconnect messages (no **[GUI]** change).

### 8.3 Move the two checks

- Move the weapon whitelist and the speed counter into the validator as rules with today's thresholds. Remove `speedHackCount` from `Player`.

### 8.4 Configuration

- Thresholds as `sv_*` variables registered in `game/src/GameVar.cpp`, with defaults equal to today's.

## Critical files

- `game/src/ServerRecv.cpp`, `game/src/Server.h`, `game/src/Player.h`

## Acceptance checks

```bash
ctest --test-dir build/linux-x64-asan --output-on-failure -R validator
```

- The 8.1 tests pass unchanged before and after the refactor.
- `grep -n speedHackCount game/src` finds nothing.
- `/thermo-nuclear-code-quality-review` has run on the step diff ([REVIEW.md](../../../REVIEW.md)).
- `ARCHITECTURE.md` lists every file added, moved or removed in this step.
- The secret scan (`gitleaks git --staged` / CI) is clean.
