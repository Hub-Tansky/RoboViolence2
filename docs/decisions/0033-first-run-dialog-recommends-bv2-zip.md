# 0033: The first-run data dialog recommends the original BV2 zip and can open it

- Status: Superseded by 0040
- Date: 2026-10-23
- Affects: `game/src/DataDirPicker.cpp`, Project item "[GUI] First-run data dialog: recommend downloading the original BaboViolent 2 data" (owner-approved **[GUI]**, 2026-10-20); amends ADR 0016

## Context
- The first-run message box of ADR 0016 only said to choose the folder that contains `main/`. Players don't know where to get the data.
- The upstream repository's zip (`https://github.com/Daivuk/BaboViolent2/archive/refs/heads/master.zip`) holds the data at `BaboViolent2-master/BaboViolent2/Content/main/`; `bv2::findDataIn` already accepts every level from `BaboViolent2-master` down to `main/`, and `bv2::detectDataDir` finds it unzipped in Downloads, Desktop or Documents.
- SDL message boxes can't show clickable links.

## Decision
The box (owner's final copy, 2026-10-23) is marked "[Temporary workaround]", strongly recommends downloading that zip, shows its URL and the Download button as the alternative, says to choose the `BaboViolent2-master` folder (any level down to `main/` still works) or to unzip it into Downloads, Desktop or Documents and restart. Buttons: "Choose folder..." (Enter), "Download BV2 data" (opens the URL with `SDL_OpenURL`, then shows the box again) and "Use placeholders" (Escape; closing the box counts the same). Lines are broken by hand because the X11 box doesn't wrap.

## Consequences
- The text stays English only until PNS-21, like the rest of the dialog.
- If the upstream repository moves or is renamed, the URL in `DataDirPicker.cpp` and here must change.
- Rejected: a custom in-game screen with a real link (a GUI change far beyond a message box); bundling or downloading the data ourselves (releases ship without the original assets, ADR 0020).
