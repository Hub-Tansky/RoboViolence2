# 0023: The step-closing review subagent and its rubric live in the repo

- Status: Superseded by 0037
- Date: 2026-10-17
- Affects: [REVIEW.md](../../REVIEW.md) section 5

## Context
- The step review needed the owner's account skill `thermo-nuclear-code-quality-review`. It sets `disable-model-invocation: true`, so an agent's Skill call fails and only the owner can start the review. Cloud thread sessions also may not list it.
- The owner's Cursor agent of the same name pointed at a `cursor-team-kit` skill and Cursor `Task` subagent types, which Claude Code doesn't have.

## Decision
- `.claude/agents/thermo-nuclear-code-quality-review.md`: the reviewer subagent; read-only tools; runs REVIEW.md section 5 step 2.
- `.claude/skills/thermo-nuclear-code-quality-review/SKILL.md`: the owner's rubric, tracked without `disable-model-invocation`. Same name, so the report's load line and the DONE gate are unchanged.

## Consequences
- Any Claude Code session with the repo checked out can close a step without the owner starting the review.
- The rubric is versioned and reviewed like code.
- On a machine where the owner's personal skill of the same name is installed with `disable-model-invocation: true`, it may take precedence; remove that flag there if the subagent's Skill call fails.
