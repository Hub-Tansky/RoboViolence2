# 0061: Refactor on main; production hotfixes on a release branch

- Status: Accepted
- Date: 2026-11-17
- Affects: `.github/workflows/build.yml` (`on.push`, `notify`), `.github/workflows/containers.yml` (`push`, `manifest`), `tools/staging/deploy.sh` (`pick_run`, `select`), [runbook: Production](../ops/runbook.md#production), rv2-docs `20-product/releases/release.md` ("Hotfix release"); ROBO-229 (owner decision 2026-11-17)

## Context
- v0.0.6 runs on production. The long-term refactor (rv2-docs `30-dev/refactor-direction-handoff.md`, ROBO-230 to ROBO-234) now lands on `main`, so `main` will hold unreleased changes for weeks.
- Production still needs small fixes. Before this ADR only `main` produced deployable images (`containers.yml` pushed to GHCR from `main`) and `deploy.sh` picked only `main` builds.

## Decision
`main` stays the one development branch; production fixes go to `release/<major>.<minor>` (first `release/0.0`, cut from tag `v0.0.6` when the first fix is needed), are tagged `v0.0.N` there and cherry-picked to `main`, and pushes to `release/**` get a `build` run, a red-run ntfy and GHCR `:<sha>` images that `deploy.sh --sha <full sha>` deploys.

## Consequences
- PR CI, `review` (against the PR base, `hygiene.yml`) and the merge tiers (ADR 0060) work unchanged for a PR into `release/0.0`. Locally pass the base: `tools/review.sh --base origin/release/0.0`.
- `:latest` and staging stay on `main`: a release-branch commit is not playtested on staging, so a HIGH fix is tested with the PR's packages.
- `deploy.sh` without `--sha` (staging cron) still picks only `main`; a release build needs its full SHA. Production's `/opt/rv2-prod/deploy.sh` must be updated from this commit before the first hotfix.
- The next release from `main` takes a new minor version (`v0.1.0`), so the two lines never share a version.
- The owner adds `refs/heads/release/**` to the ruleset "main protection" targets.
- Rejected: `main` as the production branch with the refactor on a long-lived branch. The refactor is a strangler that keeps the game playable at each step (no long-lived branch, handoff "Principles"), a long branch drifts from every fix and ends in one large merge, and CI, review, staging and the tiers all assume work merges to `main`.
