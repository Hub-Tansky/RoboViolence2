# ROBO-249: A merged PR's backlog item moves by workflow

- Status: Accepted
- Date: 2026-11-18
- Affects: `.github/workflows/backlog-move.yml`, `tools/backlog-move.py`, [AGENTS.md](../../AGENTS.md) "Work tracking"; ROBO-249 (workflow audit WP-14, approved 2026-11-11, decision 7)

## Context
- After every merge an agent moved the PR's item in rv2-docs by hand (`git mv`, `status`, `updated`, push to `main`). Sessions forgot it, or two sessions pushed to rv2-docs `main` at once.
- Every ready PR names its item in the description's `Item` row; `tools/check-review-record.py` already parses that row (`ITEM_ROW`).
- Criteria starting `(post-deploy)` may stay unticked at merge; the item then stays in `30-inprogress/` as Testing (ADR 0057, rv2-docs AGENTS.md § Backlog).
- The owner allowed automation to move items (workflow audit decision 7, 2026-11-11).

## Decision
On `pull_request: closed` with `merged == true`, into `main` or `release/**`, from this repository only, `backlog-move.yml` checks out the target branch, clones rv2-docs and runs `tools/backlog-move.py`. It takes the first `ROBO-NNN` in the Item row. An unticked `- [ ] (post-deploy)` criterion in the item's Acceptance criteria section gives `status: Testing` in `30-inprogress/`; anything else gives `status: Done` in `90-done/`. It sets `updated:` to the day of the run, commits as Hub-Tansky `backlog: ROBO-NNN -> <status> (PR #n)` and pushes, rebasing up to five times. An item already Done in `90-done/` is never moved back. The workflow has no concurrency group, since one would cancel a pending run when a third merge queues.

The token is `SYMBOLS_TOKEN` (fine-grained, Contents write on rv2-docs only), shared with the release symbols upload; the owner chose reuse over a separate secret on 2026-11-18.

## Consequences
- Agents no longer move an item after a merge; they still move it into `30-inprogress/` when work starts, and the owner still ticks `(post-deploy)` criteria and moves Testing items to Done.
- No ROBO id in the Item row: nothing to do. No item file, two item files, no frontmatter or no token: a warning in the run, no commit, never a red merge; the item is then moved by hand. A merged PR from a fork (no secrets) or into another branch is skipped; its item is moved by hand.
- An item already at its target status and folder is left alone, so a second PR for the same item adds no commit.
- The workflow is not a required check; a failed push (rebase conflict) shows as a failed run, the merge stands.
- The PR body comes in through `env`, never into the shell script; the tool runs from the target branch, not the PR head.
- Rejected: moving items in the review gate before merge (the merge may never happen); a scheduled sweep over merged PRs (late, and has to remember what it already moved).
