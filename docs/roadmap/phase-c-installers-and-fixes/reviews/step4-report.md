SKILL: Launching skill: thermo-nuclear-code-quality-review

Reviewed: `git diff db8f31a..HEAD` on refactor/phase-c-step4-housekeeping (commits 77cf973..44829d0), REVIEW.md §2, the step4 scope file, and the touched files. I left the uncommitted owner files alone.

Findings

1. **Medium**: `content-seed/bv2.sql` and `game/src/Paths.cpp:285-294` (openClientDb) are now two copies of the same client schema. Nothing consumes bv2.sql any more, so it is an untested "reference" that will drift. The cleaner move is one source of truth: delete bv2.sql and treat `openClientDb` as the schema, or embed bv2.sql into the binary and exec it. `content-seed/` is outside the allowed paths, so **fix**: record it as a PNS entry in `docs/roadmap/possible-new-scope.md`.

2. **Medium**: `config/README.md:8` still says to run `sqlite3 bv2.db < content-seed/bv2.sql`. After 4.2 this step is pointless: the game creates `bv2.db` in the pref dir by itself. The file is in the allowed paths and PNS-24 is this step's item. **Fix**: say that `master.db` and `web.db` are generated from `content-seed/*.sql` by the build, and the client creates `bv2.db` in the pref dir (`game/src/Paths.cpp` openClientDb).

3. **Medium (REVIEW §2, "new behaviour has a test")**: `tools/osv-vcpkg.py` has no test. The only check is a manual, network-dependent acceptance run. The pure parts are easy to test offline: `upstream()` (GitHub regex, the Microsoft/vcpkg exclusion, sqlite tarball to `version-x.y.z`) and the hash pick in `commit()`. **Fix**: add a small ctest (Python, `tests/`) that feeds fixture SPDX dicts to `upstream()`. If you don't add one, the review record must explicitly justify skipping it.

4. **Low**: the exit-code contract in `tools/osv-vcpkg.py:7` doesn't hold. In `commit()` (line 38), `check=True` on `git ls-remote` raises CalledProcessError, and `osv()` raises on HTTP or URL errors. Both end in a traceback with exit 1, which means "vulnerabilities found" instead of 2, "could not be checked". **Fix**: drop `check=True` (an empty stdout already gives `None`, so the port goes to NOT CHECKED). In `main`, wrap the `osv(sha)` call so a failure prints NOT CHECKED and sets `unchecked`.

5. **Low**: the hardcoded `SKIP` set (`tools/osv-vcpkg.py:19`) is a list to maintain, and it breaks the weekly job (exit 2) the next time vcpkg adds a helper port (e.g. `vcpkg-tool-meson`, `vcpkg-pkgconfig-get-modules`). **Fix**: replace the set with `if port.startswith("vcpkg-"): continue`.

6. **Low**: in `game/CMakeLists.txt:163`, `target_link_libraries(bv2dedicated PRIVATE bv2server_lib Threads::Threads)` lists Threads twice. `bv2_game_target` already links `Threads::Threads` to the static lib, and a static library's private link dependencies still propagate to whatever links it. **Fix**: drop `Threads::Threads` there.

7. **Low (structural)**: `bv2server_lib` still has its usage requirements twice. `bv2_game_target` adds `src`, babonet, dko, zeven_core, sqlite and sodium PRIVATE, then lines 158-159 repeat them PUBLIC. That was tolerable while the lib was a test hook. Now that the shipped server is built from it, the cleaner shape is to give `bv2_game_target` a visibility argument, or to stop calling it for the lib and declare PUBLIC once. That deletes the duplicate list. Phase B step3 report #7 settled only the sqlite part, so this is not a re-raise. **Fix**: add `bv2_game_target(<t> <PUBLIC|PRIVATE>)`, or accept the duplication with a one-line justification in the review record.

8. **Low (accuracy)**: in db8f31a, `bv2_seed_db` was an `add_custom_target` without `ALL`, and no target depended on it. A clean build therefore likely never produced `runtime/bv2.db` before this step either. The doc claims being removed (AGENTS.md, ARCHITECTURE, the step context "generates runtime/bv2.db") overstated it. Nothing breaks, but the PNS-24 rationale and the acceptance check ("not created by a clean build") don't prove what the change did. **Fix**: say this in the review record; no code change needed.

9. **Low (docs)**: the ARCHITECTURE.md "Decisions" row (line 24) lists only ADRs 0001–0012. 0013, 0014 and 0015 exist and are cited elsewhere in the file (ADR 0013 in this very step). This step edits the neighbouring Supply-chain row and is meant to make the docs match the code. **Fix**: append 0013, 0014 and 0015 to that row.

10. **Nit**: the W1 paragraph in `docs/analysis/ALGORITHM_03-Weapons-Hitscan-Projectiles.md:238` now opens "W1 (now fixed) was" but keeps present tense: "becomes 1", "will see it flip back", "Fix: `==`". **Fix**: rewrite it in past tense and replace "Fix: `==`" with a pointer to ADR 0013.

11. **Nit**: `tools/osv-vcpkg.py:63`, `json.load(open(...))` leaves the file handle open. **Fix**: use `with open(...) as fh`.

12. **Nit**: the CI bullet in AGENTS.md:25 is now one very long sentence chain. This is optional and only if you're tightening it anyway: split "Not required" and "Scheduled (`dependency-graph`, `osv-vcpkg`)" into their own sentences.

