# 0044: The backlog is one Markdown file per scope item in the private rv2-docs vault

- Status: Accepted
- Date: 2026-11-05
- Affects: [AGENTS.md](../../AGENTS.md) "Work tracking", [REVIEW.md](../../REVIEW.md), `tools/review.sh`, `.github/pull_request_template.md`; supersedes [0024](0024-github-project-is-the-backlog.md)

## Context
- The private GitHub Project held 154 items (2026-11-05). Cloud sessions can't call its GraphQL API, so every read or move went through a dispatched workflow in a helper repo with a stored PAT.
- The owner plans and writes in Obsidian; scope lived in two places (Project and vault notes).

## Decision
The backlog is `40-backlog/` in the private repo [Hub-Tansky/rv2-docs](https://github.com/Hub-Tansky/rv2-docs), which is the owner's Obsidian vault. One file per scope item, `ROBO-NNN <Title>.md`; the folder is the status (`10-next/`, `20-proposed/`, `30-inprogress/`, `90-done/`). PNS-n became ROBO-0nn. Conventions live in that repo's AGENTS.md. PRs and review records cite `ROBO-NNN`. GitHub issues stay the public intake.

## Consequences
- Agents read and move items with plain git; no PAT, no helper workflow. The helper repo and the Project are retired.
- A session that works on the backlog needs rv2-docs attached.
- Two sessions can pick the same next number; the second push fails and renumbers.
- Rejected: backlog in iCloud only (cloud sessions see it only while the Mac is online, git in iCloud corrupts `.git`); keeping the Project (two places, API not reachable from cloud); public repo issues for everything (exposes private plans).
