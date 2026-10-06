# Roadmap

One folder per phase; inside it, the phase README (index and "done when") and one scope file per step, copied from [_template-step.md](_template-step.md).

| Phase | Folder | Status |
|---|---|---|
| A: modern portable build | [phase-a-modern-portable-build/](phase-a-modern-portable-build/README.md) | DONE (2026-10-06) |
| B: security, infrastructure, anti-cheat | plan in [future-phases.md](future-phases.md); step files go in `phase-b-security-infrastructure-anti-cheat/` | Planned |

## Rules

- Every step file has a **Status** line under its title: `TODO`, `IN PROGRESS` or `DONE (YYYY-MM-DD)`. Set `DONE` when the step's PR merges; a phase is `DONE` when all its steps are.
- When closing a step, add anything learned about later phases or out-of-scope work to [possible-new-scope.md](possible-new-scope.md): where it was found, an extract of the source, why it matters and a suggested home. Don't change later phase plans directly; the owner accepts or rejects each entry.
- Phase B ground rules and section order: [future-phases.md](future-phases.md).
