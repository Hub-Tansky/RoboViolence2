# Step 2b review

Reviewer: fresh-context subagent, /thermo-nuclear-code-quality-review, two rounds (2026-10-10)
Diff: round 1 origin/main...03720e6; round 2 03720e6..0003a7a (merges from main not reviewed)
Skill evidence: both rounds `SKILL: Launching skill: thermo-nuclear-code-quality-review` (raw reports: [step2b-report.md](step2b-report.md))

## review.sh

`tools/review.sh --full macos-arm64` and `--full macos-arm64-release` at 0003a7a: architecture, hygiene, content-case, original-assets, secrets, diff-rules, configure, build, ctest all PASS.

CI on 177f526: `build` run 38064323679 green on all three OSes (Debug and Release build and ctest, Release packages uploaded); `package-check` passes on the Linux Release package; CodeQL: no open alerts.

## Findings (round 1)

| # | Finding | Outcome |
|---|---|---|
| 1 | CI no longer builds or tests Debug on Windows and macOS | fixed in 524dd9e (Debug build and ctest kept; `-release` built, tested and packaged) |
| 2 | No preset reproduces the shipped Release build | fixed in 524dd9e (`<os>-release` presets), 5290ea0 (docs) |
| 3 | Nothing guards that packages are Release | fixed in 92568ba (`make-package.py` refuses a non-Release build) |
| 4 | Release-only GCC warnings may fail; no CI evidence | fixed: they did fail on Linux; owner approved fixing them (task 2b.4: 107de51, 9d6725d, e9c86ff, 0003a7a); CI evidence above |
| 5 | Owner-padded scope table drifts | rejected: owner's formatting choice |

## Findings (round 2)

| # | Finding | Outcome |
|---|---|---|
| 1 | Language loader hangs and overflows on a last line without newline or `END` | fixed in 97f4396 (EOF ends the value; value bounded) |
| 2 | DKT texture: allocation in `int`, read in `size_t` | fixed in 97f4396 (one `size_t` size for both) |
| 3 | Most 2b.4 fixes have no test or fix proof | fixed for `CString::loadFromFile` and the fixed-string decode (ASan proof below); rejected for the DKO/DKT loaders, language loader, `mapName` and `getLine`: no test seam without GL and game linkage, minimal-change rule |
| 4 | `CString::loadFromFile` can leave `tmp` unterminated | fixed in 97f4396 |
| 5 | DKT chunk read repeated 6 times | fixed in 97f4396 (`readChunkDKT`) |
| 6 | `getFixedString` half explicit, half punned; encoder still puns | rejected: encoder and decoder agree (round-trip test); minimal-change rule |
| 7 | Step file and `ARCHITECTURE.md` wording | fixed in 177f526 |
| 8 | CI run IDs not recorded | fixed in this record |

## Fix proof

- Release guard (round 1 #3): `make-package.py build/macos-arm64/runtime …` on a Debug build: `build/macos-arm64 is not a Release build`, exit 1. It also caught the first preset version (`inherits: [os, release]` kept Debug).
- `bv2ReadBytes` zero-fill: `test_fileio` with the `memset` removed: `FAIL tests/test_fileio.cpp:85: buf[0] == 200 && buf[1] == 1 && buf[2] == 0 && buf[3] == 0`.
- Fixed-string decode, with the old bitfield pun restored, `macos-arm64-asan`: `SUMMARY: AddressSanitizer: stack-buffer-overflow FileIO.cpp:195 in FileIO::getFixedString()`.
- `CString::loadFromFile` bound, reverted to `i<MAX_CARAC`, `macos-arm64-asan`: `SUMMARY: AddressSanitizer: stack-buffer-overflow CString.cpp:156 in CString::set(char*, ...)`.
- Linux `-Werror` fixes: CI run 38062289838 listed 23 errors on Linux Release; run 38064323679 builds clean.
- Fullscreen: owner, macOS 27.0.1, CI package of 107de51: starts fullscreen.

## Not done / unsure

- No unit tests for the truncated-file paths in the DKO and DKT loaders, the language loader, `mapName` and `FileIO::getLine` (round 2 #3).
- Windows and Linux Release packages not started by a person in this step (the Windows client needs an OpenGL 2.1 PC, PNS-31).
