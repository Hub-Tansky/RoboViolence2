# Step 5: Crash fixes R1–R5 and known undefined behaviour

**Status:** TODO

**Depends on:** [Step 3](step3-test-harness.md). **Next:** [step6-sql-prepared-statements.md](step6-sql-prepared-statements.md). **Index:** [README.md](README.md)

## Scope

| Field | Value |
|---|---|
| Goal | The high-severity defects R1–R5, the CodeQL findings R16–R18, the music-stream race R20, the known UBSan reports and the start-up/teardown null dereferences are fixed, each with a test |
| In scope | Tasks 5.1–5.6 below |
| Out of scope | Medium and low defects (step 7); SQL (step 6); restructuring object lifetimes (`unique_ptr`, PNS-19) |
| Allowed paths | `game/src/**`, `engine/babonet/src/**`, `tests/**`, `docs/analysis/KEY_QUESTIONS.md`, `docs/decisions/**`, `ARCHITECTURE.md`, `docs/roadmap/**` |
| Inputs | `AGENTS.md`, `ARCHITECTURE.md`, earlier review records `reviews/step*.md`, `docs/analysis/KEY_QUESTIONS.md` Part D, PNS-5, PNS-6 |
| Deliverables | One commit per defect with its test; an ADR for R3 |
| Definition of done | See "Acceptance checks". All must pass. `ARCHITECTURE.md` is updated |

## Context

`KEY_QUESTIONS.md` paths predate the step 2 move; current locations:

- R1: `fclose(fic)` outside `if (fic)` in the map upload loop, `game/src/Server.cpp` (`fclose` at 213, 239, 1297).
- R2: rocket owner dereferenced without a null check, `game/src/GameProjectile.cpp`.
- R3: `if ((gameVar.sv_serverType = 1))` (`game/src/GameProjectile.cpp:714`) is an assignment that forces Pro rules. It is load-bearing: fixing it changes the ruleset on servers that set `sv_serverType 0`.
- R4: the master ban answer kicks `players[ID]` without a null check (`game/src/Server.cpp`).
- R5: the map loader keeps `isValid` true for unknown versions and never bounds-checks `size` (`game/src/Map.cpp:110–206`); clients load server-supplied maps.
- R16 (critical, CodeQL): `FileIO` passes file text to `CString(char* fmt, ...)` as a format string, and `getString()` overflows `char tmp[256]` (`game/src/FileIO.cpp:73, 213, 225`). Downloaded maps reach it.
- R17 (CodeQL): unbounded `ficIn >> char[256]` in the config loader (`engine/zeven/src/CSystemVariable.cpp:93, 134`).
- R18 (CodeQL): `w*h*3` allocation sizes overflow in `int`; the map and texture sites (`game/src/Map.cpp:1112, 1682`, `engine/dko/src/ePTexture.cpp:214`) take sizes from files. The other R18 sites are in step 7.
- R20 (CI smoke, ASan): `dksPlayMusic` (`engine/zeven/src/dks.cpp:241`) fails to open a missing music file in streaming mode, and miniaudio frees the stream while its job thread still writes to it (heap-use-after-free). Timing-dependent: one failed smoke run in 30.
- PNS-5: UBSan out-of-range `bool` loads and NaN casts in `CUserLogin.cpp`.
- PNS-6: null dereferences in start-up and teardown (`Scene::disconnect`, the `Server` calls fixed in Phase A); more are likely.

## Tasks

### 5.1 R3 ruleset decision (owner)

- Ask the owner: keep Pro-only (remove the branch and `sv_serverType`'s effect), or restore the comparison. Record it as an ADR before changing code.

### 5.2 R1, R2, R4, R5

- One commit each, each with a harness or unit test that crashed under ASan before.

### 5.3 CodeQL findings R16–R18

- R16: never pass data as a format (`CString("%s", buf)` or a non-format constructor); bound `getString()`. R17: `std::string` or `std::setw`. R18 (map and texture sites): compute in `size_t` and reject sizes above a sane limit. Each with a test; the CodeQL alerts close on the next scan.

### 5.4 R20 music-stream race

- Don't reach miniaudio's failing async path: check the file exists before `ma_sound_init_from_file`, and/or initialise with `MA_SOUND_FLAG_WAIT_INIT`. Then check whether the vcpkg miniaudio version has an upstream fix and record it.
- Test: a loop that calls `dksPlayMusic` on a missing file many times under ASan.

### 5.5 UBSan defects

- Fix the reports listed in `ARCHITECTURE.md` Open items. Remove that line when they're gone.

### 5.6 Start-up and teardown audit

- Run client and server start-up/quit paths under ASan/UBSan (quit at each scene, `dedicate` before `set sv_*`, disconnect mid-map-download). Fix null dereferences; list anything larger in `possible-new-scope.md`.

## Critical files

- `game/src/Server.cpp`, `game/src/GameProjectile.cpp`, `game/src/Map.cpp`, `game/src/Scene*.cpp`, `game/src/CUserLogin.cpp`

## Acceptance checks

```bash
ctest --test-dir build/linux-x64-asan --output-on-failure
```

- Passes with a new test for each of R1, R2, R4, R5, R16, R17 and the R18 map/texture sites.
- The R20 loop test passes under ASan, and the CI smoke job shows no miniaudio use-after-free.
- No open CodeQL alert for R16–R17 or the R18 map/texture sites.
- The R3 ADR exists and its decision is implemented.
- The CI smoke job (client under xvfb, ASan) reports no UBSan errors.
- R1–R5 are marked fixed in `KEY_QUESTIONS.md`.
- `tools/review.sh --full <preset>` passes, and the fresh-context review record `reviews/step5.md` and the raw reviewer report `reviews/step5-report.md` (with the skill's load line) exist ([REVIEW.md](../../../REVIEW.md) section 5).
- `ARCHITECTURE.md` lists every file added, moved or removed in this step.
- The secret scan (`gitleaks git --staged` / CI) is clean.
