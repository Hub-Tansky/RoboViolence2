<!-- Scope: REVIEW.md section 5 and AGENTS.md "Agent rules". tools/review.sh reads the Item and Allowed paths rows and the review record below. -->

| Field | Value |
|---|---|
| Item | <ROBO-NNN scope item; `Closes #n` when its `issue:` links one> |
| Goal | <one sentence: what is true after this PR> |
| Out of scope | <tempting work left out, with its ROBO-NNN item> |
| Allowed paths | <globs in backticks, e.g. `game/src/Server*.cpp` `tests/**` `ARCHITECTURE.md`> |

## Owner brief
<the line tools/merge-risk.py prints, e.g. Tier: MEDIUM (game/src/Game.cpp)>
Player sees: <what changes in the game, plain words>
Not changed: <formats, protocol, menus, as applies>
Proof: <test name; for a fix, the assertion that fails without it>
Not covered by a test: <what, and the manual steps>
Undo: <revert sha; data migration yes or no>

<!-- Required on a ready MEDIUM or HIGH PR. For a content/LaunchScript/** change, list every value it changes. -->

Before:

After:

## How

## Tests

<commands run and their results; fix proof for fixes>

<!-- Before marking ready: add "## Review record" from REVIEW.md (template at the end). -->
