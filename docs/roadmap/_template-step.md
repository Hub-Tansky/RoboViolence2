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
| Inputs | `AGENTS.md`, `ARCHITECTURE.md`, <analysis docs, files> |
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
- `REVIEW.md` checklist and `/anthropic-skills:thermo-nuclear-code-quality-review` done on the step diff; every finding fixed or confirmed with the owner.
- `ARCHITECTURE.md` lists every file added, moved or removed in this step.
- The secret scan (`gitleaks git --staged` / CI) is clean.
