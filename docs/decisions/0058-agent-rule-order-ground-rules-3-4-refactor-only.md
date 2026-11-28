# 0058: Agent rules have an order; ground rules 3 and 4 apply only in refactor items

- Status: Accepted; amended by [ROBO-278](ROBO-278-drop-bv2-asset-compatibility.md): ground rule 3 applies in items that rewrite a file or module in a big way
- Date: 2026-11-17
- Affects: [AGENTS.md](../../AGENTS.md) "Agent rules" and "Ground rules" 3 and 4; amends [ADR 0009](0009-rename-internal-identifiers-continuously.md); ROBO-226 (workflow audit WP-05, owner approved 2026-11-11)

## Context
- AGENTS.md asked for minimal changes and no corner cases, and also asked every PR to rename `bv2`/`babo` identifiers and replace unsafe C string calls in the files it touches (ground rules 3 and 4). The two pulled against each other and gave no winner.
- Reviews raised renames, string-call cleanups and unnamed edge cases on bug fixes, which then grew past their scope item (audit 2026-11-11).

## Decision
When rules conflict, the scope item wins, then the three minimal-change rules in AGENTS.md "Agent rules", then the ground rules. Ground rules 3 (internal renaming) and 4 (touch-and-improve) apply only in items of kind `refactor`. A reviewer does not raise a case the scope item does not name unless it is reachable from the network.

## Consequences
- Renames and string-call cleanups happen in `refactor` items, which can be planned and reviewed as mechanical work; feature and fix PRs stay small.
- `bv2`/`babo` names stay longer in files only touched by feature and fix work.
- Wire input stays in scope for every review, since it is reachable from the network.
- Rejected: dropping ground rules 3 and 4 (the owner still wants the code renamed and modernised).
