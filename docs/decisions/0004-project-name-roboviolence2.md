# 0004: Name the project RoboViolence 2, an unofficial fork of BaboViolent 2

- Status: Superseded by 0008
- Date: 2026-10-03
- Affects: [step0 §0.2](../refactoring/phase-a-modern-portable-build/step0-fork-rename-and-asset-removal.md)

## Context
- The GPLv3 covers code only. The name "BaboViolent 2" and its logos are claimed by RndLabs Inc. (`License.txt` §2c) with no grant (the asset licence notes (kept outside this repository)).
- The fork must ship under a distinct name until a grant arrives (the permission request (kept outside this repository)).
- Binaries, window titles, in-game strings and code identifiers (`bv2`, `babo`) still carry the old name.

## Decision
The project is **RoboViolence 2**, an unofficial fork of BaboViolent 2. GitHub repo `Hub-Tansky/RoboViolence2`; local dir `RoboViolence2-master`. Code identifiers, binaries and in-game strings keep the old names until [future-phases.md](../refactoring/future-phases.md) §H.

## Consequences
- Docs and README say "RoboViolence 2" and "unofficial fork"; they never imply endorsement by the original authors.
- §H must rebrand `en.lang` (`lang_gameName`), credits and window titles before any public release.
- Claude Code memory and local settings are keyed by directory path; the key changed with the directory.
- Rejected: keeping "BaboViolent 2" (trademark claim, no grant); a name that still contains "Babo" (confusable).
