# 0040: Build branches through their PR only, and re-run a build whose job never got a runner

- Status: Accepted
- Date: 2026-10-29
- Affects: `.github/workflows/build.yml`, `.github/workflows/ci-retry.yml`, PR #122

## Context
- `build (macos-arm64)` and `package-smoke (macos-arm64)` were cancelled 23 times from 2026-10-27 to 2026-10-29. Each had no runner and no steps, was cancelled 15 min after it was queued (e.g. PR #119, run 40926542252), and carries GitHub's annotations "The job was not acquired by Runner of type hosted even after multiple attempts" and "Due to capacity constraints, jobs targeting macOS arm64 runners may experience longer queue times."
- No workflow sets `timeout-minutes`. A macOS build that gets a runner takes 4.5 to 9 min; its queue wait varied from 0 to 14 min.
- Every commit on a PR branch started a `push` and a `pull_request` build run (different concurrency groups, `.github/workflows/build.yml:7`): two macOS build jobs and two macOS package-smoke jobs for one commit.

## Decision
`build` runs on pull requests, pushes to `main` and `v*` tags. `ci-retry` re-runs the failed jobs of a completed `build` run (attempts 1 and 2 only) when one of its cancelled jobs has the "not acquired by Runner" annotation and its commit is still the head of its branch.

## Consequences
- Half the macOS jobs per PR commit, and a capacity drop no longer needs a manual re-run. A real build failure still fails: it is only re-run when a runner-less job sits in the same run, and fails again.
- A branch without a PR gets no `build` run; open a (draft) PR for CI.
- `ci-retry` fails on an API error, except the branch-head lookup: its failure is read as a deleted (merged) branch and skips the re-run.
- `workflow_run` workflows run from `main`'s copy, so `ci-retry` only works after merge. If GitHub does not start `workflow_run` for a re-run made with `GITHUB_TOKEN`, a run gets one automatic retry, not two.
- Rejected: raising `timeout-minutes` (the cancelled jobs never started; the timeout counts running time only). Paid or larger runners and external CI (cost; not approved). `macos-15-intel` (x86_64 package, not the shipped arm64 one). Merging `package-smoke` into `build` (one fewer macOS queue wait, but a larger change to the release jobs).
