# 0054: The game ships its own maps in content/maps

- Status: Accepted
- Date: 2026-11-14
- Affects: `content/maps/`, `content/LaunchScript/*.cfg`, `CMakeLists.txt`, `game/src/Paths.cpp` (`listMapNames`, `mapFileForRead`), `.gitignore`, `.dockerignore`; amends [ADR 0016](0016-player-data-lookup-and-package-content.md) and [ADR 0053](0053-three-public-servers-and-owner-defaults.md); ROBO-214

## Context
- The repo held no real maps: original BV2 maps are blocked by hash ([ADR 0010](0010-bv2-asset-compat-and-gui-freeze.md)), so the server image and the public servers played generated placeholder maps.
- On 2026-11-14 the owner made ten maps (six CTF/TDM, four free for all) and asked for them on the public servers. The owner is their author, so they ship under the repo licence without the map submission form.
- [ADR 0016](0016-player-data-lookup-and-package-content.md) reads maps only from the data root. With `BV2_DATA_DIR` or the player's BV2 copy as the data root, maps in the package would be invisible.

## Decision
- The owner's maps are tracked in `content/maps/` (BV2 format 20202, unchanged). The build copies them into `main/maps` beside the placeholders, so every package and the server image ship them.
- `bv2::listMapNames` lists the data root's `main/maps`, the pref dir's `maps` and the package's `main/maps`. `bv2::mapFileForRead` reads the pref dir first, then the data root, then the package. A player's or server owner's map of the same name wins over the package's.
- The CTF and TDM launch scripts rotate the six CTF/TDM maps, the FFA script the four `DM-` maps. Tests that rely on the placeholder layout (`bot_sim`, e2e `ctf_capture`) set the placeholder map themselves.
- `tests/test_content_maps.py` checks the maps: valid names, spawns and flag pods on open connected cells, flag pods at height 0, and launch scripts naming only these maps.

## Consequences
- Flag pods must be at z 0. `ServerCTF.cpp` takes a flag within 0.25 of its pod with the player at z 0, so a pod at z 0.25 (the owner's map generator wrote that) can never be taken.
- `content/maps/` is no longer in `.gitignore` or `.dockerignore`. `tools/check-original-assets.py` still refuses original maps by hash.
- Map file names follow the repo's no-space rule (`tools/check-hygiene.py`), so the owner's "Thunder World" ships as ThunderWorld.
- Rejected: copying the maps into the pref dir at start (writes user data the player didn't make), and maps only on the server (the client's Host tab would not list them).
