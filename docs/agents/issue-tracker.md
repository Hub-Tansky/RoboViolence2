# Issue tracker: rv2-docs scope items

Work is tracked as scope items `ROBO-NNN <Title>.md` in `40-backlog/` of the private repo [Hub-Tansky/rv2-docs](https://github.com/Hub-Tansky/rv2-docs) ([ADR 0044](../decisions/0044-backlog-in-the-rv2-docs-vault.md)). Its [AGENTS.md](https://github.com/Hub-Tansky/rv2-docs/blob/main/AGENTS.md) § Backlog owns the rules: numbering, folders, fields, moves, git. GitHub Issues here are only the public intake ([AGENTS.md](../../AGENTS.md#work-tracking)).

## When a skill says "publish to the issue tracker"

Create one Proposed scope item per ticket, per rv2-docs `AGENTS.md` § Backlog and `40-backlog/_templates/scope.md`. Link dependent items with `blocked_by`. Never one combined tickets file.

## When a skill says "fetch the relevant ticket"

Read `40-backlog/*/ROBO-NNN *.md` in rv2-docs. `#n` is the intake issue; its scope item has an `issue:` URL ending in `/issues/<n>`.

## Triage (GitHub intake)

`/triage` works on this repo's GitHub issues with `gh issue list/view/comment/edit --add-label/close`. For `ready-for-agent` and `ready-for-human`, the brief goes into a new Proposed scope item (Context and Acceptance criteria), and the issue gets a comment naming its `ROBO-NNN`.

## Wayfinding operations

Used by `/wayfinder`. Map, tickets, answers and research notes live in rv2-docs `40-backlog/`, never in this repo or `.scratch/`. Frontmatter keys `wayfinder` and `map` replace the skill's labels and parent link. Exception to rv2-docs `AGENTS.md` § Backlog (canonical entry: ROBO-180): tickets move to `90-done/` when resolved or ruled out of scope, and the map when its acceptance criterion holds, without a PR.

- **Map**: a scope item with `kind: research`, `wayfinder: map`, created directly in `30-inprogress/` (In progress). Its body has the skill's sections (Destination, Notes, Decisions so far, Not yet specified, Out of scope); Acceptance criteria is "Destination reached, no open tickets".
- **Ticket**: a Proposed scope item, `kind: research`, with `wayfinder: <research|prototype|grilling|task>`, `map: ROBO-NNN` and `## Question` in place of Context and Acceptance criteria.
- **Blocking**: `blocked_by: [ROBO-NNN]`, written at creation. If a push forces a renumber, update every `map:` and `blocked_by` citing the old number in the same commit.
- **Frontier**: items in `10-next/` or `20-proposed/` with this `map:` whose `blocked_by` items are all in `90-done/`; `10-next/` first, then lowest number.
- **Claim**: move the ticket to `30-inprogress/` (status In progress) and push before any work. A rejected push means another session moved first: pull and pick again.
- **Resolve**: add `## Answer`, move the ticket to `90-done/`, and append a linked one-line gist to the map's Decisions so far, in one commit.
- **Out of scope**: add `## Out of scope` with the reason, move the ticket to `90-done/`, and add one line to the map's Out of scope section.
- **Research findings**: in the ticket's `## Answer`, or, when long, a `type: research` note in rv2-docs `20-product/` linked from it. This replaces the skill's `research/<name>` branch and the research skill's in-repo file.
- **Prototypes**: a logic prototype goes in rv2-docs as a note linked from `## Answer`; a UI prototype on a throwaway `prototype/<name>` branch here, never merged, linked from `## Answer`. Nothing is folded into game code under a ticket.
- **Glossary and ADRs**: domain-modeling edits follow [domain.md](domain.md) and reach this repo only through a normal scoped PR; record them as a Proposed item.

## PRs as a request surface

Off.
