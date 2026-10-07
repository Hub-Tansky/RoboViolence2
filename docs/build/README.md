# Building and running Robo Violence 2

Pick your OS: [macOS](macos.md), [Linux](linux.md), [Windows](windows.md). Each guide covers building from source and running a downloaded package.

## Download or build

- **Download:** every CI run of the `build` workflow on GitHub (Actions → build → a green run → Artifacts; you need to be signed in to GitHub, and artifacts expire after 90 days) publishes one package per OS: `roboviolence2-win-x64-msvc.zip`, `roboviolence2-macos-arm64.zip` and `roboviolence2-linux-x64.tar.gz`. Each holds one folder with the client, `bv2dedicated`, `bv2master`, `main/`, `master.db`, `web.db` and run scripts. The builds are unsigned; each guide says how to get past the OS warning.
- **Build:** CMake 3.25+, Ninja, Python 3 and vcpkg. The first build is slow because vcpkg compiles SDL3, miniaudio and SQLite; later builds reuse them.

## Game data (required)

- Playing needs your own copy of the original BaboViolent 2 data; it is never in this repository or the packages. Find the folder that contains `main/` (with `maps/`, `textures/`, `sounds/`, `fonts/`) and set `BV2_DATA_DIR` to it permanently, as each OS guide shows, before the first game. Pointing it at `main/` itself also works. Building and the tests work without it.
- The client and the game server both read `BV2_DATA_DIR`: the server loads its maps and launch scripts from there and sends missing maps to clients.
- Without it the game falls back to generated placeholder maps, textures and sounds. That is only for CI and development: it looks rough, has no music, and doesn't match other players' maps.
- Saved settings, `bv2.db`, logs and downloaded maps go to the per-user pref dir ([ADR 0007](../decisions/0007-data-root-pref-dir-config-layers.md)); `BV2_PREF_DIR` overrides it:

  | OS | Pref dir |
  |---|---|
  | Windows | `%APPDATA%\BaboViolent2\bv2` |
  | macOS | `~/Library/Application Support/BaboViolent2/bv2` |
  | Linux | `${XDG_DATA_HOME:-~/.local/share}/BaboViolent2/bv2` |

## Playing a local game

Run each program from its folder (a package folder, or `build/<preset>/runtime/`). Ports: game server TCP 3333, master TCP 10207.

| Program | Package | Arguments |
|---|---|---|
| Master server | `run-master` | none; only needed for the server browser, with `BV2_MASTER_SERVERS` (below) |
| Game server | `run-server` | launch script name, default `CTF` (`main/LaunchScript/CTF.cfg`). The default applies only with no arguments: with `--config <file>`, also name the script |
| Client | `run-client` | one console command, e.g. `connect 127.0.0.1` |

- To list the server in the client's server browser, start the master first and set `BV2_MASTER_SERVERS=127.0.0.1:10207` for both the server and the client.
- Players on other machines use the server's LAN IP instead of `127.0.0.1`.
- The server console reads commands from stdin (`maplistall`, `changemap <map>`, `quit`).
- Full test script: [Phase A manual test](../roadmap/phase-a-modern-portable-build/phase-a-manual-test.md).

## Tests

```bash
ctest --test-dir build/<preset> --output-on-failure
```

Runs the netPacket layout, config, file IO and dedicated-server smoke tests.
