# 0041: Options "Game assets" sets the BV2 data folder, and "Use placeholders" is remembered

- Status: Accepted
- Date: 2026-10-29
- Affects: `game/src/COption.cpp`, `startDataDir`, `saveDataDirField` and `findRoot` in `game/src/Paths.cpp`, `chooseDataDir` in `game/src/DataDirPicker.cpp`, paste in `engine/zeven/src/dkw.cpp`; Project item "[GUI] Options: Game assets path field" (owner-approved design, 2026-10-29); amends [0016](0016-player-data-lookup-and-package-content.md) and [0040](0040-first-run-downloads-original-data.md)

## Context
- ADR 0040 removed the folder dialog from the first-run box, so players with their own BV2 copy outside the auto-detected places had no way to point the game at it.
- ADR 0016 asked a player without data on every start; ADR 0040 kept that until a way back existed.
- Windows Explorer's Extract All puts the zip's folder inside a folder of the same name, which the lookup rule missed. PR #123 fixed that in code; this ADR records it.

## Decision
- Options, at the bottom, section "Game assets": one plain text field for the BV2 data folder (no picker). Enter (or leaving the field after a change) saves it in `data-dir.txt` when the folder holds `main/textures` (any level `findDataIn` accepts), an empty field saves `placeholders`, anything else is refused with a message. It applies on the next start, because the data root is set before the window opens.
- `data-dir.txt` holds a folder or `placeholders`. "Use placeholders" (or closing the box) in the first-run box saves `placeholders`; "Open in browser" doesn't, so the box returns if the unzipped data isn't found. Order at start (`bv2::startDataDir`): `BV2_DATA_DIR`, then the saved folder; a saved `placeholders` stops there, like `--placeholders`, so an emptied field means placeholders even with data in an auto-detected place; else auto-detect, else the box.
- Text fields paste with Ctrl+V (Cmd+V on macOS) through `SDL_GetClipboardText`.
- The lookup rule (ADR 0016) also tries `<path>/<path's name>/BaboViolent2/Content` (Windows Extract All), and ignores surrounding spaces and one pair of quotes (Explorer's "Copy as path").

## Consequences
- The field's text goes through the game's Latin-1 text input: a path with characters outside Latin-1 can't be typed or pasted; such players use `BV2_DATA_DIR` or a path without them.
- A player who chose placeholders and later unzips BV2 data is not switched to it automatically; Options is the way.
- Rejected: an OS folder dialog in Options (the owner asked for the simplest field); applying the folder without a restart (reloading every texture, model, sound and map).
