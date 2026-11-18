# ROBO-250: Dependabot's GitHub Actions minor and patch updates merge themselves

- Status: Accepted
- Date: 2026-11-18
- Affects: `.github/workflows/dependabot-automerge.yml`, ADR 0060 (tiered merge); ROBO-250 (workflow audit WP-15 part 1, approved 2026-11-11)

## Context
- Dependabot opens weekly `github-actions` PRs (`.github/dependabot.yml`); each waited for the owner to merge it by hand (#110, #143).
- Such a PR changes `.github/`, so `tools/merge-risk.py` rates it HIGH, which needs the owner's "merge" (ADR 0060). The owner approved auto-merge for Actions minor and patch updates in the workflow audit (WP-15).
- The review gate already skips the review record for bot PRs (`REVIEW_BOT`), and the repository allows auto-merge.

## Decision
`dependabot-automerge.yml` runs on Dependabot's own PR events (`pull_request.user.login` and `github.actor` both `dependabot[bot]`). It reads `dependabot/fetch-metadata`; for ecosystem `github_actions` with update type semver minor or patch it runs `gh pr merge --auto --squash` with the job's `GITHUB_TOKEN`. GitHub then merges once the required checks pass. Majors, Docker updates and every other PR are left alone. This is the one exception to ADR 0060's HIGH tier.

## Consequences
- Actions minor and patch bumps land without the owner; a red required check keeps them open, and a major still needs the owner.
- A merge made with `GITHUB_TOKEN` starts no `push` workflows: `build`, `containers` and `dependency-graph` don't run on `main` for that commit. The next human merge runs them; an Actions version bump changes no game code or image.
- A person pushing to Dependabot's branch gets no auto-merge from that run, but auto-merge turned on earlier can stay on; turn it off on the PR if the push needs review.
- `tests/test_dependabot_automerge.py` evaluates the two `if:` conditions for each case, so a widened condition fails ctest.
- Rejected: a personal token so `main` runs `push` workflows after these merges (no new tokens without the owner).
