# RoboViolence 2
RoboViolence 2 is an unofficial fork of BaboViolent 2 by bitHeads / RndLabs
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
Targets: Windows 11, macOS 12+ (arm64) and Linux x64, built with CMake, Ninja and vcpkg.

```bash
cmake --preset linux-x64        # or macos-arm64, win-x64-msvc (from a Developer PowerShell)
cmake --build --preset linux-x64 --target bv2dedicated bv2master
```

- Needs `VCPKG_ROOT`, CMake 3.25+, Ninja, Python 3. Linux host packages: `tools/setup-dev.sh --linux-packages`.
- The build creates `build/<preset>/runtime/` with the executables, `main/` (languages, launch scripts, `bv2.example.cfg` from `config/bv2.example.cfg`, generated placeholder assets) and the databases generated from `content-seed/`. Run the server from there: `./bv2dedicated`, then `execute CTF`. Saved config, databases and logs go to the per-user pref dir (`BV2_PREF_DIR` overrides).
- `bv2` (the client) is part of `all`: SDL3 window and input, miniaudio audio, glad GL 2.1. The `client` vcpkg feature supplies them; `-DBV2_BUILD_CLIENT=OFF` builds the servers only.
- Run `ctest --test-dir build/<preset> --output-on-failure` for the tests (netPacket layout, config, dedicated-server smoke). CI builds all three OSes; see [AGENTS.md](AGENTS.md).
- Windows 11: install Visual Studio Build Tools (C++), Ninja, CMake and vcpkg, then run the preset from a Developer PowerShell. macOS: `brew install cmake ninja autoconf autoconf-archive automake libtool`.
- Run `tools/setup-dev.sh` (or `.ps1`) once to activate the commit hooks.

## Where to read more
- [AGENTS.md](AGENTS.md): project rules for fellow robots, agents and even human contributors
- [ARCHITECTURE.md](ARCHITECTURE.md): architecture index
- [docs/refactoring/](docs/refactoring/README.md): step-by-step refactoring plan
- [docs/analysis/](docs/analysis/README.md): code analysis
- [config/README.md](config/README.md): config and secrets policy
