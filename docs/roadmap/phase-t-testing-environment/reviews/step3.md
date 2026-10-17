# Step 3 review

Reviewer: fresh-context subagent, 2026-10-17, round 1 without the skill (see Not done)
Diff: baf8ac7..c48e65c

## review.sh

`tools/review.sh --step docs/roadmap/phase-t-testing-environment/step3-ci-package-smoke.md`: architecture, hygiene, content-case, original-assets, diff-rules PASS; secrets skipped locally (CI `secret-scan`). `--full` not run locally (no vcpkg in the session); CI on c48e65c ran Debug and Release build and ctest on Windows, macOS and Linux, all green.

## Findings

Raw report: [step3-report.md](step3-report.md).

| # | Finding | Outcome |
|---|---|---|
| 1 | Mesa llvmpipe: no ADR, no bump policy, hash taken on first download | deferred → PNS-69; source and how the hash was taken stated in the step file |
| 2 | Job name carries ", not required" | fixed: name is `package-smoke (<preset>)` |
| 3 | `ls -l` leftover; zip found by glob | fixed: exact file name |
| 4 | Failed client run leaves no diagnostics | fixed: client output to `client.log`, printed on both paths |
| 5 | Why Windows starts `bv2.exe` directly is undocumented | fixed: comment |
| 6 | Task 3.2 spread over four commits | rejected: the later commits are 3.2's own CI iterations, each named `step3.2`; squashing rewrites a pushed branch for no reader gain |

## Fix proof

CI job, no unit test. Evidence that the start signal fails without a working init: none yet (see Not done).

## Not done / unsure

- The `thermo-nuclear-code-quality-review` skill can only be invoked by the owner (`disable-model-invocation`), so this round is not the DONE-gate review. The owner runs `/thermo-nuclear-code-quality-review` on this step; the step stays IN PROGRESS until then.
- `tools/review.sh --full <preset>` not run locally.
- Gating on 10 consecutive green runs on `main` is the owner's call after merge.
