# Step 1 raw reviewer reports

## Round 1

Not kept. The reviewer's session ended before the evidence rule existed; its findings are in [step1.md](step1.md).

## Round 2

SKILL: Launching skill: thermo-nuclear-code-quality-review

I reviewed `git diff b01f7a3..HEAD` (head 4ef26cd, 19 commits) read-only. `tools/review.sh` in fast mode passes at HEAD: architecture, hygiene, content-case, original-assets, secrets and diff-rules. I did not run `--full`. Every relative link in `docs/build/*.md` and in the README's `docs/build/` links resolves. The code is in good shape; no structural problems:
- `make-package.py` is a flat 60-line script with an explicit list of required files.
- The run scripts are minimal.
- The CI `package-check` job uses pipefail and checks the documented packages.

The problems are in the process records and the planning docs.

| # | Severity | Finding (file:line) | Suggested change |
|---|---|---|---|
| 1 | High | `reviews/step1.md:3-4,8-10`: the record covers only `b01f7a3..a350528`, and `review.sh --full` is recorded only "before the fixes" at a350528. Commits e8d9d7a (all four guides rewritten), 857e42e (step 2 task added, PNS-25) and 4ef26cd (Status DONE) came after that review. DONE was set before those changes were reviewed or re-run through `--full`, which goes against REVIEW.md §4 ("marking a step DONE … before the review has run") and §5.1. | Revert Status to IN PROGRESS until this review's findings are handled. Re-run `tools/review.sh --full macos-arm64` at the final head and paste the result. Extend the record's Diff line to the final head, or add a second review section for e8d9d7a..4ef26cd. |
| 2 | Medium | `reviews/step1.md:3`: "Reviewer: fresh-context subagent, /thermo-nuclear-code-quality-review" has no evidence the skill actually loaded (no skill result line, no reviewer output). REVIEW.md §5.2 says to stop if the skill isn't available, so the record should show proof. | Record the reviewer's `SKILL:` line, e.g. "Skill: `Launching skill: thermo-nuclear-code-quality-review`", for both review rounds. |
| 3 | Medium | Step file Acceptance checks vs `reviews/step1.md:39-49`: (a) "owner follows `docs/build/macos.md` from scratch and gets a running client and server": the owner ran only the CI package, not the build-from-source path in macos.md. (b) "The Markdown link check finds no broken link": no link-check tool exists in `tools/` or CI, and the record never says one ran. The step is DONE with two acceptance checks unproven. | Under **Not done / unsure**, list "build-from-source path of macos.md not run by the owner" and "link check: manual check only (no tool)". Or have the owner do the source build. The links do resolve today (I checked). |
| 4 | Medium | `step2-cross-os-playtest.md` Allowed paths says `game/src/**` (blocking fixes only), but the new task 2.4 changes `game/src/Client.cpp:444` for a non-blocking owner request. It is also player-visible behaviour, and the step's Out of scope excludes "anything that changes the interface". `tools/review.sh` scope can't catch this; the reviewer of step 2 would have to. | Add the owner approval next to the path, e.g. "`game/src/Client.cpp` (owner-approved 2026-10-08: task 2.4)". Or move task 2.4 to its own small step/PNS entry. |
| 5 | Low | `step2-cross-os-playtest.md` task 2.4 says "create the folder if needed". `bv2::userFile` already creates parent dirs (`game/src/Paths.cpp:183-189`), and the save paths already go to the pref dir (`game/src/screengrab.cpp:70-86`). The task is only deleting the `#ifdef` at `Client.cpp:444,455`. Its "test" is manual, while REVIEW.md §2 asks new behaviour to have a test. | Reword to "remove the `#ifdef BV2_PLATFORM_WINDOWS` at `Client.cpp:444-455`; paths already use `bv2::userFile`". State that the proof is manual because it needs a GL context, or add a unit test of the path/BMP writer. Also have the task check that on HiDPI, `SaveScreenGrab` reads the framebuffer at `dkwGetResolution()` size (points vs pixels on Retina). |
| 6 | Low | `possible-new-scope.md` PNS-25 says the colour depth "changes nothing… SDL3 and modern OpenGL pick the colour depth themselves". The value is used: `engine/zeven/src/dkw.cpp:352-355` sets RGB 5/6/5 and alpha 0 for 16-bit. The option's tooltip says it needs a restart (`COption.cpp:142`), so the owner's observation may predate a restart. | Cite `dkw.cpp:352-355` and state that the sizes are minimums the driver may exceed. Note whether the owner tested after a restart. Keep the [GUI] decision with the owner. |
| 7 | Low | PNS-18 play-test note cites `glPolygonMode` only at `Player.cpp:539, 553`. The same calls are also at `Player.cpp:581, 588`. | List all four call sites, or write "`Player.cpp` (4 calls)". |
| 8 | Low | `packaging/scripts/run-server.sh:4` and `run-server.cmd:4`: the CTF default applies only when there are no arguments. `./run-server.sh --config x.cfg` therefore starts no game: `main.cpp:370-380` skips `--config <file>` and finds no launch-script argument. The docs table (`docs/build/README.md` "Arguments: launch script name, default `CTF`") implies the default always applies. | Either document "pass the launch script explicitly when you add `--config`", or default to CTF when no non-`--config` argument is given. Documenting is enough for a play-test package. |
| 9 | Low | `docs/build/macos.md` "Use a downloaded package" step 3 and `windows.md` step 2 start `run-master` first, but the master does nothing for the client unless `BV2_MASTER_SERVERS=127.0.0.1:10207` is set (said only in `README.md`). A first-time player starts a useless process. | Drop `run-master` from the minimal sequence, or add "(only needed for the server browser; see README)". |
| 10 | Low | `docs/build/README.md` "Game data (required)" and each guide's "required, once" conflict with `AGENTS.md` Assets ("Without it, CI and contributors use generated placeholders") and with the code (`Paths.cpp:144-176` falls back to the package/runtime `main/`). It conflicts for contributors without the data, who can still build and run `ctest`. Also, `ls "$BV2_DATA_DIR/main/maps"` fails for a valid `BV2_DATA_DIR=…/main`, which the code accepts (`Paths.cpp:148-153`). | Phrase it as "required to play; building and tests work without it". Optionally note that pointing at `main/` itself also works. |
| 11 | Low | `reviews/step1.md` Fix proof for findings 3 and 14: "verified by this PR's `package-check` run" gives no run ID or URL, so it can't be checked later. | Add the Actions run URL or ID. |

**Repeats of earlier findings:** none re-raised. Findings 1-3 are new process gaps caused by the commits after the previous review (e8d9d7a, 857e42e, 4ef26cd). Finding 8 is next to earlier finding 2 (argument passing) but is a different defect: the default is dropped when only `--config` is given. I didn't re-raise the earlier review's items 4 (two `main/` copies), 6 (ADR → PNS-23) and 13 (`package-check` waits for the matrix).

**Not verifiable from the diff:**
- Whether the original BaboViolent 2 data's `main/` contains `LaunchScript/CTF.cfg`, `languages/en.lang` and `bv2.example.cfg`. With `BV2_DATA_DIR` set, the data root and current directory switch entirely to that folder (`Paths.cpp:166-176`), and `execute CTF` reads `main/LaunchScript/CTF.cfg` (`Console.cpp:668`). The owner's successful run suggests the first two exist; `bv2.example.cfg` defaults probably won't be there.
- CI results at 4ef26cd (build, `package-check`) and artifact contents on Windows and macOS.
- The Windows guide and package (never run on Windows; recorded as step 2).
- That `setx` reaches double-clicked `.cmd` files without a re-login.
- `review.sh --full` at the final head.
- Whether the earlier reviewer actually loaded the skill (finding 2).
