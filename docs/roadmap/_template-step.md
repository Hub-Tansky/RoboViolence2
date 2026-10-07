# Step N: <title>

**Status:** TODO | IN PROGRESS | DONE (YYYY-MM-DD)

**Depends on:** <steps>. **Next:** <step>. **Index:** [README.md](README.md) (the phase README)

## Scope

| Field | Value |
|---|---|
| Goal | <one sentence: what is true after this step> |
| In scope | Tasks N.1 … N.M below |
| Out of scope | <tempting work that belongs elsewhere, with the step or phase it belongs to> |
| Allowed paths | <globs the agent may create or modify; add `docs/analysis/KEY_QUESTIONS.md` if the step may find defects> |
| Inputs | `AGENTS.md`, `ARCHITECTURE.md`, earlier review records `reviews/step*.md`, <analysis docs, files> |
| Deliverables | <files and commits that must exist> |
| Definition of done | See "Acceptance checks". All must pass. `ARCHITECTURE.md` is updated |

## Context

<why this step exists; evidence with file:line references>

## Tasks

### N.1 <task>

- <concrete actions>

## Critical files

- <paths>

## Acceptance checks

- <commands, each in its own code block, and their expected results>
- `tools/review.sh --full <preset>` passes, and the fresh-context review record `reviews/stepN.md` exists (`REVIEW.md` section 5).
- `ARCHITECTURE.md` lists every file added, moved or removed in this step.
- The secret scan (`gitleaks git --staged` / CI) is clean.
