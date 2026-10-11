# Step 7 review

Reviewer: fresh-context subagent, /thermo-nuclear-code-quality-review, 2026-10-11
Diff: 001dfaa...c7624d1
Skill evidence: `SKILL: Launching skill: thermo-nuclear-code-quality-review` (raw report: [step7-report.md](step7-report.md))

## review.sh

`tools/review.sh --full macos-arm64` and `--full macos-arm64-asan` at 64d5fef: all checks PASS, 12/12 ctests.

## Findings

| # | Finding | Outcome |
|---|---|---|
| 1 | R15: `reinit` (every map change) zeroed `pingSum` but kept `pingLog`, so the average drifted to 0 | fixed in e4f3d31; `r15` runs a second window after `reinit` |
| 2 | R7 test overflows under ASan until the R31 commit; R31 labelled 7.1 after 7.4; 7.4 not a task | rejected: pushed history not rewritten (as step 6 #14) |
| 3 | R31 row without its commit | fixed in this commit |
| 4 | `balance()`: `carriedFlag` equals `toTeam`; team-changed check always true | fixed in 64d5fef |
| 5 | R10 interior condition written twice | fixed in 64d5fef |
| 6 | `test_babonet.cpp` lacks `<algorithm>`; port 10208 has no resource lock | include fixed in 64d5fef; lock not added (ctest runs serially) |
| 7 | `r12` comment claims the take branch is tested | fixed in 64d5fef (comment) |
| 8 | An R8 move refused by approval still counts toward the cycle | accepted, stated in the PR text (old behaviour) |

## Fix proof

- R15 after `reinit` (Player.cpp reverted): `FAIL tests/test_crash_fixes.cpp:223: p->avgPing == 10`.
- The other defects: step notes.

## Not done / unsure

- R9, R10 and R13 fix proofs were run under macOS ASan only (libc++ container annotations); not repeated on Linux.
- `babonet_large_message` on Windows CI with R28: watched in this PR's CI.
