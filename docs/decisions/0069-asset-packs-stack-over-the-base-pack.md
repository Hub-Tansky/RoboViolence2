# 0069: Asset packs stack over the base pack; the BV2 data is an optional pack on top

- Status: Accepted; amended by [ROBO-264](ROBO-264-art-pack-checks-and-fixture.md); amended by [ROBO-260](ROBO-260-art-pack-downloads-beside-the-gpl-game.md)
- Date: 2026-11-18
- Affects: `game/src/AssetRegistry.h`, `game/src/AssetRegistry.cpp`, `game/src/BasePackIds.h`, `game/src/BasePackIds.cpp.in`, `game/CMakeLists.txt`, `game/src/main.cpp`; ROBO-234 (refactor Phase 1 A4, [ADR 0062](0062-asset-packs-mode-modules-client-independent-server.md) asset seam); succeeds [0010](0010-bv2-asset-compat-and-gui-freeze.md) for asset loading; builds on [0065](0065-asset-pack-manifest.md), [0066](0066-game-loads-assets-by-id.md) and [0042](0042-game-assets-option-remembers-placeholders.md)

## Context
- ADR 0010 kept the game on the original BV2 data until replacement assets ship: one data folder, chosen by `BV2_DATA_DIR`, Options > Game assets or the first-run box, else the package's placeholders.
- ADR 0065 made the package's `main/` the base pack, described by `content/base-pack.tsv`; ADR 0066 made game code ask for assets by id. The replacement art the owner is making lands in the base pack, row by row.
- The BV2 data has no `pack.tsv`, uses the base pack's file names (the manifest was derived from them) and lacks `textures/sky.tga`, which the code loads (it also lacks `textures/sniperScope.tga`, a manifest row the code draws itself).

## Decision
- The base pack is the package's `main/` and is always at the bottom of the stack. The data folder the player chose (`BV2_DATA_DIR`, Options > Game assets, the first-run download) is stacked on top of it; with no folder chosen the base pack plays alone. Client and dedicated server set the stack after the data root is known (`bv2::setAssetPacks`).
- An id resolves to the first pack, from the top, whose file for it exists. A pack's `main/pack.tsv` names its files and may add ids; a pack without one (the BV2 data) uses the base pack's file names. The base pack's ids come from the compiled-in manifest, not from its copied `pack.tsv`.
- Under a pack without `pack.tsv`, a base file whose manifest source is `generated placeholder` does not show through: the BV2 data plays as it did where it lacks a file. The base pack's real art does show through, so new art and new ids reach players on the BV2 data too.
- Paths in the top pack stay `main/<file>` (the process works in the data root, ADR 0007), lower packs give `<root>main/<file>`, and an id no pack holds keeps the old missing path `main/<file>`, so loaders fail as before.
- Options > Game assets keeps its layout and meaning (ADR 0042, owner decision D1): a folder with BV2 data is the pack stacked on top, an empty field means the base pack alone.

## Consequences
- Supersedes ADR 0010's "the game loads original BV2 data unchanged until replacement assets ship" for assets: the BV2 data is now an optional pack. Its other rules (no GUI change without approval, new formats beside old ones) stay. On the BV2 data nothing changes today: every base row is a placeholder, and a run of CTF-Amazon on base and head gives identical logs, an identical menu frame and an in-game frame within run-to-run noise.
- When the owner replaces a row with art (`source` no longer `generated placeholder`), players on BV2 data get that file where their data lacks it, e.g. a real `textures/sky`.
- Only two levels exist today (the chosen folder over the base pack). A list of packs (mods, map packs) is the same call with more roots and needs a place to choose them, which is new GUI and needs the owner's approval.
- Maps, skins, launch scripts and languages are not manifest rows and keep their own lookups (ADRs 0016, 0054).
- `test_asset_registry` covers the stack: top pack, fall-through, a `pack.tsv` override and added id, a missing file falling through, and placeholders hidden only under the BV2 data. The e2e join and capture scenarios run on the base pack alone (placeholders).
- Rejected: hiding every base file under the BV2 data (new art would never reach those players); showing placeholders through it (a flat placeholder sky changed the floor reflections on CTF-Amazon by about 313,000 px).
