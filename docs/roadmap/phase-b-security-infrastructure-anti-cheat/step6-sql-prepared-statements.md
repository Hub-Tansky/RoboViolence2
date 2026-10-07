# Step 6: SQL prepared statements

**Status:** TODO

**Depends on:** [Step 3](step3-test-harness.md). **Next:** [step7-medium-low-defects.md](step7-medium-low-defects.md). **Index:** [README.md](README.md)

## Scope

| Field | Value |
|---|---|
| Goal | Every SQL statement in the game and the master server uses `sqlite3_prepare_v2` with bound parameters |
| In scope | Tasks 6.1–6.3 below |
| Out of scope | Schema changes beyond what binding needs; removing MAC from identity (step 10); account storage and KDF (PNS-14) |
| Allowed paths | `game/src/CMaster.cpp`, `game/src/GameVar.cpp`, `game/src/Paths.cpp`, `game/src/Scene.cpp`, `masterserver/src/**`, `content-seed/**`, `tests/**`, `docs/analysis/KEY_QUESTIONS.md`, `ARCHITECTURE.md`, `docs/roadmap/**` |
| Inputs | `AGENTS.md`, `ARCHITECTURE.md`, earlier review records `reviews/step*.md`, `docs/analysis/KEY_QUESTIONS.md` Q-S3, step 3 master tests |
| Deliverables | A small statement helper per program; all call sites converted; injection tests |
| Definition of done | See "Acceptance checks". All must pass. `ARCHITECTURE.md` is updated |

## Context

- Q-S3: the client-reported MAC (`netPacket.h:428`, forwarded at `game/src/ServerRecv.cpp:466–491`) is interpolated into master-server SQL (`masterserver/src/cNetManager.cpp`, `cMasterServer.cpp`). Up to 19 attacker characters reach SQL.
- sqlite call sites: `masterserver/src/cMasterServer.cpp` (21), `cNetManager.cpp` (2), `game/src/CMaster.cpp` (4), `GameVar.cpp` (2), `Paths.cpp` (1), `Scene.cpp` (1).

## Tasks

### 6.1 Statement helper

- RAII wrapper over `sqlite3_stmt` (prepare, bind text/int, step, finalize), one copy for `masterserver` and one for `game` (no shared library yet).

### 6.2 Convert every call site

- Replace `sprintf`/`sqlite3_mprintf`-built SQL and `sqlite3_exec` with string data by bound statements. Constant DDL may keep `sqlite3_exec`.

### 6.3 Injection tests

- The master tests send names, IPs and MAC fields containing `'`, `--`, `;` and `%`. The tables are unchanged and the rows are stored literally.

## Critical files

- `masterserver/src/cMasterServer.cpp`, `masterserver/src/cNetManager.cpp`, `game/src/CMaster.cpp`

## Acceptance checks

```bash
grep -nE "sqlite3_(exec|get_table|mprintf)" game/src/*.cpp masterserver/src/*.cpp
```

- Lists only constant DDL, each with a comment saying so.
- `ctest` passes, including the injection tests.
- Q-S3 is marked fixed in `KEY_QUESTIONS.md`.
- `tools/review.sh --full <preset>` passes, and the fresh-context review record `reviews/step6.md` exists ([REVIEW.md](../../../REVIEW.md) section 5).
- `ARCHITECTURE.md` lists every file added, moved or removed in this step.
- The secret scan (`gitleaks git --staged` / CI) is clean.
