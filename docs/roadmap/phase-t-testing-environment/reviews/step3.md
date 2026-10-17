# Step 3 review

Reviewer: fresh-context subagent, /thermo-nuclear-code-quality-review, 2026-10-17 (round 1 without the skill; round 2 with it, run twice)
Diff: baf8ac7..f27128a (step commits; merge of `main` at 59f81bd excluded)

## review.sh

`tools/review.sh --step docs/roadmap/phase-t-testing-environment/step3-ci-package-smoke.md` on f27128a: architecture, hygiene, content-case, original-assets, diff-rules PASS; secrets skipped locally (CI `secret-scan` runs it). The branch name doesn't match `refactor/<phase>-stepN-*` (the session may push only `claude/project-thread-9izxz9`), so CI `review` ran without the step and didn't enforce Allowed paths; the local `--step` run above is the scope evidence. `--full` not run locally (no vcpkg in the session); CI ran Debug and Release build and ctest on Windows, macOS and Linux on every head, all green.

## Findings

Raw report: [step3-report.md](step3-report.md). R2a is this thread's round 2 run, R2b the separate thread's run on the same head.

| # | Finding | Outcome |
|---|---|---|
| R1.1 | Mesa llvmpipe: no ADR, no bump policy, hash taken on first download | deferred → PNS-69; source and how the hash was taken stated in the step file |
| R1.2 | Job name carries ", not required" | fixed in 0dcafa3 |
| R1.3 | `ls -l` leftover; zip found by glob | fixed in 0dcafa3 |
| R1.4 | Failed client run leaves no diagnostics | fixed in 0dcafa3: `client.log` printed on both paths |
| R1.5 | Why Windows starts `bv2.exe` directly is undocumented | fixed in 0dcafa3 |
| R1.6 | Task 3.2 spread over several commits | rejected: the later commits are 3.2's own CI iterations, each named `step3.2` |
| R2a.1, R2b.1 | Server and master start check copied in three places | deferred → PNS-70: the shared script belongs in `tools/`, outside Allowed paths |
| R2a.3, R2b.2 | Per-OS `if` branches instead of matrix data | deferred → PNS-70 (matrix rows become the script's arguments); `GALLIUM_DRIVER` now set only by the Mesa step, fixed in f27128a |
| R2a.2 | `MSYS_NO_PATHCONV`, `taskkill` and `cygpath` unexplained | fixed in 4c7c3ff: one-line reasons |
| R2b.5 | `cygpath -m` in the client step only | rejected: kept with its reason (4c7c3ff); both forms work on the runner, a re-verify cycle buys nothing |
| R2a.4, R2b.3 | No run shows the start signal can fail | fixed: fix proof below (fb70348, reverted in 4ac95fe) |
| R2b.4 | Branch name skips the CI scope check | rejected: this session can push only its assigned branch; local `--step` output recorded above |
| R2a.5, R2b.6 | Record's diff range out of date | fixed: this record |

## Fix proof

fb70348 set `languageFile "main/languages/missing.lang"` in the smoke config, so the client stops at "Can not load language file" (`game/src/main.cpp:452`) in a message box. CI run 39079275293: both `package-smoke` jobs stayed alive for 10 s and failed on the start signal. Windows:

```
ls: cannot access 'D:/a/_temp/pref-client/console.log': No such file or directory
##[error]Process completed with exit code 2.
```

macOS failed in the same step at the same point. 4ac95fe reverts the commit.

## Not done / unsure

- `tools/review.sh --full <preset>` not run locally; CI build and ctest on all three OSes is the evidence. The owner decides whether that counts before Status is set to DONE.
- Gating `package-smoke` after 10 consecutive green runs on `main` is the owner's call after merge.
