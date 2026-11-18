# ROBO-248: Name new ADRs after their scope item, not the next number

- Status: Accepted
- Date: 2026-11-18
- Affects: `docs/decisions/README.md`, [AGENTS.md](../../AGENTS.md) "Agent rules"; ROBO-248 (workflow audit WP-13, approved 2026-11-11)

## Context
- ADRs were numbered in order. Parallel PRs each took the next free number and collided: `1c309af`, `697bc03`, PR #152, and 0067 (PR #180 and PR #186, 2026-11-18; #186 renumbered to 0068 after a merge conflict).
- Every PR already works on one scope item `ROBO-NNN`, and rv2-docs hands those numbers out one at a time (its AGENTS.md § Backlog), so they don't collide.
- No tool parses the `NNNN-` prefix: `grep -rn "decisions/0" tools .github` finds only comments and links to existing ADRs, and the review and docs-only checks match `docs/` by path only.

## Decision
A new ADR is `docs/decisions/ROBO-NNN-<slug>.md`, titled `# ROBO-NNN: <decision>`, NNN being the scope item that makes the decision; a second one from the same item is `ROBO-NNN-2-<slug>.md`. The files `0001` to `0070` keep their names and numbers. The index lists both kinds, new rows last.

## Consequences
- Two parallel PRs can no longer pick the same ADR name; they can still both append an index row, a one-line merge conflict.
- An ADR's id is `0042` or `ROBO-248`; `Superseded by` and citations use either.
- The index is no longer sorted by one number: old ADRs by number, then new ones in the order they merged.
- Rejected: a date prefix (two PRs on one day still collide); reserving numbers in a shared file (one more step that can be forgotten).
