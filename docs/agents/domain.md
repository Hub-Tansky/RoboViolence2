# Domain docs

Single-context repo. How the engineering skills read this repo's domain docs.

## Before exploring, read these

- `CONTEXT.md` at the repo root: the code glossary. Doesn't exist yet; `/domain-modeling` creates it when a term gets resolved.
- `CONTEXT.md` in rv2-docs: backlog terms (scope item, `ROBO-NNN`, status, GUI item, area).
- `docs/decisions/` (not `docs/adr/`): the ADRs, indexed in its README. Read the ones touching the area you work in. New ADRs go there in its format.
- `AGENTS.md` and `ARCHITECTURE.md`: rules, layout and the file inventory.

If a file doesn't exist, proceed silently. Don't suggest creating it upfront.

## Use the glossary's vocabulary

Name domain concepts (issue titles, refactor proposals, test names) with the terms in `CONTEXT.md`. A missing term is either invented language (reconsider) or a real gap (note it for `/domain-modeling`).

## Flag ADR conflicts

If your output contradicts an ADR, say so explicitly instead of overriding it:

> _Contradicts ADR 0010 (BV2 asset compatibility), but worth reopening because…_
