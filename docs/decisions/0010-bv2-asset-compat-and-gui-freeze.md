# 0010: Keep BV2 asset compatibility and the BV2 interface until replacement assets ship

- Status: Accepted
- Date: 2026-10-06
- Affects: [future-phases.md](../refactoring/future-phases.md) ground rules 1 and 2, §F, §H.1

## Context
- Replacement assets (§H) will take a long time; until then the game is tested with the owner's local BV2 data (`BV2_DATA_DIR`).
- Format or layout changes would make that data unusable or make the game look different from what testers compare against.

## Decision
Until §H.1 ships a full replacement set, the game loads original BV2 data unchanged (formats, names, `main/` layout) and keeps the BV2 interface (menus, HUD, fonts, text). Any change visible to the player is marked **[GUI]** and needs the owner's explicit approval first.

## Consequences
- New formats (e.g. `.po`, ADR 0011) are added beside the old ones, never instead.
- Renderer work (§F) must produce the same picture; verified by comparing screenshots with original data.
- Languages needing glyphs outside the BV2 font wait for §H.6 fonts.
- Dropping BV2 compatibility after §H.1 needs a new ADR.
