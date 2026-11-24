# ROBO-260: The proprietary art pack downloads in the background and stacks over the base pack

- Status: Accepted
- Date: 2026-11-24
- Affects: `game/src/ArtPack.cpp`, `game/src/ArtDownloader.cpp`, `bv2::artPackDir` in `game/src/Paths.cpp`, `bv2::setAssetPacks` in `game/src/AssetRegistry.cpp`, `game/src/main.cpp`, `content/base-pack.tsv` (`textures/splash`); ROBO-260 (owner decisions 2026-11-24); amends [0069](0069-asset-packs-stack-over-the-base-pack.md)

## Context
- The new art and the name are proprietary (`LicenseRef-RV2-Art`); the code and the placeholder base pack stay GPL-3.0-or-later. The public repo and the source tarball must build a complete game without the art.
- Owner, 2026-11-24: the art downloads automatically on every OS, with no question; the first run shows a free splash and opens the menu with the art, with no restart; a failure never blocks play. Hosting: a new public repo `Hub-Tansky/rv2-art` under the art licence, one release per art version.
- ADR 0040 already downloads the BV2 data with the OS's `curl` and `tar`/`unzip`, checks hashes and installs by rename.

## Decision
- The art pack is a pack (ADR 0065) with its own `main/pack.tsv`, published as `rv2-art-<version>.tar.gz` on the `rv2-art` repo's release `rv2-art-<version>`. Each game build pins one version, its URL and the archive's SHA-256 (`game/src/ArtPack.cpp`); an empty pin downloads nothing.
- Install folder: `artPackDir()`, `robo_art` beside `original/` (inside `BV2_PREF_DIR` when set). A `VERSION` file there names the installed version; only the pinned version is stacked.
- At start, before any asset loads, the client stacks `robo_art` (when it holds the pinned version) over the data root over the base pack. `setAssetPacks` takes the index of the working-directory pack, since the art pack now sits above it.
- When the pinned version isn't installed, a detached thread runs `curl` into `robo_art.partial`, checks the archive's SHA-256 before unpacking, runs `tar -xzf`, requires `main/pack.tsv`, writes `VERSION` and renames the folder into place (ADR 0040's `replaceDir`).
- After the GL context exists and before the lobby, console and scene load assets, `artSplashWait(60)` shows `textures/splash` (free, in the base pack) with a thin progress line while the download runs. It ends when the pack is installed (then restacks), after 60 s, or on Esc or window close. Assets load once, after the stack is final, so this is not the hot reload ADR 0042 rejected.
- A download still running after the splash continues in the background and is used on the next start; a failure is logged and retried on the next start.
- `--no-art-download` turns the download off and is remembered as `<prefDir>/art-download-off`. `--placeholders` (base pack only, used by every test and screenshot) also skips it, not remembered, so CI never fetches the art.

## Consequences
- The pin changes with each art release (an art drop PR); the first is `rv2-art-1` (4 map looks, HUD textures, sky, splash). The archive is built reproducibly by the art repo's `tools/build-release.sh`. A build with an empty pin behaves as before this ADR.
- Packages and the source tarball never contain the art; licence files in packages are a separate PR (`packaging/`, HIGH tier).
- The Options entry to turn the download off, the per-file version fallback and the real splash image follow in later PRs of ROBO-260.
- Rejected: a first-run button (owner chose automatic); restarting after the download (owner: no restart); per-file hashes compiled in (the archive hash covers every file and needs one line per release); unpacking before the hash check (a hostile archive could write outside the folder).
