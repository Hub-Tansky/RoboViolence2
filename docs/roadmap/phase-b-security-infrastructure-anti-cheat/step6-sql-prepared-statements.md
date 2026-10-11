# Step 6: SQL prepared statements

**Status:** DONE (2026-10-11)

**Depends on:** [Step 3](step3-test-harness.md). **Next:** [step7-medium-low-defects.md](step7-medium-low-defects.md). **Index:** [README.md](README.md)

## Scope

| Field | Value |
|---|---|
| Goal | Every SQL statement in the game and the master server uses `sqlite3_prepare_v2` with bound parameters |
| In scope | Tasks 6.1–6.3 below |
| Out of scope | Schema changes beyond what binding needs; removing MAC from identity (step 10); account storage and KDF (PNS-14) |
| Allowed paths | `game/src/CMaster.cpp`, `game/src/GameVar.cpp`, `game/src/Paths.cpp`, `game/src/Scene.cpp`, `game/src/SqlStatement.h` (owner-approved 2026-10-11: the game's helper), `masterserver/src/**`, `content-seed/**`, `tests/**`, `docs/analysis/KEY_QUESTIONS.md`, `ARCHITECTURE.md`, `docs/roadmap/**` |
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

## Notes

- The ban queries read `Select *` results through fixed offsets (`BV2_BAN_*` = `CurrentRow*nColumn+6`…) that assumed a column order the seeded `BanList` table doesn't have; they now name their columns, and the offset macros are gone.
- Names, maps, nicks and survey text were "escaped" by turning `'` into a backtick; bound, they are stored as sent.
- The master NUL-terminates the text fields of the ban messages (`CACHE_BAN_LIST`, `CACHE_BAN`, `CACHE_BANNED`) before binding them; the rest of the master's packet checks is not in this step.
- Commented-out SQL (`RegisterPlayer`, `RegisterServer`) was deleted rather than converted.
- The ban messages are authorised by a password compared in the source (`cNetManager.cpp`); that is Q-S5 (step 10).
- The ban list's filter is `Nick LIKE '%' || ? || '%'`: an empty filter no longer lists rows whose `Nick` is NULL (only hand-inserted rows can have one; `AddBan` always sets it).
- `SqlStatement` logs a failed prepare (schema drift) to stderr; a ctest keeps the two copies identical.
- `masterserver/README.md` still describes `sprintf` SQL (outside this step's paths) → PNS-36.

## Critical files

- `masterserver/src/cMasterServer.cpp`, `masterserver/src/cNetManager.cpp`, `game/src/CMaster.cpp`

## Acceptance checks

```bash
grep -nE "sqlite3_(exec|get_table|mprintf)" game/src/*.cpp masterserver/src/*.cpp
```

- Lists only constant DDL, each with a comment saying so.
- `ctest` passes, including the injection tests.
- Q-S3 is marked fixed in `KEY_QUESTIONS.md`.
- `tools/review.sh --full <preset>` passes, and the fresh-context review record `reviews/step6.md` and the raw reviewer report `reviews/step6-report.md` (with the skill's load line) exist ([REVIEW.md](../../../REVIEW.md) section 5).
- `ARCHITECTURE.md` lists every file added, moved or removed in this step.
- The secret scan (`gitleaks git --staged` / CI) is clean.