Process notes (expected at this stage, not defects): the step Status is IN PROGRESS, and `reviews/step4.md` and `reviews/step4-report.md` don't exist yet. Commits keep mechanical and functional changes separate. All touched paths are inside Allowed paths. ARCHITECTURE.md lists the two new files and the scope file. 4.3 removes the double compile as scoped, and `bv2dedicated` keeps CONSOLE (PUBLIC), the manifest and placeholder dependencies.

Verdict: **not yet approvable, but close.** Nothing is structurally wrong: the 4.3 restructuring is the right code-judo move and the diff is small. Before closing: fix #2 (stale doc in scope), #3 (test for the new script, or a documented waiver), and #4/#5 (the weekly job's exit codes are unreliable). Record #1 as a PNS entry. #6–#12 are cheap cleanups.

## Round 2

SKILL: Launching skill: thermo-nuclear-code-quality-review

Round 2 reviewed `git diff 44829d0..HEAD` (commit 7fae6bf), with `db8f31a..HEAD` for context, against REVIEW.md §2, the step4 scope file and the round-1 report. I ran `tests/test_osv_vcpkg.py` locally and it passes ("ok: osv-vcpkg"). I modified nothing and ignored the owner's uncommitted files.

**Round-1 status**
- Resolved: #2 (config README), #3 (offline test added and wired into ctest), #4 (`check=True` dropped; OSV `OSError` now goes to NOT CHECKED with exit 2), #5 (helper ports skipped by `vcpkg-` prefix), #6 (duplicate Threads link), #7 (`bv2_game_target(<t> <visibility>)`, usage requirements declared once), #9 (ADRs 0013–0015 listed), #10 (W1 in past tense, points to ADR 0013), #11 (`with open`), #12 (CI bullet split).
- #1 is recorded as PNS-55, which is fine. #8 still has to be stated in the review record: step file line 22 still says `tools/CMakeLists.txt:10` "generates `runtime/bv2.db`".

**Findings**

1. **Low (process, REVIEW §5.3 and AGENTS commit rules)**: commit 7fae6bf puts all the round-1 fixes in one commit: Python, CMake, test, three docs and the PNS entry. Its subject is "step4 review: …", not `stepN.M: review: <summary>`, and REVIEW §5.3 says fixes go in their own commits. Nothing mechanical is mixed in, so §2's mixing rule holds, and earlier steps (step1) did the same. **Fix**: split it per task (4.1 osv-vcpkg and test; 4.3 CMake; 4.2/4.4 docs) with `step4.M: review:` subjects before the PR. Otherwise, note the deviation in the review record.

2. **Low (REVIEW §2, "a fix has a test with fix proof")**: the exit-code fix in `tools/osv-vcpkg.py:59-65` (OSV failure goes to NOT CHECKED, exit 2) and the `vcpkg-` prefix skip (`:56`) have no test. `tests/test_osv_vcpkg.py` covers only `upstream()` and the commit-SHA short path of `commit()`. The annotated-tag "last hash" pick at `:37-38`, which round 1 asked about, is also untested because it needs git. These fix code from this same step that never shipped, so a waiver is reasonable. **Fix**: either add a test that monkeypatches `osv` to raise `urllib.error.URLError`, runs `main()` on a fixture `vcpkg_installed` tree and asserts exit 2, or state the waiver in the review record.

3. **Low (docs accuracy, new this round)**: `AGENTS.md:25` lists `dependency-graph (on main)` under "Scheduled". `.github/workflows/dependency-graph.yml:3-6` runs on push to `main` and on manual dispatch, not on a schedule. **Fix**: write "On `main`: `dependency-graph`. Weekly: `osv-vcpkg` (…)", or move `dependency-graph` back to its own clause.

4. **Nit**: the comment at `game/CMakeLists.txt:138`, "(what links it needs the same)", is hard to read. **Fix**: "PUBLIC for bv2server_lib, whose dependents (bv2dedicated, tests) need the same includes and libraries".

5. **Nit**: in `tools/osv-vcpkg.py:57-59`, if `git` is missing, `commit()` raises `FileNotFoundError` outside the new `try`, which exits 1 ("vulnerabilities found"). The docstring requires git and CI always has it. Per the AGENTS corner-case rule, no change is needed; this is recorded only so it isn't raised again.

6. **Nit**: the "Tests" bullet at `AGENTS.md:24` doesn't name the Python tool tests (`lang_check`, `osv_vcpkg`). The list is illustrative, so this is optional.

**Structure**
- The `bv2_game_target(target visibility)` change is the right fix: the duplicate PUBLIC list is gone, and `bv2dedicated` is just `main.cpp` plus `bv2server_lib`.
- The curl source and definition stay PRIVATE on the lib. That is correct, because a static lib's link dependencies still propagate.
- No file crosses 1k lines, and no spaghetti was added.
- The script's control flow (one `try` around `osv`, a `vulns is None` path for unresolved commits) is direct.
- All touched paths are inside Allowed paths, and `ARCHITECTURE.md` lists the new test.

**Verdict**: approvable once the review record covers #8 (round 1), the waiver or test for #2, and the commit-split decision for #1. #3 is a one-line doc fix worth doing now. No structural blockers remain.
