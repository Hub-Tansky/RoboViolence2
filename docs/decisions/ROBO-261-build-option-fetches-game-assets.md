# ROBO-261: `RV2_ASSETS` fetches game assets at configure time into the build's runtime/packs/

- Status: Accepted; amended by [ROBO-278](ROBO-278-drop-bv2-asset-compatibility.md): `original` is unsupported, removed by ROBO-279; `RV2_*` names move to `ROBO_*` with ROBO-282
- Date: 2026-11-25
- Affects: `cmake/RV2Assets.cmake`, `tools/safe-extract.py`, `CMakeLists.txt` (cache variables, install check), `packaging/make-package.py` (stage check), `bv2::buildPacksDir` and `detectDataDir` in `game/src/Paths.cpp`, `bv2::assetPackStack` in `game/src/ArtPack.cpp`, `game/src/ArtDownloader.cpp`, `tests/assets_fetch.cmake`; ROBO-261 (owner request 2026-11-24); builds on [0040](0040-first-run-downloads-original-data.md), [0065](0065-asset-pack-manifest.md), [ROBO-260](ROBO-260-art-pack-downloads-beside-the-gpl-game.md)

## Context
- A source build runs on placeholders until the client downloads the art pack (ADR ROBO-260) or the player fetches the original data (ADR 0040). The owner wants real assets in the build folder without that step.
- Owner, 2026-11-24: one variable with the RV2_ prefix, `RV2_ASSETS` = `robo_art` (default), `original`, `url` or `none`, plus `RV2_ASSETS_URL`; offline never fails the build; packages never carry the original data.
- CMake 3.25 (the project minimum) downloads with `file(DOWNLOAD)`; `cmake -E tar` unpacks the hash-pinned `robo_art`, and Python 3 (already needed for the placeholders) unpacks the others safely, so no new tool or library.

## Decision
- **Variables:** `RV2_ASSETS` is a list of `robo_art`, `original`, `url`, `none` (default `robo_art`, or `none` when the environment has `CI`, which GitHub Actions sets). `RV2_ASSETS_URL` is the archive for `url`. `RV2_ASSETS_CACHE` is the download cache, outside the build tree (`~/.cache/roboviolence2/assets`, `~/Library/Caches/RoboViolence2/assets`, `%LOCALAPPDATA%\RoboViolence2\assets-cache`).
- **When:** at configure time, so `cmake --preset` is enough and the stamp in each pack (kind, version, hash or URL) makes a reconfigure with the same pin do nothing. `game/src/ArtPack.cpp` and `DataDownload.cpp` are configure dependencies, so a pin bump fetches again.
- **Where:** `runtime/packs/<kind>/` (`robo_art` with `VERSION`, `original`, `url`), beside `runtime/main/`. The game finds it as `packs/` beside the real folder of its package's `main/` (`buildPacksDir`), which also covers the macOS development bundle, whose `Resources/main` links to `runtime/main`. Install rules copy only `runtime/main`, so these packs never reach an install or package.
- **Per kind:**
  - `robo_art`: the pin (`RV2_ART_PACK_URL`, `_SHA256`, `_VERSION`) is read from `game/src/ArtPack.cpp`; the archive's SHA-256 is checked before unpacking; it needs `main/pack.tsv`. The client stacks it like its own download and skips the download.
  - `original`: the upstream zip at `BV2_UPSTREAM_COMMIT` (read from `DataDownload.cpp`); only `Content/main` is unpacked, and every `Content/main` row of `tools/original-assets.sha256` must match, or nothing is installed. `detectDataDir` looks in `packs/original` right after the pref dir's `original`.
  - `url`: any `.tar.gz` or `.zip` holding `main/pack.tsv` at its top or one folder down, which `tools/check-pack.py` must pass. It has no pin, and a warning says so. The client stacks it on top.
- **Stack:** placeholders < original < robo_art < url, the client's order. `--placeholders` (every test and golden) leaves the build's packs out, so local tests give the same frames with or without them.
- **Failures:** a failed download, wrong hash, broken archive or failed check warns and installs nothing for that kind; the configure step goes on. A kind dropped from `RV2_ASSETS` is removed from `runtime/packs`.
- **Packages:** `cmake --install` (which the `.deb` uses) ends with a check that no file it wrote (`CMAKE_INSTALL_MANIFEST_FILES`) matches a hash in `tools/original-assets.sha256`; `packaging/make-package.py`, which copies `runtime/main` for the `.zip`, `.tar.gz`, `.dmg` and the Windows installer, checks its stage the same way. A match fails the package job.

## Consequences
- A developer's first configure downloads the art pack (13 MB for pack 2), once per pin. CI builds fetch nothing; `tests/assets_fetch.cmake` (ctest `assets_fetch`) runs the fetch against local fixtures in every CI job instead of a separate job: robo_art with its cache, wrong hash, unreachable URL, url with and without `pack.tsv`, a broken archive, original data that fails the hash list, combined kinds, `none`, and the install check; `package_originals` covers the package builder's check.
- `url` and `original` archives have no archive hash, so they unpack through `tools/safe-extract.py`, which refuses the whole archive when a member is absolute, has a `..` segment or is a link (`cmake -E tar` would write outside the folder); `original` then keeps only the files its hash list names. These kinds need Python 3. A `url` pack's content is trusted as served, redirects included, and its cache keys on the URL; the client never sees a URL.
- Rejected: fetching at build time through a custom target (every build runs it, and offline builds would need care in each generator); copying packs into `runtime/main` (installs would ship them and the goldens would change); a separate CI job for the smoke (the ctest runs on every OS already).
