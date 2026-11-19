# ROBO-251-2: The owner starts CI on the bot's vcpkg PR by closing and reopening it

- Status: Accepted
- Date: 2026-11-19
- Affects: `.github/workflows/vcpkg-baseline.yml` (PR body to-do), `docs/build/README.md`; amends [ROBO-251](ROBO-251-monthly-vcpkg-baseline-pr.md)

## Context
- ADR ROBO-251 said the bot's draft PR gets CI once someone pushes to its branch. The first one, PR #193, changes only `vcpkg.json` and needed nothing more: there was no real change to push.
- Agents may not push an empty commit, or close and reopen a PR, only to start CI. The owner closed and reopened #193; `build`, `hygiene` and `secret-scan` ran (their `pull_request` triggers include `reopened`), and it merged.

## Decision
The PR body's to-do and `docs/build/README.md` say: the owner closes and reopens the bot's vcpkg PR to start CI; an agent asks the owner for it. A push of a real change (for example `docs/build/linux.md` versions) also starts CI.

## Consequences
- Each monthly PR needs one owner action before it can merge; it is MEDIUM tier (`vcpkg.json`), so an agent merges it once green.
- Rejected: an empty commit pushed by an agent (forbidden as CI kicking); a personal token for the bot so its PR starts CI by itself (no new tokens without the owner).
