# ROBO-259: Four map looks; the 23 theme ids are aliases

- Status: Accepted
- Date: 2026-11-24
- Affects: `game/src/MapThemes.h`, `game/src/Map.cpp` (`reloadTheme`, `reloadWeather`), `game/src/Editor.cpp`, `content/base-pack.tsv`; ROBO-259 (owner decisions 2026-11-24)

## Context
- Map files store a theme id (0 to 22, `game/src/Map.h:37`) that names a folder under `textures/themes/` and picks the weather.
- The new art has four looks: Wasteland, Foundry, Winter Coming, Scrap Deck. The owner chose aliasing over rewriting maps, Wasteland as the fallback, and one weather per look.
- Ground rule 1 (ADR 0010): the game must keep running on the BV2 data until the new art ships.

## Decision
- `bv2::themeLook` maps each id to a look by the owner's table (`game/src/MapThemes.h`); an id outside 0 to 22 gets Wasteland. Map files are never rewritten.
- Textures load from the look folder (`wasteland`, `foundry`, `winter-coming`, `scrap-deck`). If that load fails, they load from the map's own old folder. Under the BV2 data the look's generated placeholder doesn't show through (ADR 0069), so BV2 players still see the BV2 theme textures until the art pack holds the look folders.
- Weather follows the look: Wasteland wind (`sounds/wind`), Foundry lava sound, Winter Coming snow, Scrap Deck none. The water reflection and lava smoke follow the look too.
- The editor's Theme button cycles the four looks and writes an old id per look (sand, lava, winter, grass), so the map format and older builds are unchanged.
- The snow ice slide (`sv_slideOnIce`) stays on the `snow` id only: it is gameplay, and the server must not change it with the look.

## Consequences
- The base pack keeps the 69 old theme rows (BV2 data fallback) and adds 12 look rows; the old rows go when BV2 data support ends.
- `reloadTheme` no longer overwrites `theme` with grass for an unknown id or `cl_grassTextureForAllMaps`; ice slide, lava smoke and reflection follow the map's real id or look in both cases.
- Fog and rain weathers are no longer picked by any map; their code stays.
- A map saved by the editor on a look keeps a valid old id, so an older build shows its BV2 theme.
- Rejected: new ids 23 to 26 (older builds would fall back to grass); migrating map files (owner chose alias).
