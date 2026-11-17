# 0063: ARCHITECTURE.md lists module directories, not files

- Status: Accepted
- Date: 2026-11-17
- Affects: [ARCHITECTURE.md](../../ARCHITECTURE.md), `tools/check-architecture.sh`, [AGENTS.md](../../AGENTS.md), [REVIEW.md](../../REVIEW.md) section 2; ROBO-235 (workflow audit WP-12, owner decision 6 of 2026-11-11)

## Context
- `ARCHITECTURE.md` had one row per tracked file (90 KB), and `tools/check-architecture.sh` failed on any file without a row.
- 42 of 59 merges in 30 days edited it (audit 2026-11-11); parallel PRs conflicted on it, and the asset registry work (refactor direction A1, A2) would add a row for every new source file.

## Decision
`ARCHITECTURE.md` keeps the summary, facts and open items and has one row per module directory under `## Modules`; `tools/check-architecture.sh` checks that every directory under `engine/`, `packaging/` and `content/`, and `game/src`, `masterserver/src`, `tests` and `tools`, holding tracked files has a row, and that every row names an existing directory.

## Consequences
- A PR that only adds files to an existing module needs no `ARCHITECTURE.md` edit; a new module directory needs a row.
- A file's purpose lives in its header comment, its module README or the ADR that added it. The last per-file table is `git show 276d92d:ARCHITECTURE.md`.
- Rejected: generating the table from header comments (another tool to keep green for a list nobody needs to read in full).
