# 0023: The step-closing review subagent and its rubric live in the repo

- Status: Accepted
- Date: 2026-10-17
- Affects: [REVIEW.md](../../REVIEW.md) section 5, `tools/review.sh` DONE gate

## Context
- The step review needed the owner's account skill `thermo-nuclear-code-quality-review`. It sets `disable-model-invocation: true`, so an agent's Skill call fails and only the owner can start the review. Cloud thread sessions also may not list it.
- The owner's Cursor agent of the same name pointed at a `cursor-team-kit` skill and Cursor `Task` subagent types, which Claude Code doesn't have.

## Decision
- `.claude/agents/thermo-nuclear-code-quality-review.md`: the reviewer subagent; read-only tools; runs REVIEW.md section 5 step 2.
- `.claude/skills/thermo-nuclear-review/SKILL.md`: the rubric, model-invocable, written for this repo. A distinct name, because a personal skill of the same name would shadow it.
- The DONE gate accepts the load line of either skill name, so earlier reports stay valid.

## Consequences
- Any Claude Code session with the repo checked out can close a step without the owner starting the review.
- The rubric is versioned and reviewed like code; changing it needs the owner's approval (guard-file rule).
- Rejected: copying the account skill verbatim (third-party text of unknown licence in a GPL repo; still shadowed by the personal copy).
