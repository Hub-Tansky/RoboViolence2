# Step 1 review

Reviewer: fresh-context subagent, /thermo-nuclear-code-quality-review, two rounds (2026-10-14)
Diff: round 1 db8f31a..89b8e04; round 2 89b8e04..8a54f56
Skill evidence: both rounds `SKILL: Launching skill: thermo-nuclear-code-quality-review` (raw reports: [step1-report.md](step1-report.md))

## review.sh

`tools/review.sh --full macos-arm64` on 89b8e04: architecture, hygiene, content-case, original-assets, secrets, diff-rules, configure, build, ctest all PASS (owner's uncommitted `SECURITY.md` and step 9 edits set aside for the run). ctest after 218f9e1: 20/20 pass, including `paths` and `lang`.

## Findings (round 1)

| # | Finding | Outcome |
|---|---|---|
| 1 | A saved `languageFile` resolves into the player's data | fixed in 3383a9a (`contentFile`); `test_paths` case. The `reloadLanguage` wiring has no direct test (SDL and `CString` glue) |
| 2 | `config/local` follows the data root | fixed in 3383a9a, narrowed in 218f9e1 (package only). Fix proof: with the old loop, `test_paths` prints `FAIL config/local beside the package` |
| 3 | Two "is this BV2 data" rules | fixed in 3383a9a and 218f9e1 (`findRoot`, one rule) |
| 4 | `chooseDataDir` re-reads the env var and rewrites the saved file | fixed in 3383a9a; 218f9e1 adds `dataFromEnv()` |
| 5 | `game/README.md` not updated | fixed in 8a54f56 |
| 6 | ADR 0005 and 0007 Status lines | fixed in 8a54f56 |
| 7 | "Use placeholders" not remembered | rejected: approved behaviour (PNS-51); recorded in ADR 0016 Consequences |
| 8 | Magic index in `findDataIn` | fixed in 3383a9a |
| 9 | Defensive empty check in `pickFile` | fixed in 3383a9a |
| 10 | Language folder scanned twice | fixed in 3383a9a |
| 11 | Detect test can find a real Windows install | fixed in 3383a9a |
| 12 | Test gaps | fixed in 3383a9a (invalid `BV2_DATA_DIR`, data-only script); manual check: see Not done |
| 13 | SDL video started and stopped before `dkwInit` | open: owner's manual check (Not done) |
| 14 | Docs miss the `BaboViolent2` Program Files name | fixed in 8a54f56 |
| 15 | Tasks 1.1 and 1.2 in one commit | rejected: both edit the same `Paths.cpp` functions; noted here, no history rewrite |

## Findings (round 2)

| # | Finding | Outcome |
|---|---|---|
| 1 | Header comments claim one rule while two thresholds remain | fixed in 218f9e1 (`BV2_DATA_DIR` also needs `main/textures/`) |
| 2 | `dataRoot() != packageRoot()` in three places | fixed in 218f9e1: `dataFromEnv()` for the picker, `config/local` no longer branches; one use left in `reloadLanguage` |
| 3 | `BV2_DATA_DIR` equal to the package lets the picker override it | fixed in 218f9e1 (`dataFromEnv()`) |
| 4 | `config/local` beside the data root has no user | fixed in 218f9e1 (dropped) |
| 5 | `config/README.md` doesn't say where `config/local` is read | fixed in 218f9e1 |
| 6 | Review record missing | this file |
| 7 | `game/README.md` lines read as contradictory | fixed in 218f9e1 |
| 8 | Saved `en.lang` loaded twice | rejected: harmless, as the reviewer says |

## Not done / unsure

- Owner check with original data on macOS: the dialog shows; the chosen folder is saved and the next start doesn't ask; "Use placeholders" starts with placeholders; the game window opens normally after the dialog (round 1 #13).
- Windows run of the dialog: not tested (no Windows machine in this session).
