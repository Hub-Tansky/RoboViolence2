# 0009: Rename internal identifiers continuously; interface names wait for §H

- Status: Accepted
- Date: 2026-10-06
- Affects: [future-phases.md](../refactoring/future-phases.md) ground rule 3, §G, §H.7; amends [0008](0008-display-name-robo-violence-2.md)

## Context
- [ADR 0008](0008-display-name-robo-violence-2.md) kept every `bv2`/`babo` identifier until §H.
- The owner wants the code to read as Robo Violence 2 without waiting for replacement assets.
- Binaries, `BV2_*` env vars, cvars, pref paths, `bv2.db`, protocol and master-server IDs are interfaces: scripts, saved configs and servers depend on them.

## Decision
Replace `bv2`/`babo` with `roboviolence2` in internal names (code symbols, namespaces, source file and class names, internal CMake targets and variables, comments) whenever a file is touched. Interface and visible names stay until §H.7.

## Consequences
- Each rename is its own mechanical commit; tests and CI must stay green.
- Old and new prefixes coexist for a long time; grep both.
- Rejected: renaming binaries and env vars now with aliases (two names to support for no player benefit); renaming pref paths now (needs a migration that §H.7 already plans).
