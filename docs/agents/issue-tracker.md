# Issue tracker: rv2-docs scope items

Work is tracked as scope items in the private repo [Hub-Tansky/rv2-docs](https://github.com/Hub-Tansky/rv2-docs), folder `40-backlog/`, one file `ROBO-NNN <Title>.md` per item ([ADR 0044](../decisions/0044-backlog-in-the-rv2-docs-vault.md)). Its `AGENTS.md` has the folders, fields and who moves what. GitHub Issues in this repo are only the public intake for bugs and requests. If rv2-docs can't be reached, say so and stop.

## When a skill says "publish to the issue tracker"

- Create `40-backlog/20-proposed/ROBO-NNN <Title>.md` from `40-backlog/_templates/scope.md`: `status: Proposed`, `kind`, `area`, `gui`, a Context section and an **Acceptance criteria** checklist.
- Next number: highest `ROBO-NNN` anywhere in `40-backlog/` plus one. Never reuse one.
- Several tickets from one spec: one file each, linked with `blocked_by: [ROBO-NNN]`. Never one combined tickets file.
- Commit in rv2-docs, open a PR and merge it (owner's standing rule for adding backlog items).
- Never create task lists, `NEXT_STEPS.md`, roadmap or step files in this repo.

## When a skill says "fetch the relevant ticket"

Read `40-backlog/*/ROBO-NNN *.md` in rv2-docs. A GitHub issue number (`#n`) refers to the intake issue; its scope item links it in `issue:`.

## Status

The folder is the status: `20-proposed/` Proposed, `10-next/` Next (only the owner moves items here), `30-inprogress/` In progress or Testing, `90-done/` Done. A move is `git mv` plus a `status`/`updated` edit in one rv2-docs commit.

## PRs as a request surface

Off. External PRs are not triaged as requests.
