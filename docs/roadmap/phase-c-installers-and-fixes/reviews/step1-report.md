SKILL: Launching skill: thermo-nuclear-code-quality-review

# Step 1 review (Phase C): player data outside the package

Diff: `db8f31a..HEAD` (e7b8431, f76d2c2, a7ea657, 89b8e04). I made no file changes. I did not run `tools/review.sh` or `ctest`, and I did not do the manual acceptance checks (picker with original data, `execute CTF`).

## Findings

1. **High: `game/src/GameVar.cpp:1247-1251`. An explicit `languageFile` bypasses the package, so the player's original English overrides the package's.**
   - `languageFile` is a path relative to the cwd, and the cwd is now the player's data root.
   - `content/languages/README.md:21` says pre-step-13 builds saved `languageFile "main/languages/en.lang"`. For those players, `lang::load(dir, file)` loads the package `en.lang` and then the player's upstream `en.lang` on top of it. Every key upstream has is replaced by the original text, which undoes the step 2 rebrand.
   - It also breaks `languageFile "main/languages/de.po"` (package-only files), against the "package wins" rule in ADR 0016 and the README line changed in this step.
   - No test covers it.
   - Fix: resolve a non-empty relative `languageFile` through `bv2::contentFile(file)` before `load`. Add a `test_lang` or `test_paths` case where the explicit `main/languages/en.lang` must resolve to the package copy.

2. **Medium: `game/src/Paths.cpp:338-340`. Local config overrides follow the data root instead of the package.**
   - `config/local/*.cfg` is read from `g_dataRoot + "config/local"` and from cwd-relative `"config/local"`. These are the same folder, because `pathsInit` already `chdir`ed into the data root.
   - Before this step the data root was normally the package folder. Once a player picks or auto-detects data, overrides beside the executable stop applying with no warning. ADR 0007's "next to the content" now means "inside the player's BV2 copy".
   - The step treats `bv2.example.cfg` as package content (via `contentFile`) but missed this layer. The ADR does not mention it, and there is no test.
   - Fix: use `g_packageRoot + "config/local"` (keep the data-root one only if that is wanted), and state it in ADR 0016. Add a `test_paths` case.

3. **Medium: `game/src/Paths.cpp:171-181` vs `game/src/Paths.cpp:206-224`. Two definitions of "a data folder", with duplicated path normalisation.**
   - `pathsInit` handles `BV2_DATA_DIR` with its own trailing-slash and `main/` parent logic and accepts any folder with `main/` (`hasMain`).
   - `findDataIn` repeats the same normalisation but requires `main/textures`, and also accepts `Content` and `BaboViolent2/Content`.
   - Result: `BV2_DATA_DIR=~/Downloads/BaboViolent2-master` is rejected even though the picker accepts the same folder. A reader has to hold two rules.
   - Fix: give `findDataIn` a `requireTextures` flag, or a shared `normalise(dir)` helper, and route the env branch through it. The env-var tolerance and the picker's rules then come from one function. Update the ADR line on `BV2_DATA_DIR` accordingly.

4. **Medium: `game/src/DataDirPicker.cpp:79-95`. `chooseDataDir` redoes decisions `pathsInit` already made, and rewrites the saved file on every start.**
   - It re-reads `BV2_DATA_DIR`. A set but invalid env var silently suppresses lookup and the picker, because `pathsInit` fell back to the package and `chooseDataDir` returns early.
   - It calls `saveDataDir` even when the folder came from `savedDataDir()`, so `data-dir.txt` is rewritten on every launch.
   - Simpler shape: `pathsInit` reports whether the env var supplied the root (or `chooseDataDir` checks `dataRoot() != packageRoot()`). Then `if (saved) return setDataRoot(saved); if (detected) …; else ask`, saving only on detect or pick.

