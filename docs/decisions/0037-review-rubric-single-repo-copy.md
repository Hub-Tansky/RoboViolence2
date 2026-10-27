# 0037: The review rubric has one repo copy, and the gate checks its hash

- Status: Accepted
- Date: 2026-10-27
- Affects: [REVIEW.md](../../REVIEW.md) section 5, `tools/review.sh`, `.claude/`; supersedes [0023](0023-step-review-subagent-in-repo.md)

## Context
- Three copies of the rubric named `thermo-nuclear-code-quality-review` got loaded: the repo's `.claude/skills/`, the owner's `~/.claude/skills/` and the owner's claude.ai account skill (`anthropic-skills:` in cloud sessions, PR #85 round 1). A bare-name Skill call resolved to whichever the session listed first, and nothing kept them identical.
- Cloud thread sessions don't always load the repo's `.claude/agents/` and `.claude/skills/`, so the reviewer ran as a general-purpose agent (PR #77, #82, #85).
- The gate checked only the text `SKILL: Launching skill: thermo-nuclear-code-quality-review`, which any copy prints and an author can type (PR #85 round 2).

## Decision
- Rename the subagent and skill to `rv2-thermo-nuclear-review`, a name no other copy uses. The repo file is the only copy.
- `SKILL.md` carries `Rubric: rv2-thermo-nuclear-review <hash>`, the first 12 hex digits of the sha256 of the file without that line. `tools/review.sh` fails when the line is stale, and a ready PR's reviewer report must contain the line for the head's rubric.
- When the agent type isn't offered, a general-purpose agent follows `.claude/agents/rv2-thermo-nuclear-review.md` and reads the rubric file directly.

## Consequences
- A report made with an old or out-of-repo rubric fails the gate; a rubric edit needs its hash updated (review.sh prints the value) and a review on the new rubric.
- The owner's personal and account copies no longer matter to the gate; they can be deleted.
- Still evidence, not proof: an author could copy the line by hand. The owner's spot-check (REVIEW.md section 5) stays.
- Rejected: keeping the old name and checking the hash only. A bare-name Skill call would still load another copy first.
- Rejected: a CI check on user-level copies. CI can't see them.
