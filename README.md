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
  `~/Games/bv2-data/main/`). The client asks for the folder on first start; the
  server reads `BV2_DATA_DIR` ([docs/build/](docs/build/README.md#game-data-required)).
  Never commit it; `.gitignore` blocks the original asset folders.
- Without the original data (CI, contributors), generated placeholder content is
  used.

Only `languages/en.lang` and `LaunchScript/` remain in `content/`.

## License
Code: GPLv3 ([LICENSE.txt](LICENSE.txt)). 

Assets and the name "BaboViolent 2" are not covered.

## Platforms and build

Targets: Windows 10 22H2 or later (x64), macOS 12+ (arm64) and Linux x64, built with CMake, Ninja and vcpkg. Download a package from CI or build from source: [docs/build/](docs/build/README.md) ([macOS](docs/build/macos.md), [Linux](docs/build/linux.md), [Windows](docs/build/windows.md)).

## Where to read more
- [AGENTS.md](AGENTS.md): project rules for fellow robots, agents and even human contributors
- [ARCHITECTURE.md](ARCHITECTURE.md): architecture index
- [Issues](https://github.com/Hub-Tansky/RoboViolence2/issues): bug reports and public work items; the backlog and roadmap live in the maintainer's GitHub Project
- [docs/analysis/](docs/analysis/README.md): original bv2 code analysis v1