5. **Medium: `game/README.md`. A listed deliverable is missing.**
   - Task 1.4 names `game/README.md` ("`BV2_DATA_DIR` becomes optional for players"), but the file is unchanged.
   - Line 15 still says to run from a runtime dir containing `main/` (ADR 0005), and it does not mention `DataDirPicker`, `--placeholders` or the package/data split.
   - REVIEW.md section 2 requires every affected README to be updated (the diff changes the game's run steps and adds a module file).
   - Fix: add a short bullet covering the data lookup, `--placeholders` and the ADR 0016 link.

6. **Low: `docs/decisions/0005-runtime-main-data-root.md:3` and `docs/decisions/0007-data-root-pref-dir-config-layers.md:3`. The amended ADRs' Status lines are not updated.**
   - ADR 0016 "amends" both, but their Status lines and the index in `docs/decisions/README.md` don't say so.
   - The established convention (0008, "amended by 0009") updates the Status line and the index row.
   - Fix: set `Status: Accepted; amended by [0016](...)` on both and update the index rows.

7. **Low: `game/src/DataDirPicker.cpp:60-62` and `docs/build/*.md`. "Use placeholders" is never remembered.**
   - A player without data sees the modal on every start.
   - The docs say "restart to be asked again", so it is intentional, but it is a nag loop for anyone who chooses placeholders on purpose.
   - Fix (owner's call, since it is a [GUI] change): either save a placeholders marker in `data-dir.txt`, or record this as accepted in ADR 0016's Consequences.

8. **Low: `game/src/Paths.cpp:213-221`. Magic-index skip in `findDataIn`.**
   - `tries[4]` holds the candidate folders, and `if (i == 1 && p.filename() != "main") continue;` skips one of them by index.
   - Fix: build a `std::vector<fs::path>` and push the parent only when `p.filename() == "main"`. This removes the index coupling.

9. **Low: `game/src/Lang.cpp:217-218`. Defensive empty check with no caller that needs it.**
   - `if (dirs.empty()) return "";` guards against an input no caller passes. AGENTS.md says to skip defensive code for rare edge cases.
   - The contract "dirs[0] holds en.lang" is also stated only in a comment.
   - Fix: drop the check, or take `(const std::string & baseDir, const std::vector<std::string> & extraDirs)` so the invariant lives in the type.

10. **Low: `game/src/GameVar.cpp:1249`. The language folder is scanned twice in the default layout.**
    - With no player data, `dir` (the package path, absolute) and `"main/languages/"` (relative to a cwd that is the package) are the same folder, so every locale is probed twice.
    - It is harmless but confusing.
    - Fix: add the second folder only when `bv2::dataRoot() != bv2::packageRoot()`, or let `contentFile`-style lookup return both roots' folders without duplicates.

11. **Low: `tests/test_paths.cpp:58`. The detect test can pick up a real install on Windows.**
    - `detectDataDir()` checks Program Files before `HOME`. On a Windows machine with `C:\Program Files\BaboViolent 2` installed, the "auto-detect in ~/Downloads" check fails.
    - Fix: clear `ProgramFiles` and `ProgramFiles(x86)` in the test, as it already does for `HOME`.

12. **Low: `tests/test_paths.cpp`. Gaps in test coverage for new behaviour.**
    - Not covered:
      - an invalid `BV2_DATA_DIR` falling back to the package root;
      - `contentFile` when the package lacks the file but the data root has it;
      - `Console` `execute` resolving through `contentFile`.
    - The picker (`DataDirPicker.cpp`) is GUI and has no automated test, which is acceptable. The manual acceptance check (picker shown, choice saved, next start silent) must be recorded as evidence in `reviews/step1.md`.
    - Fix: add the two `Paths` cases. Record the manual run, or list it under "Not done / unsure".

13. **Low: `game/src/DataDirPicker.cpp:87-91`. SDL video is started and stopped before the game's own SDL init. Not verified.**
    - `SDL_Init(SDL_INIT_VIDEO)` followed by `SDL_QuitSubSystem` before `dkw` initialises SDL again.
    - On macOS, re-initialising video after a native dialog is usually fine but is unverified here.
    - Fix: confirm on macOS and Windows during the manual acceptance run and note it in the record. Or keep video initialised (no `QuitSubSystem`) if `dkw` tolerates an already-initialised subsystem.

14. **Low: `docs/build/windows.md` and `docs/build/README.md` vs `game/src/Paths.cpp:244-252`. The docs and the code disagree on Program Files names.**
    - The docs say Program Files `BaboViolent 2`.
    - The code also checks `BaboViolent2`, as the step file says.
    - Fix: list both names in the docs.

15. **Low: commit f76d2c2. Commit granularity.**
    - It combines tasks 1.1 and 1.2. AGENTS.md asks for one commit per task item.
    - No mechanical rewrite is mixed with functional changes, which satisfies REVIEW.md section 2.
    - Fix: none needed now. Note it in the record.

## Section 2 checklist

- **Ground rules:** BV2 asset loading is unchanged in path form (`main/...` relative to the data root). The [GUI] change is owner-approved per the step file (2026-10-14). No renames.
- **Tests:** `test_paths` was added and `test_lang` extended. The gaps are in items 1, 2 and 12.
- **Docs and ADRs:** ADR 0016, `ARCHITECTURE.md`, the build guides, the top-level README, AGENTS.md and `content/languages/README.md` are updated. `game/README.md` (item 5) and the Status lines of the amended ADRs (item 6) are missing.
- **Mechanical vs functional:** not mixed.
- **File sizes:** fine. `Paths.cpp` is 416 lines, `main.cpp` 628 and `DataDirPicker.cpp` 98.

## Verdict

**Not approved yet.** The structure is sound: `DataDirPicker` is isolated, and `contentFile` is a small, clear way to read package content. Two behavioural leaks in the package/data split must be fixed first:
- an explicit `languageFile` resolves into the player's data (item 1, High);
- `config/local` overrides follow the player's data (item 2).

`game/README.md` (item 5) must also be updated. Items 3 and 4 are worth doing in the same pass: they merge two "is this BV2 data" rules into one and remove the repeated env check and the save on every start. The rest are minor.

## Round 2

SKILL: Launching skill: thermo-nuclear-code-quality-review

# Step 1 review, round 2 (Phase C): player data outside the package

Scope: `git diff 89b8e04..HEAD` (3383a9a, 8a54f56), read against the round-1 report and the whole step (`db8f31a..HEAD`). I changed no files, made no commits and ignored the owner's uncommitted files. I did not run `tools/review.sh`, ctest or the manual picker check.

## Round-1 items

- **Resolved:**
  - 1: `languageFile` now goes through `contentFile`, with a test.
  - 2: `config/local` is read from the package root, with a test that would have failed before the fix.
  - 3: one search function, `firstRootWith`. Two thresholds remain; see finding 1.
  - 4: the repeated env check and the save on every start are gone.
  - 5: `game/README.md` is updated.
  - 6: ADR Status lines and the index are updated.
  - 8: the magic index is replaced by a vector.
  - 9: the empty check in `Lang.cpp` is dropped.
  - 10: the language folder is no longer scanned twice.
  - 11: `ProgramFiles` is cleared in the test.
  - 12: the two `Paths` test cases are added. The manual record is still open; see finding 6.
  - 14: the docs list both Program Files names.
- **Not changed, as you said:**
  - 7 is now recorded in ADR 0016's Consequences.
  - 13 and 15 are left to the owner.

## Findings

1. **Low: `game/src/Paths.h:36-37` and `game/src/Paths.h:53`. The comments say `BV2_DATA_DIR` is searched the same way as `findDataIn`, but it is not.**
   - `pathsInit` calls `firstRootWith(env, "main")`, while `findDataIn` uses `"main/textures"`.
   - ADR 0016:13 states the split correctly. The header says "as findDataIn looks" and "searched the same way", which is wrong.
   - The ADR gives no reason for the looser rule. Presumably the dedicated server's `BV2_DATA_DIR` may hold maps without textures.
   - Fix: say "the same folders, but only `main/` is required" in both header comments, and add the reason to ADR 0016:13. If there is no reason, use `findDataIn` for the env var too and drop the `sub` parameter.

2. **Low: repeated check `dataRoot() != packageRoot()` in three places.**
   - It appears at `game/src/DataDirPicker.cpp:77`, `game/src/GameVar.cpp:1249` and `game/src/Paths.cpp:342`.
   - The same unnamed idea ("the player's data is separate from the package") is now written three times, once as a stand-in for "BV2_DATA_DIR supplied it".
   - Simpler shape: one helper in `Paths`, e.g. `std::vector<std::string> contentDirs(const std::string & rel)`, returning the package folder, then the data-root folder only when it differs.
     - `GameVar::reloadLanguage` and the `config/local` loop both become one call.
     - The picker's guard then needs only an explicit `dataFromEnv()`, or a comment naming the meaning.

3. **Low: `game/src/DataDirPicker.cpp:77`. Using "data root differs from package" to mean "the env var was used" has a gap.**
   - If `BV2_DATA_DIR` points at the package folder itself (for example a dev runtime dir with real data copied in), the two roots are equal.
   - The picker then goes on, and a saved or detected folder overrides the variable the user set. That breaks the documented order, which puts `BV2_DATA_DIR` first.
   - This is rare, so Low under the AGENTS.md "ignore corner cases" rule.
   - Fix: `pathsInit` records whether the env var supplied the root, and `chooseDataDir` checks that. Or accept it and leave the comment as it is.

4. **Low: `game/src/Paths.cpp:342-343`. The added `config/local` read from the data root has no known user.**
   - Upstream BV2 data has no `config/local`. Round 1 suggested keeping it "only if wanted", and nothing records why it is wanted.
   - It adds a branch, and a stray `config/local` in a player's BV2 folder would silently override the package's.
   - Fix: drop it (one line, `dirs.push_back(g_packageRoot + "config/local")`), or keep it and give the reason in ADR 0016.

5. **Low: the docs don't match the new `config/local` rule.**
   - `config/README.md:6` and ADR 0007:15 still describe `config/local/*.cfg` without saying where it is read from.
   - ADR 0016 now says "beside the package, then beside the data root". `config/README.md` is the policy doc readers consult.
   - Fix: add "(beside the package's `main/`; ADR 0016)" to `config/README.md:6`.

6. **Low (process): `docs/roadmap/phase-c-installers-and-fixes/reviews/step1.md` does not exist yet, and step **Status** is still `IN PROGRESS`.**
   - Before the step can close, the record needs:
     - the manual acceptance evidence (picker shown, choice saved, next start silent, `execute CTF` with original data), as round-1 item 12 asked;
     - the macOS and Windows SDL re-init check (item 13);
     - the commit-granularity note (item 15);
     - each round-1 finding and how it was settled (REVIEW.md section 2, last bullet).
   - Fix: write `step1.md` with these, then set `DONE (date)` and update `possible-new-scope.md`.

7. **Nit: `game/README.md:15` vs `game/README.md:16`.**
   - Line 15 still says "Run from a runtime dir containing `main/`". The new line 16 says the client finds the player's data elsewhere.
   - Both are true, since the package's `main/` is still needed, but read together they look contradictory.
   - Fix: change line 15 to "The package (runtime dir) provides `main/` (ADR 0005)".

8. **Nit: `game/src/GameVar.cpp:1252`. A saved `languageFile "main/languages/en.lang"` now loads the package `en.lang` twice.**
   - `lang::load` reads `dir + "en.lang"` first and then reads the file again. This is harmless.
   - No fix needed.

## REVIEW.md section 2 checklist

- **Ground rules:** BV2 assets still load as `main/...` from the data root. The [GUI] change was owner-approved. No renames.
- **Tests:** every round-1 fix has a `test_paths` case:
  - `languageFile` resolution;
  - `config/local` beside the package, which would fail on the old code;
  - an invalid `BV2_DATA_DIR`;
  - `BV2_DATA_DIR` at the repository root;
  - a script that only the data has.
  - The `reloadLanguage` wiring itself is not tested directly; the test covers the `contentFile` contract it uses.
- **Docs and ADRs:** ADR 0016, 0005, 0007, the decisions index, `game/README.md` and `docs/build/README.md` are updated. `config/README.md` is missing (finding 5), and the reason for the two thresholds is missing (finding 1).
- **Mechanical vs functional:** not mixed.
- **File sizes:** fine. `Paths.cpp` is about 417 lines.
- **Round-1 findings acted on:** yes, apart from the review record (finding 6).

## Verdict

**Approved on code quality, with minor follow-ups.**
- The round-1 fixes are correct and tested, and `firstRootWith` is a real simplification.
- Nothing is High or Medium.
- Before closing the step:
  - fix the inaccurate header comments (finding 1);
  - settle the data-root `config/local` read (finding 4: keep with a reason, or drop);
  - update `config/README.md` (finding 5);
  - write `reviews/step1.md` with the manual evidence (finding 6).
- Finding 2 (a `contentDirs` helper) is an optional small refactor.
