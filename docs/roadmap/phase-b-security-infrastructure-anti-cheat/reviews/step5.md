# Step 5 review

Reviewer: fresh-context subagent, /thermo-nuclear-code-quality-review, 2026-10-11
Diff: origin/main...53de22b (6d9c514..53de22b)
Skill evidence: `SKILL: Launching skill: thermo-nuclear-code-quality-review` (raw report: [step5-report.md](step5-report.md))

## review.sh

Before the review, `tools/review.sh --full macos-arm64` passed; `--full macos-arm64-asan` failed on `crash_fixes` (R30, then fixed in 1829901). After the review fixes both presets build and pass all 10 ctests locally; the CI run of the final head is recorded in the PR.

## Findings

| # | Finding | Outcome |
|---|---|---|
| 1 | R18: `memset` beside the `size_t` allocations still multiplies in `int` | fixed in bbd3012 (one `size_t` size per buffer) |
| 2 | `~Map` deletes an uninitialised `texMap` after an early R5 return | fixed in bbd3012 (`texMap(0)`); client-only path, no test (needs GL) |
| 3 | The server doesn't reapply the weapon table when `sv_serverType` changes | fixed in 17f7ef5 (`Console` after `dksvarCommand`; `sendSVChange` runs before the variable is set); test in 954e812 |
| 4 | Docs still describe Pro as the only ruleset / R3 as current | fixed in KEY_QUESTIONS Q6 and ADR 0013 context; `AGENTS.md` and `docs/analysis` lines deferred → PNS-36 |
| 5 | `MemIO` bounds by capacity, not received data; docs overstate | fixed in 8899b11 (`m_end`; also `put` grew only 50% and could overflow on a large write); test in 954e812 |
| 6 | `MemIO` duplicates every `FileIO` getter | deferred → PNS-37 |
| 7 | R20 covers only a missing file; no fix proof | fixed in 2b5fa4c (`MA_SOUND_FLAG_WAIT_INIT`); no fix proof (see Not done) |
| 8 | `GetMapData` has no `default:` | fixed in bbd3012 (callers in the editor still ignore the result: editor-only, local files) |
| 9 | R5 size check and cell loop copied 8 times | deferred → PNS-37 |
| 10 | DKT skip leaves pixel data to be parsed as chunks; null `ptrLayer` | fixed in 5c41f29 (seek to end, stop); null `ptrLayer` added to R29 (PNS-38) |
| 11 | Weak tests (R18 proof by crash, R2 timing, `test_dks` vacuous) | fixed in 954e812 (R2 counts its own rocket; `test_dks` fails if nothing loaded); R18 proof by crash accepted; KEY_QUESTIONS R18 reworded |
| 12 | Test helpers duplicated | fixed in 954e812 (`CHECK`, `deliver<T>`, `spawned` in `server_harness.h`) |
| 13 | Second `Console` constructor; literal `1` | fixed in 17f7ef5 |
| 14 | Commit bundling (db5fc7e: R16 + MemIO); R30 row without a hash | hash added; bundling noted (pushed history not rewritten) |
| 15 | Map size limit unrecorded | fixed in the step notes: all 81 original maps are at most 63x48 |
| 16 | Open acceptance items | tracked below |

## Fix proof

Each fix reverted, the test fails or crashes:
- R3: `FAIL tests/test_crash_fixes.cpp:63: gameVar.sv_serverType == 0`; the server weapon table (finding 3): `FAIL tests/test_crash_fixes.cpp:44: gameVar.weapons[WEAPON_SHIELD]->fireDelay == 2.5f`.
- R2, R4: `test_crash_fixes` exits 139 (segfault).
- R5: `FAIL ... !version.isValid` and an abort (exit 134) on the size.
- R16: `AddressSanitizer: stack-buffer-overflow FileIO.cpp:224 in FileIO::getString()`; MemIO: `heap-buffer-overflow MemIO.cpp:108 in MemIO::getLong()`.
- R17: `AddressSanitizer: stack-buffer-overflow istream:511`.
- R18: `test_crash_fixes` exits 139 (the 4 GB texture reaches GL).
- R30: `AddressSanitizer: heap-buffer-overflow CString.cpp:307 in CString::getNextToken(int)`.
- Teardown: `AddressSanitizer: heap-use-after-free miniaudio.h:69760` (`test_dks`, the smoke job's stack).
- Console format string: `smoke_server` fails.

## Not done / unsure

- R20 has no fix proof: the race didn't reproduce locally; the CI smoke job is the long-run check.
- Client paths that need a window: quit at each scene, disconnect mid-download, and the `texMap` early-return path.
- R28 (Windows babonet crash on disconnect) and R29 stay open → PNS-38.
- The CodeQL alerts for R16–R18 close on the next scan of `main`.
