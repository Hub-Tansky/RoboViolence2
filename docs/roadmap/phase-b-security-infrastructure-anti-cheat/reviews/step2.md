# Step 2 review

Reviewer: fresh-context subagent, /thermo-nuclear-code-quality-review, 2026-10-10
Diff: origin/main...963baf5 (the step's own changes, without the merge from main)
Skill evidence: `SKILL: Launching skill: thermo-nuclear-code-quality-review` (raw report: [step2-report.md](step2-report.md))

## review.sh

`tools/review.sh --full macos-arm64` at 963baf5: architecture, hygiene, content-case, original-assets, secrets, diff-rules, configure, build, ctest all PASS.

## Findings

| # | Finding | Outcome |
|---|---|---|
| 1 | Goal, Deliverables and Acceptance checks still demand the checks the owner postponed | fixed in this round's commit (amended to the owner's deferral) |
| 2 | Tasks 2.2, 2.5 and 2.6 don't show they are postponed | fixed in this round's commit |
| 3 | Phase README and PNS-1 still tie the manual-test baseline to step 2; PNS-31 has no deadline | fixed in this round's commit (README row and "done when", PNS-1 Status, PNS-31: Results rows before step 4) |
| 4 | No unit test for the BMP writer (task 2.4) | rejected: the step adds no BMP code, only removes a Windows-only `#ifdef`; the owner verified the files on macOS and Linux; minimal-change rule |
| 5 | Task 2.5 fix has no test and its manual proof is deferred | deferred → PNS-31 (see Not done) |
| 6 | Duplicated zoom blocks in `Map::update` | rejected: the duplication is upstream; minimal-change rule for a blocking-fix step |
| 7 | `wheelUsedByUi()` null-checks `console`, which always exists in the client | fixed in 5c69de1 |
| 8 | Owner's `docs/build/linux.md` edits: H1 sections, trailing spaces, wording | rejected: owner-authored text; layout is the owner's call |
| 9 | PNS-26 points at step 2's cross-OS game | fixed in this round's commit (PNS-31) |
| 10 | Commit subjects `step2:` not `step2.M:`; d2ed299 says PNS-27 for what is now PNS-30 | rejected: pushed history not rewritten (as step 2a). d2ed299's PNS-27 was renumbered PNS-30 when merging main (main had PNS-27, 28; PNS-29 is on the step 2b branch) |

## Fix proof

- 2.4 (P/L keys): manual, needs a GL context. Owner on macOS 27.0.1 (56a0d08) and Linux Mint 22.3 (CI package): P and L save `.bmp`/`.txt` in the pref dir's `screenshots/`. Before the fix, the keys did nothing on both (owner report, step file Context).
- 2.5 and 7: no headless seam (needs a GL window and menu state). Build and ctest pass.

## Not done / unsure

- Task 2.5's wheel fix is unverified on Linux: re-test → PNS-31.
- Task 2.2 cross-OS game, Windows client run, Results rows → PNS-31 (owner, 2026-10-10).
