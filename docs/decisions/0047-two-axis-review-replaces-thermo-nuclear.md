# 0047: PR review runs isolated Standards and Spec workers from a vendored skill

- Status: Accepted
- Date: 2026-11-07
- Affects: ROBO-181, [REVIEW.md](../../REVIEW.md), `.claude/skills/rv2-code-review/`, `tools/check-review-record.py`; supersedes [0037](0037-review-rubric-single-repo-copy.md)

## Context
- One reviewer judged maintainability, rules and scope together, with a rubric that asked for ambitious restructuring. PR 133, docs only, took five rounds and 12 findings.
- `tools/review.sh:59-60` (at 9255151) checked the working tree while the record claimed `merge-base..HEAD`. The gate didn't validate the `<details>` wrapper or the reviewed head, and skipped any PR without an Allowed paths row.
- Upstream `code-review` separates the axes, but its name collides with other installed skills and it skips Spec when no spec is found.
- CI's token can't read private rv2-docs, and the local vault is often ahead of GitHub.

## Decision
- A vendored `rv2-code-review` skill, pinned to an upstream SHA, runs Standards and Spec as fresh, isolated workers. The parent aggregates and never re-ranks.
- `tools/check-review-record.py` derives the tier from changed paths: docs gets Standards only, listed high-risk paths add Risk. A ready non-bot PR needs an Allowed paths row and a record for its head.
- A missing spec is `not assessable` and blocks. The spec source is the vault's file on disk first, pinned by its blob hash, because a commit SHA can't pin uncommitted text.
- Findings are blockers or suggestions. Green required CI on the head counts as `--full`.

## Consequences
- Every new commit makes the record stale, so reviews run last. A description edit reruns the gate without a new commit.
- A missing test in tooling is a suggestion: on PR 134, most blockers that made review rounds repeat were such gaps.
- Structural review shrinks to two suggestion-level rules in REVIEW.md. If maintainability slips, add documented rules, not a broader reviewer.
- CI checks the Spec source's format, not its content. The gate's finding count is a lower bound, since Outcomes keeps earlier rounds' rows. The `Protocol:` hash detects drift but doesn't prove a review ran, so the owner's spot-check stays.
- Rejected: using the skill live from GitHub. It is mutable, so reviews could change without a PR.
- Rejected: custom agent types. Cloud sessions don't always load them (ADR 0037).
- Rejected: a PAT for CI to read rv2-docs. It adds a secret, and CI would still miss uncommitted items.
- Rejected: a "trivial change" exemption. It can't be checked mechanically.
