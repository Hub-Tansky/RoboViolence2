# 0019: The game shows its own name and credits the original as an unofficial fork

- Status: Accepted
- Date: 2026-10-14
- Affects: [Phase C step 2](../roadmap/phase-c-installers-and-fixes/step2-name-and-credits.md); amends [0008](0008-display-name-robo-violence-2.md) and [0010](0010-bv2-asset-compat-and-gui-freeze.md)

## Context
- Engine-only releases ship before replacement assets (PNS-20, owner 2026-10-14). ADR 0008 requires the in-game name, credits and window title to change before any public release, without implying endorsement.
- ADR 0010 freezes the BV2 interface until replacement assets ship.

## Decision
Owner-approved **[GUI]** exception to ADR 0010 (2026-10-14):
- `lang_gameName` (window title), the default `sv_gameName` and the dedicated-server banner say Robo Violence 2.
- Credits open with "Robo Violence 2", an unofficial-fork line naming bitHeads / RndLabs and "not endorsed by them", a thank-you, then "Original credits below" and the original list.
- A "LOADING" splash in the menu font replaces the RndLabs logo.
- The account note no longer sends players to the dead official forum. The unused survey is removed.

## Consequences
- Source copyright notices, in-game terms ("Babo") and the rest of the rebrand (binaries, `BV2_*`, pref path, master game ID) are unchanged until PNS-20.
- These strings come from the package's `en.lang` (ADR 0016); the player's original `en.lang` no longer sets the name.
- Survey message IDs 1016–1018 stay unused.
