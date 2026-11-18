# 0067: A docs-only PR skips the build steps; its required checks still report success

- Status: Accepted
- Date: 2026-11-18
- Affects: `.github/workflows/build.yml`, `.github/workflows/containers.yml`, `.github/actions/docs-only/`, `tools/check-docs-only.py`; ROBO-246 (workflow audit WP-11, approved 2026-11-11)

## Context
- `build.yml` and `containers.yml` run in full on every PR: a README-only PR (#141) built on three OSes and two container runners.
- `paths-ignore` cannot skip them: a required check whose workflow never starts never reports, and the PR waits forever. A matrix job skipped with a job-level `if:` reports under its unexpanded name (`build (${{ matrix.preset }})`), so `build (linux-x64)` still never reports.

## Decision
Each job in `build.yml` that builds (`build`, `smoke`, `tidy`) and `containers.yml`'s `images` checks out with `fetch-depth: 2` and runs `./.github/actions/docs-only` first. It runs `tools/check-docs-only.py`, which sets `DOCS_ONLY=true` when the run is a `pull_request` and every path the merge commit changes against its base is the review gate's docs tier (`review_tier()` in `tools/check-review-record.py`: `docs/` or `.md`, not `AGENTS.md`, `REVIEW.md`, `.claude/`, `.github/` or `docs/agents/`), except `docs/build/linux.md`, whose package list `package-check` checks. Every later step has `if: env.DOCS_ONLY != 'true'`, so the job ends green under its real name. Jobs that need `build`'s packages (`release`, `package-check`, `deb-check`, `package-smoke`, none required) skip at job level on `needs.build.outputs.docs`. Push (`main`, `release/*`, tags), schedule and `workflow_dispatch` always run everything; any error in the check runs everything.

## Consequences
- A docs-only PR's required checks finish in about a minute: each runner still starts, checks out and runs the check.
- A new step in a gated job needs the `if:`; ctest `docs_only` fails when one runs on a docs-only PR without it. A doc that a skipped job reads joins `CHECKED_DOCS` in `tools/check-docs-only.py`.
- A PR can change `tools/check-docs-only.py`, `review_tier()` or the workflows to skip its own build, as it already could by editing `build.yml`. All of them are guard files (`tools/check-*`, `.github/`), so such a PR is HIGH tier: Risk review and the owner's "merge".
- Every `build` matrix leg writes `outputs.docs`; the legs run the same tool on the same commit, and an error on one leg gives `false` there, so at worst a docs-only PR runs a package job that fails red.
- Rejected: a separate `changes` job that every job waits on (delays every build); `paths-ignore` (required checks never report).
