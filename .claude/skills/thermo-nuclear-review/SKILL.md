---
name: thermo-nuclear-review
description: Strict maintainability review of a branch's diff (structure, simplification, 1k-line files, spaghetti branching, boundaries). Rubric for the step-closing review in REVIEW.md section 5; loaded by the thermo-nuclear-code-quality-review subagent.
---

# Thermo-nuclear review rubric

Review the diff for code quality without changing behaviour. Be ambitious: look for the restructuring ("code judo") that deletes branches, modes, helpers or layers, not one that moves the same complexity around. Working code is not enough to pass.

## Presumptive blockers

Each needs a clear justification from the author, or it is a finding:

1. **Missed simplification.** A plausible reframing would delete whole categories of complexity, and the diff keeps it.
2. **File past 1000 lines.** The diff takes a file from under 1000 to over 1000 lines. Ask for decomposition first.
3. **Spaghetti growth.** New ad-hoc `if`s, one-off flags or special cases bolted onto an unrelated or already busy flow. Push the logic into its own function, type, table or module. Here that includes scattered `#ifndef CONSOLE` blocks and `sv_serverType` checks.
4. **Wrong layer.** Feature logic in a shared path, implementation details leaking through an API, or a bespoke helper where a canonical one exists (`CString`, `dksvar`, `bb_*`, the `checkPacket` table, `tools/` scripts).
5. **Indirection without value.** Thin or identity wrappers, "magic" generic handling that hides a simple data shape.
6. **Muddy contracts.** Casts, nullable or optional parameters, or silent fallbacks that paper over an unclear invariant. Make the boundary explicit.

## Also check

- Copy-pasted logic that should be one helper.
- Edge-case handling in the middle of a busy function.
- Related updates that can leave state half-applied; serialized work that is independent.
- Refactors that pass tests but leave the code less modular or harder to scan.

## Output

Order findings: structural regressions, missed simplifications, branching growth, boundary and type problems, file size, modularity, legibility. Per finding: severity, `path:line`, the problem, the concrete fix. Prefer a few high-conviction findings over many nits; skip cosmetic notes when structural issues exist. Be direct: say plainly when the change makes the codebase messier.
