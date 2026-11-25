# ROBO-268: macOS builds run on main, release branches, tags and nightly, not on PRs

- Status: Accepted
- Date: 2026-11-25
- Affects: `.github/workflows/build.yml` (`build`, `package-smoke`, `release` file check, `schedule`), `AGENTS.md`, `docs/build/README.md`, ruleset "main protection" (owner); extends [ADR 0041](0041-macos-ci-runner-not-acquired-retry.md); ROBO-268

## Context
- Hosted macOS arm64 jobs often get no runner and are cancelled after 15 min (ADR 0041); `ci-retry` re-runs them at most twice.
- From 2026-11-11 to 2026-11-24, 18 of 19 failed `build` runs on main were macOS or packaging jobs, none a clash of two PRs. PR #199 waited about 4 hours (7 attempts) for a macOS runner and was merged with the owner's admin bypass.
- The owner cancelled the merge queue (WP-17, organization transfer O7) on 2026-11-25 and approved running macOS only on main and nightly.

## Decision
- On `pull_request`, the `build` and `package-smoke` matrices leave out `macos-arm64`; the `release` job's file check leaves out the macOS files. Pushes to `main`, `release/**` and `v*` tags build all three OSes as before.
- A nightly `schedule` run on `main` (01:23 UTC) builds everything, so a day whose main runs never got a macOS runner still gets a macOS result. Its failure pings ntfy through the existing `notify` job; `ci-retry` re-runs its runner-less jobs.
- The owner removes `build (macos-arm64)` from the required checks; with it required, a PR would wait for a check that never runs.

## Consequences
- PRs no longer wait on Apple runners; macOS-only breakage (Cocoa, Metal, audio, `.dmg`) shows up on main after merge or the next night, and is fixed by a follow-up PR. Before merging a PR that touches platform code, the owner can build it on the Mac (`cmake --preset macos-arm64`, `docs/build/macos.md`).
- One full nightly run more per day; public-repo Actions minutes are free.
- The playtest agent takes successful `push` runs on main only; a main run whose macOS job never started gives it nothing, as before.
- Rejected: a merge queue through an organization (O7, cancelled: the clash it prevents did not happen, and a required macOS job would stall the whole queue); a placeholder `build (macos-arm64)` job reporting success on PRs (a green macOS check that built nothing); a self-hosted runner on the owner's Mac (a public repo's PRs could run code on it).
