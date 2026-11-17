# 0060: Merge by risk tier; MEDIUM and HIGH PRs carry an owner brief

- Status: Accepted
- Date: 2026-11-17
- Affects: `tools/merge-risk.py`, `tools/review.sh`, `tools/check-review-record.py`, `.github/pull_request_template.md`, [AGENTS.md](../../AGENTS.md) "Agent rules"; ROBO-228 (workflow audit WP-08, owner decisions 1, 2 and 4 of 2026-11-11)

## Context
- The owner cannot judge code, yet every PR waited for his "merge": median 1.8 h, p90 10.3 h, 34 of 95 PRs over 4 h (audit 2026-11-11).
- Most PRs touch no wire protocol, credentials, packaging or review guard, and are already reviewed by isolated workers and three-OS CI.

## Decision
`tools/merge-risk.py` puts each PR in LOW, MEDIUM or HIGH by its changed paths, added lines, file count, `GAME_VERSION_*` changes and a `[GUI]` item (first match wins, any error is HIGH); agents merge LOW and MEDIUM PRs on green required checks and post the owner brief, and HIGH waits for the Risk worker, a Copilot review and the owner's playtest on staging and "merge".

## Consequences
- A ready MEDIUM or HIGH PR fails the `review` check without `## Owner brief` holding the script's exact line, so the brief always matches the diff.
- LOW covers docs, tests and non-guard tooling alone or mixed; a path no rule names (root `CMakeLists.txt`, `vcpkg.json`) is MEDIUM.
- The script's rules are the policy: changing a threshold is a change to `tools/merge-risk.py` and its fixtures.
- Copilot review on HIGH PRs is a trial: after five HIGH PRs it stays only if it found a confirmed blocker the workers missed.
- Rejected: an LLM judging risk (not reproducible, and the gate could not check it); merging everything on green (wire protocol and credentials need a playtest).
