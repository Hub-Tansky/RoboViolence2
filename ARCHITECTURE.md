# Architecture

Robo Violence 2: unofficial fork of BaboViolent 2, a top-down multiplayer shooter (C++, GPLv3). State: Phases A and B done; Phase C done, first release pending; Phase T steps 1–3 done. Backlog: `40-backlog/` in the private vault repo [rv2-docs](https://github.com/Hub-Tansky/rv2-docs) (ADR 0044); public bug intake: [issues](https://github.com/Hub-Tansky/RoboViolence2/issues).

## Summary

| Topic | Fact |
|---|---|
| Deliverables | `bv2` (client), `bv2dedicated` (headless server, `CONSOLE`) and `bv2master`. CI builds and tests all three on Windows, macOS and Linux; the client runs 10 s under xvfb (ASan). Real games on each OS: not yet verified (Phase B step 2). |
| Build | CMake 3.25+, Ninja presets (`CMakePresets.json`), vcpkg manifest. Output in `build/<preset>/runtime/` ([ADR 0005](docs/decisions/0005-runtime-main-data-root.md)). |
| Platforms | Targets: Linux x64, macOS 12+ arm64, Windows 10 22H2 or later x64 ([ADR 0012](docs/decisions/0012-windows-10-22h2-minimum.md)); Windows API level pinned to Windows 10 20H1 (`NTDDI_VERSION`). `engine/zeven/include/platform.h` defines `BV2_PLATFORM_*`, `BV2_POSIX`; CMake force-includes it. |
| Modules | `game` (client, server, editor); `engine/babonet` networking (`bb_*`, `CThread` on `std::thread`); `engine/zeven` utilities (`zeven_core`: `dkc dksvar` + `CString/CVector/CMatrix`; `zeven_console`: no-op `dkt` for the server; `zeven_client`: `dkw dki dkgl dkt dkf dkp dks`); `engine/dko` model loader; `engine/credentials` password HMAC proofs (libsodium); `masterserver`. |
| Dependencies | vcpkg (`vcpkg.json`, pinned baseline): sqlite3, libsodium (ADR 0014); feature `client`: sdl3, miniaudio, stb; feature `http`: curl (off, ADR 0002). Generated, committed: glad GL 2.1 (`engine/zeven/third_party/glad`). System: GLU. No libcurl or OpenSSL is linked by default. |
| Platform layer | SDL3 window and input (`dkw`, `dki`), miniaudio (`dks`), glad ([ADR 0006](docs/decisions/0006-sdl3-miniaudio-glad-platform-layer.md)). One client `main()` for all OSes. |
| Tick | Fixed 30 Hz: every `update(float delay)` gets `1/30`; "frames" are a time unit (30 = 1 s). |
| Network | TCP, raw structs `memcpy`'d from `game/src/netPacket.h`. The server is authoritative for hits, damage, spawns, projectiles and flags; clients for their own movement. `playerID` (slot) differs from `babonetID` (connection). Protocol `GAME_VERSION_SV/CL` = 22100 (`game/src/Server.h:35`). Join and admin passwords are HMAC proofs over a per-connection challenge; master bans are HMAC-signed packets (`engine/credentials/include/Credentials.h`, ADR 0014, 0038). |
| Variants | `CONSOLE` = headless server (explicit file list in `game/CMakeLists.txt`). Direct3D, non-Pro and VLD code were removed in Step 1. |
| Config and secrets | Tracked: `config/*.example.cfg` with empty secrets, `content-seed/*.sql` for generated DBs. Real values stay local; gitleaks, hooks and CI enforce it ([config/README.md](config/README.md)). |
| Assets | The original assets are removed and blocked by hash (`tools/check-original-assets.py`). Only `content/languages/` (English strings, template, guide), `content/LaunchScript/`, `content/bots/` (bot names) and `content/maps/` (the owner's own maps, not BV2's) remain; the build generates placeholders (`tools/gen-placeholder-content.py`) from the base pack manifest `content/base-pack.tsv` (id, file, author, licence per asset; ADR 0065), copied to `main/pack.tsv` and checked by `tools/check-pack.py`. |
| Encoding | UTF-8 without BOM, LF; `tools/check-encoding.py`. Some comments hold U+FFFD where upstream lost accents. |
| Supply chain | Dependency graph fed by vcpkg (`dependency-graph.yml`, on push to `main`); Dependabot alerts and security updates on; Dependabot version updates for GitHub Actions (`.github/dependabot.yml`); CodeQL `c-cpp` and `actions` (`codeql.yml`, not required). Dependabot and OSV can't read vcpkg purls; `osv-vcpkg.yml` (weekly) queries OSV by each port's upstream commit (`tools/osv-vcpkg.py`). Bump the vcpkg baseline monthly, or sooner when it reports a port. |
| Website | roboviolence.com is built and deployed from [Hub-Tansky/rv2-website](https://github.com/Hub-Tansky/rv2-website) ([ADR 0036](docs/decisions/0036-website-in-rv2-website-repository.md)); its tests read the release file names from `build.yml`. |
| Hosting | Server images from `packaging/docker/Dockerfile` (GHCR on `main`; `:<sha>` only on `release/**`, ADR 0061), runbook [docs/ops/runbook.md](docs/ops/runbook.md). |
| Known defects | Backlog items with `kind: bug`: `40-backlog/` in [Hub-Tansky/rv2-docs](https://github.com/Hub-Tansky/rv2-docs) ([ADR 0044](docs/decisions/0044-backlog-in-the-rv2-docs-vault.md)). |
| Decisions | [docs/decisions/README.md](docs/decisions/README.md) is the ADR index. |

### Open items

- **Master server:** `MasterClient` is reconstructed (the original class is missing from the source release); its timeout is an assumption. See [masterserver/README.md](masterserver/README.md).
- **GLU:** still the system library (headers and link) for `dko`, `zeven_client` and the game, so the headless server needs GLU dev packages; `dkt` is stubbed there. The renderer itself is unchanged fixed-function GL 2.1 (ADR 0003).
- **OGG music** needs `stb_vorbis.c` (vcpkg `stb`, found in CI builds); the placeholders have no music, and playback was never heard.
- **Frame pacing:** the main loop never waits on its own; vsync (`cl_enableVSync`, default on) paces a visible focused window, `main.cpp` sleeps when the window is unfocused or hidden.
- **HiDPI:** `dkwGetResolution()` is the pixel size; the UI math assumes nothing else. Untested on a Retina display.
- **Dead code:** 16 old-menu `.cpp` files in `game/src` are not built (see [game/README.md](game/README.md)).
- **Warnings:** every target except `glad` builds with `-Werror` / `/WX`. Cosmetic classes (unused names, signed compares, initialiser order) are off for `game` and `masterserver` (`bv2_target_legacy_strict`); about 4800 string-literal-to-`char*` warnings are suppressed.
- **clang-tidy** runs in CI as a non-required job.

## Modules

One row per module directory. `tools/check-architecture.sh` fails when a directory under `engine/`, `packaging/` or `content/`, or `game/src`, `masterserver/src`, `tests` or `tools` holds tracked files and has no row, or when a row names a directory without tracked files. Files get no rows: a file's purpose is in its header comment, its module README or the ADR that added it. The per-file table this replaced (ADR 0063) is in `git show 276d92d:ARCHITECTURE.md`.

| directory | purpose |
|---|---|
| `engine/babonet/` | Networking library: `bb_*` API, `CThread` on `std::thread`; see [README](engine/babonet/README.md) |
| `engine/credentials/` | Password HMAC proofs and signed master ban packets (libsodium, ADR 0014, 0038); see [README](engine/credentials/README.md) |
| `engine/dko/` | `.DKO` model loader; see [README](engine/dko/README.md) |
| `engine/zeven/` | Engine utilities: `zeven_core` (`dkc dksvar`, `CString/CVector/CMatrix`), `zeven_console` (no-op `dkt` for the server), `zeven_client` (`dkw dki dkgl dkt dkf dkp dks`); generated glad in `third_party/glad`; see [README](engine/zeven/README.md) |
| `game/` | Targets `bv2`, `bv2dedicated`, `bv2server_lib`; [README](game/README.md), glossary [CONTEXT.md](game/CONTEXT.md) |
| `game/src/` | Client, server and editor sources; `netPacket.h` is the wire format, `CONSOLE` builds the headless server |
| `masterserver/` | Target `bv2master`; see [README](masterserver/README.md) |
| `masterserver/src/` | Master server sources |
| `tests/` | ctest suite (`tests/CMakeLists.txt`, `ctest -N` lists the tests): unit tests, `harness/` in-process server, `corpus/` packet samples, `autotest/` client scripts, `golden/` screenshot goldens, `e2e/` scenarios |
| `tools/` | Repo checks (`check-*`, guard files), `review.sh`, `merge-risk.py`, generators (placeholders, icons, `.pot`), packaging and release helpers |
| `tools/playtest/` | Playtest pull agent for the owner's Mac (ADR 0017); see [README](tools/playtest/README.md) |
| `tools/staging/` | Staging pull deploy and health watch (ADR 0018); see [README](tools/staging/README.md) |
| `packaging/docker/` | Server and master images (`Dockerfile`), master entrypoint, Caddy for the admin APIs; runbook [docs/ops/runbook.md](docs/ops/runbook.md) |
| `packaging/icons/` | Application icons per OS, generated by `tools/gen-app-icon.py` |
| `packaging/linux/` | Linux desktop entry |
| `packaging/macos/` | macOS bundle `Info.plist` template and `.dmg` layout (ADR 0030) |
| `packaging/scripts/` | Start scripts shipped in the packages |
| `packaging/windows/` | Inno Setup installer, Windows manifest and resource template |
| `content/LaunchScript/` | Dedicated-server launch scripts per mode (ADR 0053) |
| `content/bots/` | Bot names (`main/bots/names.txt` at runtime) |
| `content/languages/` | English strings and translation template; see [README](content/languages/README.md) |
| `content/maps/` | The owner's maps (not BV2's) |
| `config/` | Example configs with empty secrets; see [README](config/README.md) |
| `content-seed/` | SQL schemas and defaults for generated databases |
| `docs/` | Build guides (`build/`), ADRs (`decisions/`), runbook (`ops/`), agent skill config (`agents/`) |
| `.github/` | CI workflows, the vcpkg setup action, Dependabot, issue and PR templates |
| `.githooks/` | Commit and push hooks (ADR 0045) |
| `.claude/` | Agent skills shipped with the repo (`rv2-code-review`) |
