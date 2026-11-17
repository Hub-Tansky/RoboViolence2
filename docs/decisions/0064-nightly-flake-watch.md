# 0064: A nightly workflow repeats the e2e and bots tests and files one issue per flaky test

- Status: Accepted
- Date: 2026-11-17
- Affects: `.github/workflows/flake-watch.yml`, `tools/flake-issues.py`, [AGENTS.md](../../AGENTS.md) "Conventions and gotchas"; ROBO-237 (workflow audit WP-09, approved 2026-11-11)

## Context
- `e2e_weapon_choice` (ROBO-201) and `bots` (ROBO-196) failed on `main` and on five PRs (#148, #150, #152, #153, #154); four of the last 20 `build` runs before the audit were re-runs.
- A flaky test shows up only on someone's PR, where it costs a re-run and looks like the PR's fault.

## Decision
`flake-watch.yml` runs nightly and on `workflow_dispatch` on Linux: it builds `linux-x64-asan` and runs the `e2e_*` and `bots` tests with `ctest --repeat until-fail:10`. On a failure `tools/flake-issues.py` comments on the open issue titled "Flaky test: <name>" (label `flaky-test`) or opens one, with the failing run's output, and the job pings the owner's ntfy topic like `build.yml`'s `notify` job.

## Consequences
- One open issue per flaky test collects every failing night; closing it after a fix starts a new one on the next failure.
- A test waits for a condition with a deadline, never a fixed time (AGENTS.md).
- The run takes up to three hours of one Linux runner per night and is not required.
- Rejected: repeating tests on every PR (slower PRs, and the PR author still gets the blame); retrying failed tests automatically (hides the flake).
