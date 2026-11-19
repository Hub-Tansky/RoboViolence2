# ROBO-253: A weekly workflow appends PR and CI numbers to a CSV in rv2-docs

- Status: Accepted
- Date: 2026-11-19
- Affects: `.github/workflows/workflow-metrics.yml`, `tools/workflow-metrics.py`, `docs/build/README.md`; ROBO-253 (workflow audit WP-15 part 4, approved 2026-11-11)

## Context
- The workflow audit (rv2-docs `30-dev/workflow-audit-handoff.md`) counted merges, time to merge, review fix commits, merges from `main` and failed or re-run `build` runs by hand; WP-19 re-measures them after the packages land, and that needs a weekly series.
- Every number comes from the public REST API of this repo (pulls, PR commits, `build` workflow runs), readable with `GITHUB_TOKEN`. The vault is private and already written by `backlog-move.yml` with `SYMBOLS_TOKEN` (ADR ROBO-249).

## Decision
Every Monday (and by hand, optionally for an earlier week) `workflow-metrics.yml` runs `tools/workflow-metrics.py` for the Monday-to-Monday UTC week that just ended and appends one row to `30-dev/workflow-metrics.csv` in rv2-docs, pushed as Hub-Tansky with the same clone, rebase and retry steps as `backlog-move.yml`. The columns are listed in the tool's docstring. A row whose `week_start` is already in the file is not added again, a week that has not ended is refused, and runs queue one after another. `main_merges_per_pr` counts every merge commit in a PR; here those are merges from `main`.

## Consequences
- A CSV in the vault, readable in Obsidian or a spreadsheet; WP-19 compares weeks before and after the audit.
- A missing `SYMBOLS_TOKEN` fails the run; the token expiry check (ADR ROBO-252) covers this token.
- `build` runs count by the conclusion of their latest attempt; a run started in one week and re-run in the next counts in the first.
- Rejected: a CSV in this repo (a weekly commit to `main` through a PR); GitHub Insights (no time-to-merge percentiles, no review commit count).
