# Building and running Robo Violence 2

Pick your OS: [macOS](macos.md), [Linux](linux.md), [Windows](windows.md). Each guide covers building from source and running a downloaded package.

## Download or build

- **Download:** every CI run of the `build` workflow on GitHub (Actions → build → a green run → Artifacts; you need to be signed in to GitHub, and artifacts expire after 90 days) publishes one package per OS: `roboviolence2-win-x64-msvc.zip`, `roboviolence2-macos-arm64.zip` and `roboviolence2-linux-x64.tar.gz`. Each holds one folder with the client, `bv2dedicated`, `bv2master`, `main/`, `master.db`, `web.db`, run scripts and licences; Windows also gets `-setup.exe`, macOS a `.dmg`, Linux a `.deb` (client and `bv2dedicated` only, [ADR 0031](../decisions/0031-linux-deb-package.md)). Tagged releases (`v*`) publish the installer, the `.dmg` (only the app, [ADR 0030](../decisions/0030-app-icon-and-dmg-layout.md)), the macOS and Linux packages, the `.deb` and the source on the [releases page](https://github.com/Hub-Tansky/RoboViolence2/releases) ([ADR 0020](../decisions/0020-package-and-release-format.md)). Packages are Release builds and start fullscreen; the presets are Debug and start windowed; `<preset>-release` (e.g. `macos-arm64-release`) builds what the packages ship. The builds are unsigned; each guide says how to get past the OS warning.
- **Build:** CMake 3.25+, Ninja, Python 3 and vcpkg. The first build is slow because vcpkg compiles SDL3, miniaudio and SQLite; later builds reuse them.

## Game data (required)

- Playing needs your own copy of the original BaboViolent 2 data: the folder that contains `main/` (with `maps/`, `textures/`, `sounds/`, `fonts/`). It is never in this repository or the packages. Building and the tests work without it.
- The client finds it in this order ([ADR 0016](../decisions/0016-player-data-lookup-and-package-content.md)):
  1. `BV2_DATA_DIR` (the folder or its `main/`).
  2. The folder saved on an earlier start (`data-dir.txt` in the pref dir).
  3. `original` beside the pref dir (installed by step 4), then `BaboViolent2-master` or `BaboViolent2` (the upstream repository, unzipped, also with the extra level Windows Extract All adds) in `Downloads`, `Desktop` or `Documents`; on Windows also `BaboViolent 2` or `BaboViolent2` in Program Files.
  4. On first start, a box offers "Download and play": the OS's `curl` and `tar`/`unzip` fetch the upstream data, which is checked against `tools/original-assets.sha256` and saved ([ADR 0040](../decisions/0040-first-run-downloads-original-data.md)). "Use placeholders" (or `--placeholders`) skips it.
- The game server reads only `BV2_DATA_DIR`: it loads its maps from there and sends missing maps to clients.
- Launch scripts, language files and `bv2.example.cfg` always come from the package's own `main/`.
- Without data the game uses generated placeholder maps, textures and sounds. That is only for CI and development: it looks rough, has no music, and doesn't match other players' maps.
- Saved settings, `bv2.db`, logs and downloaded maps go to the per-user pref dir ([ADR 0007](../decisions/0007-data-root-pref-dir-config-layers.md)); `BV2_PREF_DIR` overrides it:

  | OS | Pref dir |
  |---|---|
  | Windows | `%APPDATA%\BaboViolent2\bv2` |
  | macOS | `~/Library/Application Support/BaboViolent2/bv2` |
  | Linux | `${XDG_DATA_HOME:-~/.local/share}/BaboViolent2/bv2` |

## Playing a local game

Run each program from its folder (a package folder, or `build/<preset>/runtime/`). Ports: game server TCP 3333 (and UDP 3333 for the server browser's ping), master TCP 10207.

| Program | Package | Arguments |
|---|---|---|
| Master server | `run-master` | none; only needed to test with a local master, with `BV2_MASTER_SERVERS` (below) |
| Game server | `run-server` | launch script name, default `CTF` (`main/LaunchScript/CTF.cfg`). The default applies only with no arguments: with `--config <file>`, also name the script |
| Client | `run-client` | one console command, e.g. `connect 127.0.0.1` |

- Server and client use the public master `master.roboviolence.com:10207` by default ([ADR 0028](../decisions/0028-built-in-public-master.md)), so a local server shows up there after 20 s. To test with a local master, start it first and set `BV2_MASTER_SERVERS=127.0.0.1:10207` for both the server and the client; `BV2_MASTER_SERVERS=none` keeps a server off every master.
- Players on other machines use the server's LAN IP instead of `127.0.0.1`.
- The server console reads commands from stdin (`maplistall`, `changemap <map>`, `quit`).
- Full test script: [Phase A manual test](https://github.com/Hub-Tansky/RoboViolence2/blob/58c91b2a660db2b290b0b5f504381980a6aedba2/docs/roadmap/phase-a-modern-portable-build/phase-a-manual-test.md).

## Tests

```bash
ctest --test-dir build/<preset> --output-on-failure
```

Runs the netPacket layout, config, file IO and dedicated-server smoke tests.
