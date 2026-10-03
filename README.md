# RoboViolence 2

RoboViolence 2 is an unofficial fork of BaboViolent 2 by bitHeads / RndLabs
(GPLv3 source release): <https://github.com/Daivuk/BaboViolent2>
Not affiliated with or endorsed by the original authors.

## Original assets removed

The original game assets (maps, models, textures, skins, fonts, sounds, music,
source art), the non-English translations and the RndLabs EULA are NOT in this
repository. The GPL covers code only; no grant exists for them, so they are not
redistributed. Replacements are created by this project.

- [docs/ASSETS-LICENSE.md](docs/ASSETS-LICENSE.md): policy and replacement register
- [docs/assets/ASSET-INVENTORY.md](docs/assets/ASSET-INVENTORY.md): every removed file, where it is used, what to recreate
- `tools/check-original-assets.py`: fails if an original file is committed again

Upstream history still contains the originals; this fork's `main` does not.

## Running without the original assets

The game needs a data directory `main/` next to the executable.

- If you own the original game, keep its data **outside** the repo (for example
  `~/Games/bv2-data/main/`) and point the game at it with `BV2_DATA_DIR`
  (implemented in step 4 §4.4; until then, symlink `main/` into the run dir).
  Never commit it; `.gitignore` blocks the original asset folders.
- Without the original data (CI, contributors), generated placeholder content is
  used (step 2 §2.6; not yet available).

Only `languages/en.lang` and `LaunchScript/` remain in `BaboViolent2/Content/main/`.

## License

Code: GPLv3 ([LICENSE.txt](LICENSE.txt)). Assets and the name "BaboViolent 2" are not covered.

## Platforms and build

Targets: Windows 11, macOS and Linux (client and dedicated server), built with
CMake and vcpkg (Step 2 onwards). Until then:

- Linux dedicated server: run `make` in the repository root (needs sqlite3, libcurl
  and GLU dev packages; no OpenSSL). The Windows solution and old project files were removed in Step 1.
- There are no automated tests.
- Run `tools/setup-dev.sh` (or `tools/setup-dev.ps1`) once to activate the commit hooks.

## Where to read more

- [AGENTS.md](AGENTS.md): project rules for agents and contributors
- [ARCHITECTURE.md](ARCHITECTURE.md): architecture index
- [docs/refactoring/](docs/refactoring/README.md): step-by-step refactoring plan
- [docs/analysis/](docs/analysis/README.md): code analysis
- [config/README.md](config/README.md): config and secrets policy
