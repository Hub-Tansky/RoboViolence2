# 0008: Spell the display name "Robo Violence 2"

- Status: Accepted; code-identifier timing amended by [0009](0009-rename-internal-identifiers-continuously.md)
- Date: 2026-10-04
- Affects: [step0 §0.2](../refactoring/step0-fork-rename-and-asset-removal.md); supersedes [0004](0004-project-name-roboviolence2.md)

## Context
- [ADR 0004](0004-project-name-roboviolence2.md) named the project "RoboViolence 2" for the trademark reasons it lists; those still hold.
- The owner chose the two-word spelling for the display name.

## Decision
The project's display name is **Robo Violence 2**, an unofficial fork of BaboViolent 2. Identifiers keep the one-word form: GitHub repo `Hub-Tansky/RoboViolence2`, local dir `RoboViolence2-master`, vcpkg name and desktop file `roboviolence2`, bundle/app id `org.roboviolence.bv2`. Code identifiers, binaries and in-game strings keep the old `bv2`/`babo` names until [future-phases.md](../refactoring/future-phases.md) §H.

## Consequences
- Docs, README and OS-visible names (`packaging/macos/Info.plist.in` `CFBundleName`/`CFBundleDisplayName`, `packaging/linux/roboviolence2.desktop` `Name`) say "Robo Violence 2" and "unofficial fork"; they never imply endorsement by the original authors.
- Completed step files keep "RoboViolence 2" as the historical record.
- §H must rebrand `en.lang` (`lang_gameName`), credits and window titles before any public release.
- Rejected: renaming the repo, directory and ids too (breaks clones, memory keys and installed-app identity for a cosmetic change).
