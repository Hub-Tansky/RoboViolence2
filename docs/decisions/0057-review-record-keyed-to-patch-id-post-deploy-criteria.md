# 0057: The review record is keyed to the diff's patch-id; post-deploy criteria don't block

- Status: Accepted
- Date: 2026-11-17
- Affects: `tools/check-review-record.py` (`patch_id`, `check_record`, `check_spec_matrix`), `REVIEW.md` section 5 and the record template, `.claude/skills/rv2-code-review/SKILL.md`, `tests/test_review_record.py`; amends [ADR 0047](0047-two-axis-review-replaces-thermo-nuclear.md); ROBO-224 (workflow audit WP-03, owner approved 2026-11-11)

## Context
- The gate failed when the PR head differed from the head in the record's `Diff:` line. A merge from `main` that left the diff unchanged forced a new review round: 32 such merges in 25 PRs (audit 2026-11-11).
- The Spec matrix accepted only `pass`, `partial` and `fail`. PR #152 waited 2 h 14 min for the owner because one criterion could only be checked on a deployed server.

## Decision
A review record stays current while `git diff --text <merge-base>...<head> | git patch-id --verbatim` (git diff options pinned) equals its `Patch-id:` line, even if the PR head moved; and the Spec status `post-deploy` passes only for a criterion that starts with `(post-deploy)`. Details: [REVIEW.md section 5](../../REVIEW.md#5-pr-close-isolated-review).

## Consequences
- A merge from `main` with no conflict in the PR's files needs no new review round. `git patch-id --verbatim` ignores line numbers but not whitespace or context lines (`--stable` alone would accept a re-indented Python or YAML block), so a merge that changes lines next to the PR's hunks still asks for a round.
- A record without `Patch-id:` behaves as before.
- Known limit: the gate cannot read the private scope item, so it trusts that the Spec worker copied the criterion text verbatim, tag included.
- Rejected: comparing trees (a merge from `main` always changes the tree), and a `--force` override for the gate (it would also skip real changes).
