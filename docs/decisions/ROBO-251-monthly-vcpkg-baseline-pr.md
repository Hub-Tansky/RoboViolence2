# ROBO-251: A monthly workflow opens the vcpkg baseline PR

- Status: Accepted
- Date: 2026-11-19
- Affects: `.github/workflows/vcpkg-baseline.yml`, `tools/vcpkg-ports-diff.py`, `docs/build/README.md`, ARCHITECTURE.md "Supply chain"; ROBO-251 (workflow audit WP-15 part 2, approved 2026-11-11)

## Context
- The vcpkg ports are pinned by `builtin-baseline` in `vcpkg.json`. Dependabot can't update vcpkg; `docs/build/README.md` asked for a monthly bump by hand, and nothing reminded anyone.
- `osv-vcpkg` (weekly) checks the installed ports against OSV by upstream commit (`tools/osv-vcpkg.py`).

## Decision
On the 1st of each month (and by hand), `vcpkg-baseline.yml` reads the newest `microsoft/vcpkg` `master` commit. If it differs from the baseline and the branch `bot/vcpkg-baseline-<YYYY-MM>` does not exist, it moves `builtin-baseline`, installs the ports on Linux, and opens a draft PR from that branch. The body lists every installed port's version at both commits (`tools/vcpkg-ports-diff.py`, changed ports first) and the `osv-vcpkg` result at the new baseline. The job that installs the new ports has a read-only token and no stored credentials, and its own vcpkg binary cache; a second job, which builds nothing, commits the changed `vcpkg.json` as Hub-Tansky (AGENTS.md "Git identity"), pushes it and opens the PR with its `GITHUB_TOKEN`.

## Consequences
- The bump arrives as a PR to review, not as a reminder; a vulnerable port shows in its body.
- A PR opened with `GITHUB_TOKEN` starts no workflows, so it has no CI until someone pushes to its branch. The body's to-do list says so, together with the `docs/build/linux.md` versions to update.
- The author is a bot, so the review gate skips the review record (`REVIEW_BOT`); the person who takes it over still reviews the port changes.
- Opening the PR needs the repository setting "Allow GitHub Actions to create and approve pull requests". Without it the run pushes the branch and fails with that hint.
- One branch per month: once the branch exists (PR open, handled, or failed to open), that month opens no second one; a run in a new month opens the next.
- Portfiles at an unreviewed vcpkg commit run only in the read-only job; a compromised upstream commit can't push or open PRs from there.
- Rejected: bumping in the weekly `osv-vcpkg` run (a weekly PR is noise); bumping only when OSV reports a port (ports would drift for months).
