# 0057: The review record is keyed to the diff's patch-id; post-deploy criteria don't block

- Status: Accepted
- Date: 2026-11-17
- Affects: `tools/check-review-record.py` (`patch_id`, `check_record`, `check_spec_matrix`), `REVIEW.md` section 5 and the record template, `.claude/skills/rv2-code-review/SKILL.md`, `tests/test_review_record.py`; amends [ADR 0047](0047-two-axis-review-replaces-thermo-nuclear.md); ROBO-224 (workflow audit WP-03, owner approved 2026-11-11)

## Context
- The gate failed when the PR head differed from the head in the record's `Diff:` line. A merge from `main` that left the diff unchanged forced a new review round: 32 such merges in 25 PRs (audit 2026-11-11).
- The Spec matrix accepted only `pass`, `partial` and `fail`. PR #152 waited 2 h 14 min for the owner because one criterion could only be checked on a deployed server.

## Decision
- The record carries `Patch-id: <40 hex>` under `Diff:` (`git diff --text <merge-base>...<head> | git patch-id --stable`). The record is current when the PR head starts with the recorded head, or when the patch-id of `git diff --text $REVIEW_BASE...$REVIEW_HEAD` equals the recorded one. Otherwise the gate fails as before.
- Spec status `post-deploy` passes when the criterion cell starts with `(post-deploy)`; any other use fails. The scope item sets the tag, which the owner approves; the item stays in Testing until the owner ticks the criterion.

## Consequences
- A merge from `main` with no conflict in the PR's files needs no new review round. `git patch-id` ignores line numbers and whitespace but not context lines, so a merge that changes lines next to the PR's hunks still asks for a round.
- A record without `Patch-id:` behaves as before.
- Known limit: the gate cannot read the private scope item, so it trusts that the Spec worker copied the criterion text verbatim, tag included.
- Rejected: comparing trees (a merge from `main` always changes the tree), and a `--force` override for the gate (it would also skip real changes).
