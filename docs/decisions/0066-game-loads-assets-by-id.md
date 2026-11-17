# 0066: Game code loads assets by id; the base manifest's ids are compiled in

- Status: Accepted
- Date: 2026-11-17
- Affects: `game/src/AssetRegistry.h`, `game/src/AssetRegistry.cpp`, `game/CMakeLists.txt` (generates `BasePackIds.cpp`), every loader call in `game/src`; ROBO-232 (refactor Phase 1 A2: ROBO-242, ROBO-243, ROBO-244; [ADR 0062](0062-asset-packs-mode-modules-client-independent-server.md) asset seam, rule R1)

## Context
- [ADR 0065](0065-asset-pack-manifest.md) gives every asset an id in `content/base-pack.tsv`. Game code still named files: about 170 `"main/..."` literals in `dktCreateTextureFromFile`, `dksCreateSoundFromFile`, `dkoLoadFile`, `dkfCreateFont` and `dksPlayMusic` calls.
- The original BV2 data, which players still use, has no `pack.tsv`.

## Decision
Game code asks for an asset by id: `bv2::assetPath("models/Rocket")` returns `"main/models/Rocket.DKO"`, the path the `dk*` loaders open. The id and file columns of `content/base-pack.tsv` are compiled in: CMake writes them to `BasePackIds.cpp` at configure time and re-runs when the manifest changes.

## Consequences
- The original BV2 data and the placeholders resolve the same ids, with nothing to read at startup and no new failure when a data folder lacks `pack.tsv`.
- An id the manifest lacks is reported once on stderr and resolves to `main/<id>`, which no loader opens, so the game behaves as for a missing file. `test_asset_ids` fails when game code asks for a literal id the manifest lacks; `test_asset_registry` covers the lookup.
- Changing a file's format (`.tga` to `.png`) is a manifest edit, not a code edit.
- The migration goes by asset type: fonts, music and models first (ROBO-242), then textures (ROBO-243) and sound effects (ROBO-244). Ids built at runtime (map themes) come from `bv2::themeTextureId` and the theme and texture names in `game/src/MapThemes.h`, which `test_asset_registry` checks against the manifest. Skins and map files are not manifest rows and keep their paths.
- Rejected: reading `main/pack.tsv` at runtime (the BV2 data has none, so it would need the compiled-in table anyway; runtime manifests come with pack stacking, ROBO-234).
