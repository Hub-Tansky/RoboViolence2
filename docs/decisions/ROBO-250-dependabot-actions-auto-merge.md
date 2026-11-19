# ROBO-250: Dependabot's GitHub Actions minor and patch updates merge themselves

- Status: Accepted
- Date: 2026-11-18
- Affects: `.github/workflows/dependabot-automerge.yml`, `.github/dependabot.yml`, ADR 0060 (tiered merge), [AGENTS.md](../../AGENTS.md) "Agent rules"; ROBO-250 (workflow audit WP-15 part 1, approved 2026-11-11)

## Context
- Dependabot opens weekly `github-actions` PRs (`.github/dependabot.yml`); each waited for the owner to merge it by hand (#110, #143).
- Such a PR changes `.github/`, so `tools/merge-risk.py` rates it HIGH, which needs the owner's "merge" (ADR 0060). The owner approved auto-merge for Actions minor and patch updates in the workflow audit (WP-15).
- The review gate already skips the review record for bot PRs (`REVIEW_BOT`), and the repository allows auto-merge.

## Decision
`dependabot-automerge.yml` runs on every event of a PR that Dependabot opened. It reads `dependabot/fetch-metadata` (pinned by commit). When Dependabot itself pushed and the update is ecosystem `github_actions` with update type semver minor or patch, it runs `gh pr merge --auto --squash` with the job's `GITHUB_TOKEN`; GitHub then merges once the required checks pass. Otherwise (a major, a Docker update, a push by anyone else) it turns auto-merge off if an earlier run turned it on. Dependabot waits 7 days after a release before opening its PR (`cooldown` in `.github/dependabot.yml`). This is the one exception to ADR 0060's HIGH tier.

## Consequences
- Actions minor and patch bumps land without the owner; a red required check keeps them open, and a major still needs the owner.
- A merge made with `GITHUB_TOKEN` starts no `push` workflows: `build`, `containers` and `dependency-graph` don't run on `main` for that commit. The next human merge runs them; an Actions version bump changes no game code or image.
- A person pushing to Dependabot's branch turns auto-merge off; the owner merges that PR.
- A compromised Actions release passes the checks it runs in. The 7-day cooldown is the window in which such a release is usually pulled; the action then runs with repository secrets at the next merge that triggers `push` workflows.
- `tests/test_dependabot_automerge.py` evaluates the `if:` conditions for each case and pins the trigger, the job's permissions and the pinned metadata action, so a widened workflow fails ctest.
- Rejected: a personal token so `main` runs `push` workflows after these merges (no new tokens without the owner).
