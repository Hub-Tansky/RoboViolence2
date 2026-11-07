# Issue tracker: rv2-docs scope items

Work is tracked as scope items `ROBO-NNN <Title>.md` in `40-backlog/` of the private repo [Hub-Tansky/rv2-docs](https://github.com/Hub-Tansky/rv2-docs) ([ADR 0044](../decisions/0044-backlog-in-the-rv2-docs-vault.md)). Its [AGENTS.md](https://github.com/Hub-Tansky/rv2-docs/blob/main/AGENTS.md) § Backlog owns the rules: numbering, folders, fields, moves, git. GitHub Issues here are only the public intake ([AGENTS.md](../../AGENTS.md#work-tracking)).

## When a skill says "publish to the issue tracker"

Create one Proposed scope item per ticket, per rv2-docs `AGENTS.md` § Backlog and `40-backlog/_templates/scope.md`. Link dependent items with `blocked_by`. Never one combined tickets file.

## When a skill says "fetch the relevant ticket"

Read `40-backlog/*/ROBO-NNN *.md` in rv2-docs. `#n` is the intake issue; its scope item has an `issue:` URL ending in `/issues/<n>`.

## Triage (GitHub intake)

`/triage` works on this repo's GitHub issues with `gh issue list/view/comment/edit --add-label/close`. For `ready-for-agent` and `ready-for-human`, the brief goes into a new Proposed scope item (Context and Acceptance criteria), and the issue gets a comment naming its `ROBO-NNN`.

## Wayfinding operations

Off. Don't create maps or `.scratch/` files; plan as Proposed scope items linked with `blocked_by`.

## PRs as a request surface

Off.
