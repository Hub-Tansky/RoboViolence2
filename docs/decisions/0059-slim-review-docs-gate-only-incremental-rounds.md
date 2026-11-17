# 0059: Docs PRs get no review worker; later rounds review only the new commits

- Status: Accepted
- Date: 2026-11-17
- Affects: [REVIEW.md](../../REVIEW.md) sections 3 and 5 and the record template, `.claude/skills/rv2-code-review/SKILL.md`, `tools/check-review-record.py`, `tests/test_review_record.py`; amends [ADR 0047](0047-two-axis-review-replaces-thermo-nuclear.md); ROBO-227 (workflow audit WP-06, owner approved 2026-11-11)

## Context
- 44 of 55 PRs carried review-fix commits (109 in total). Docs PRs got LLM rounds for wording.
- Every round re-reviewed the full diff with workers reading every changed file in full. A session with five PRs under review hit the usage limit in 32 minutes (audit 2026-11-11).

## Decision
The docs tier runs no worker: `tools/review.sh` and a record with `### review.sh` and `### Not done / unsure` are the review. In the code tier, workers run one after another and read changed hunks with 40 lines of context; after fixes only the axes that had a blocker rerun (every axis when the skill's `Protocol:` line changed), on the PR's own new commits plus their open findings, for two rounds at most, and what is left goes to the owner in one message.

## Consequences
- A docs PR is checked for paths, encoding, links and the Allowed paths row, not for wording.
- A defect in code that a fix commit touches but no blocker named is caught only by CI or by the owner.
- The record gains `Rounds: <n>`, so rounds can be counted.
- "New behaviour" (REVIEW.md section 3) no longer covers CI jobs and scripts unless they are guard files, so those need no test.
- Rejected: dropping Standards for code PRs (it found the real bugs in WP-02 and WP-03).
