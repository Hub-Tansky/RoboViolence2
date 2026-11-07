# Issue tracker: rv2-docs scope items

Work is tracked as scope items `ROBO-NNN <Title>.md` in `40-backlog/` of the private repo [Hub-Tansky/rv2-docs](https://github.com/Hub-Tansky/rv2-docs) ([ADR 0044](../decisions/0044-backlog-in-the-rv2-docs-vault.md)). Its [AGENTS.md](https://github.com/Hub-Tansky/rv2-docs/blob/main/AGENTS.md) § Backlog owns the rules: numbering, folders, fields, moves, git. GitHub Issues here are only the public intake ([AGENTS.md](../../AGENTS.md#work-tracking)).

## When a skill says "publish to the issue tracker"

Create one Proposed scope item per ticket, per rv2-docs `AGENTS.md` § Backlog and `40-backlog/_templates/scope.md`. Link dependent items with `blocked_by`. Never one combined tickets file.

## When a skill says "fetch the relevant ticket"

Read `40-backlog/*/ROBO-NNN *.md` in rv2-docs. `#n` is the intake issue; its scope item has an `issue:` URL ending in `/issues/<n>`.

## Triage (GitHub intake)

`/triage` works on this repo's GitHub issues with `gh issue list/view/comment/edit --add-label/close`. For `ready-for-agent` and `ready-for-human`, the brief goes into a new Proposed scope item (Context and Acceptance criteria), and the issue gets a comment naming its `ROBO-NNN`.

## Wayfinding operations

Used by `/wayfinder`. Map and tickets are scope items in rv2-docs `40-backlog/`; nothing goes in this repo or `.scratch/`. Frontmatter keys `wayfinder` and `map` replace the skill's labels and parent link.

- **Map**: a scope item with `kind: research`, `wayfinder: map`, kept in `30-inprogress/` while open. Its body has the skill's sections (Destination, Notes, Decisions so far, Not yet specified, Out of scope); Acceptance criteria is "Destination reached, no open tickets".
- **Ticket**: a Proposed scope item with `wayfinder: <research|prototype|grilling|task>`, `map: ROBO-NNN` and a `## Question` section. Refer to tickets by title, linked to their file.
- **Blocking**: `blocked_by: [ROBO-NNN]`, added in a second pass once the numbers exist.
- **Frontier**: items in `20-proposed/` with this `map:` whose `blocked_by` items are all in `90-done/`; lowest number first.
- **Claim**: move the ticket to `30-inprogress/` (status In progress) and push before any work. A rejected push means another session moved first: pull and pick again.
- **Resolve**: add `## Answer`, move the ticket to `90-done/`, and append a linked one-line gist to the map's Decisions so far, in one commit.
- **Out of scope**: add `## Out of scope` with the reason, move the ticket to `90-done/`, and add one line to the map's Out of scope section.
- **Research branches**: `research/<name>` in this repo, linked from the ticket; never merged.

## PRs as a request surface

Off.
