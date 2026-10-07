# Robo Violence 2
Robo Violence 2 is an unofficial fork of BaboViolent 2 by bitHeads / RndLabs
(GPLv3 source release): <https://github.com/Daivuk/BaboViolent2>
Not affiliated with or endorsed by the original authors.

Robots will use this repo as a staging ground before winning AI war ;-)

## Original assets removed
The original game assets (maps, models, textures, skins, fonts, sounds, music,
source art), the non-English translations and the RndLabs EULA are NOT in this
repository. The GPL covers code only; no grant exists for them, so they are not
redistributed. Replacements will be created by this project.
- `tools/check-original-assets.py`: fails if an original file is committed again

Upstream history still contains the originals; this fork's `main` does not.

## Running without the original assets - temporary
The game needs a data directory `main/` next to the executable.

- If you own the original game, keep its data **outside** the repo (for example
  `~/Games/bv2-data/main/`) and point the game at it with `BV2_DATA_DIR`
  (implemented in step 4 §4.4; until then, symlink `main/` into the run dir).
  Never commit it; `.gitignore` blocks the original asset folders.
- Without the original data (CI, contributors), generated placeholder content is
  used (step 2 §2.6; not yet available).

Only `languages/en.lang` and `LaunchScript/` remain in `content/`.

## License
Code: GPLv3 ([LICENSE.txt](LICENSE.txt)). 

Assets and the name "BaboViolent 2" are not covered.

## Platforms and build

Targets: Windows 11, macOS 12+ (arm64) and Linux x64, built with CMake, Ninja and vcpkg. Download a package from CI or build from source: [docs/build/](docs/build/README.md) ([macOS](docs/build/macos.md), [Linux](docs/build/linux.md), [Windows](docs/build/windows.md)).

## Where to read more
- [AGENTS.md](AGENTS.md): project rules for fellow robots, agents and even human contributors
- [ARCHITECTURE.md](ARCHITECTURE.md): architecture index
- [docs/roadmap/](docs/roadmap/README.md): roadmap, one folder per phase (Phase A done, Phase B planned)
- [docs/analysis/](docs/analysis/README.md): original bv2 code analysis v1
