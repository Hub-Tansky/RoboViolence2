# 0024: The private GitHub Project is the backlog; no roadmap files in the repo

- Status: Accepted
- Date: 2026-10-18
- Affects: [AGENTS.md](../../AGENTS.md) "Work tracking", [REVIEW.md](../../REVIEW.md), `tools/review.sh`, `.github/pull_request_template.md`

## Context
- Plans lived as `docs/roadmap/<phase>/stepN-*.md` scope files, review records under `reviews/`, and `possible-new-scope.md` (71 PNS entries). Status was kept by hand in three places (step file, phase README, roadmap README) and drifted (Phase C README said TODO after every step merged).
- The repo is public; private plans and half-formed ideas don't belong in it. Issues were off, so outsiders could not report bugs.
- `tools/review.sh` read scope and the DONE gate from step files.

## Decision
The private Project [Robo Violence 2](https://github.com/users/Hub-Tansky/projects/1) is the only source of truth for backlog, next steps, bugs, features and proposed scope: draft items for private plans, public issues for work ready to share. A PR carries its scope (Allowed paths) and review record in its description; `tools/review.sh` reads them from there.

## Consequences
- Statuses Proposed → Next → In progress → Testing → Done; blocked work stays In progress with `Blocked by:` in the item. Project workflows move items on issue and PR events.
- `docs/roadmap/` is removed; its last state is commit `58c91b2`, which existing links point at. PNS-n IDs survive in the Project's Source field. The owner keeps a private copy in the Obsidian vault.
- The `review` CI job reruns on PR description edits.
- Rejected: Markdown task files as a fallback when the Project is unreachable (two sources drift again); public Project (exposes private plans); an Actions workflow for status sync (needs a stored PAT; built-in Project workflows cover it).
