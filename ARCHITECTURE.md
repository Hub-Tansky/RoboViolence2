# Architecture

Robo Violence 2: unofficial fork of BaboViolent 2, a top-down multiplayer shooter (C++, GPLv3). State: Phase A done (portable CMake + vcpkg build, SDL3 + miniaudio + glad platform layer); Phase B in progress ([docs/roadmap/](docs/roadmap/README.md)).

## Summary

| Topic | Fact |
|---|---|
| Deliverables | `bv2` (client), `bv2dedicated` (headless server, `CONSOLE`) and `bv2master`. CI builds and tests all three on Windows, macOS and Linux; the client runs 10 s under xvfb (ASan). Real games on each OS: not yet verified (Phase B step 2). |
| Build | CMake 3.25+, Ninja presets (`CMakePresets.json`), vcpkg manifest. Output in `build/<preset>/runtime/` ([ADR 0005](docs/decisions/0005-runtime-main-data-root.md)). |
| Platforms | Targets: Linux x64, macOS 12+ arm64, Windows 11. `engine/zeven/include/platform.h` defines `BV2_PLATFORM_*`, `BV2_POSIX`; CMake force-includes it. |
| Modules | `game` (client, server, editor); `engine/babonet` networking (`bb_*`, `CThread` on `std::thread`); `engine/zeven` utilities (`zeven_core`: `dkc dksvar` + `CString/CVector/CMatrix`; `zeven_console`: no-op `dkt` for the server; `zeven_client`: `dkw dki dkgl dkt dkf dkp dks`); `engine/dko` model loader; `masterserver`. |
| Dependencies | vcpkg (`vcpkg.json`, pinned baseline): sqlite3; feature `client`: sdl3, miniaudio, stb; feature `http`: curl (off, ADR 0002). Generated, committed: glad GL 2.1 (`engine/zeven/third_party/glad`). System: GLU. No libcurl or OpenSSL is linked by default. |
| Platform layer | SDL3 window and input (`dkw`, `dki`), miniaudio (`dks`), glad ([ADR 0006](docs/decisions/0006-sdl3-miniaudio-glad-platform-layer.md)). One client `main()` for all OSes. |
| Tick | Fixed 30 Hz: every `update(float delay)` gets `1/30`; "frames" are a time unit (30 = 1 s). |
| Network | TCP, raw structs `memcpy`'d from `game/src/netPacket.h`. The server is authoritative for hits, damage, spawns, projectiles and flags; clients for their own movement. `playerID` (slot) differs from `babonetID` (connection). Protocol `GAME_VERSION_SV/CL` = 22000 (`game/src/Server.h:32`). |
| Variants | `CONSOLE` = headless server (explicit file list in `game/CMakeLists.txt`). Direct3D, non-Pro and VLD code were removed in Step 1. |
| Config and secrets | Tracked: `config/*.example.cfg` with empty secrets, `content-seed/*.sql` for generated DBs. Real values stay local; gitleaks, hooks and CI enforce it ([config/README.md](config/README.md)). |
| Assets | The original assets are removed and blocked by hash (`tools/check-original-assets.py`). Only `content/languages/en.lang` and `content/LaunchScript/` remain; the build generates placeholders (`tools/gen-placeholder-content.py`). |
| Encoding | UTF-8 without BOM, LF; `tools/check-encoding.py`. Some comments hold U+FFFD where upstream lost accents. |
| Supply chain | Dependency graph fed by vcpkg (`dependency-graph.yml`, on push to `main`); Dependabot alerts and security updates on; Dependabot version updates for GitHub Actions (`.github/dependabot.yml`); CodeQL `c-cpp` and `actions` (`codeql.yml`, not required). Dependabot and OSV can't check vcpkg ports (no C/C++ advisory ecosystem; OSV filters `pkg:vcpkg` purls), so bump the vcpkg baseline monthly, or sooner for a published CVE. |
| Known defects | [docs/analysis/KEY_QUESTIONS.md](docs/analysis/KEY_QUESTIONS.md). |
| Decisions | [docs/decisions/README.md](docs/decisions/README.md): 0001 Ninja, 0002 libcurl compiled out, 0003 OpenGL 2.1 kept, 0004 project name (superseded by 0008), 0005 `main/` data root, 0006 platform layer, 0007 data root, pref dir and config layers, 0008 display name "Robo Violence 2", 0009 internal renaming, 0010 BV2 asset compatibility and GUI freeze, 0011 gettext PO translations. |

### Open items

- **Master server:** `MasterClient` is reconstructed (the original class is missing from the source release); its timeout is an assumption. See [masterserver/README.md](masterserver/README.md).
- **GLU:** still the system library (headers and link) for `dko`, `zeven_client` and the game, so the headless server needs GLU dev packages; `dkt` is stubbed there. The renderer itself is unchanged fixed-function GL 2.1 (ADR 0003).
- **OGG music** needs `stb_vorbis.c` (vcpkg `stb`, found in CI builds); the placeholders have no music, and playback was never heard.
- **HiDPI:** `dkwGetResolution()` is the pixel size; the UI math assumes nothing else. Untested on a Retina display.
- **Dead code:** 16 old-menu `.cpp` files in `game/src` are not built (see [game/README.md](game/README.md)).
- UBSan reports pre-existing defects (out-of-range `bool` loads, NaN casts in `CUserLogin.cpp`).
- **Warnings:** every target except `glad` builds with `-Werror` / `/WX`. Cosmetic classes (unused names, signed compares, initialiser order) are off for `game` and `masterserver` (`bv2_target_legacy_strict`); about 4800 string-literal-to-`char*` warnings are suppressed.
- **clang-tidy** runs in CI as a non-required job.


## File inventory

One row per tracked file. `tools/check-architecture.sh` fails when this list and `git ls-files` differ. Assets: per-file catalogue is the asset inventory (kept outside this repository); replacement asset folders get one row per directory.


### `.`

| path | purpose |
|---|---|
| `.editorconfig` | Editor settings: UTF-8, LF |
| `.clang-format` | Formatting style (tabs, Allman); apply with `git clang-format` only |
| `.clang-tidy` | Static analysis checks (bugprone, clang-analyzer, cert-err34-c); non-blocking |
| `.git-blame-ignore-revs` | Commits ignored by git blame (encoding conversion) |
| `.gitattributes` | Line-ending and binary attributes |
| `.gitignore` | Ignore rules (OS, editors, builds, secrets, original assets) |
| `.gitleaks.toml` | gitleaks rules: default plus config secrets, public IPs, master-server rows, hosts |
| `AGENTS.md` | Canonical agent and contributor instructions |
| `ARCHITECTURE.md` | This file: summary and file inventory |
| `CMakeLists.txt` | Root CMake project: options, runtime layout, placeholder content, subdirectories |
| `CMakePresets.json` | Presets: linux-x64, macos-arm64, win-x64-msvc, -asan variants (Ninja) |
| `LICENSE.txt` | GPLv3 text (code only) |
| `README.md` | Project overview, fork and asset-removal statement |
| `REVIEW.md` | Code review standard: checklist, when to run the thermo-nuclear review, how to act on findings |
| `docs/decisions/0007-data-root-pref-dir-config-layers.md` | ADR 0007 |
| `game/src/Paths.cpp` | Data root search, per-user pref dir, layered config loading, map and DB path helpers |
| `game/src/Paths.h` | Interface of `Paths.cpp` (namespace `bv2`) |
| `packaging/linux/roboviolence2.desktop` | Linux desktop entry |
| `packaging/macos/Info.plist.in` | macOS bundle `Info.plist` template |
| `packaging/windows/bv2.manifest` | Windows manifest: PerMonitorV2 DPI, UTF-8 code page, Windows 10/11 |
| `packaging/windows/bv2.rc.in` | Windows resource script template; CMake fills in the generated icon |
| `tests/CMakeLists.txt` | ctest targets: netPacket, config, fileio, dedicated-server smoke |
| `tests/smoke_server.py` | Starts `bv2dedicated` headless, runs the CTF script, quits |
| `tests/test_config.cpp` | dksvar config layering, transient values not saved, secrets masked |
| `tests/test_fileio.cpp` | `FileIO` byte widths for `.bvm` data and the widths the `.DKO` loader relies on |
| `tests/test_netpacket.cpp` | Byte-level layout of the packed wire structs |
| `tools/check-content-case.py` | Fails when a literal `main/...` path differs from a real file name only by case |
| `vcpkg.json` | vcpkg manifest (sqlite3, curl; Step 3 completes it) |

### `.githooks`

| path | purpose |
|---|---|
| `.githooks/pre-commit` | Pre-commit: identity, gitleaks, original-asset and encoding checks |
| `.githooks/pre-push` | Pre-push: remote and author identity check |

### `.github`

| path | purpose |
|---|---|
| `.github/dependabot.yml` | Dependabot version updates: GitHub Actions, weekly |

### `.github/workflows`

| path | purpose |
|---|---|
| `.github/workflows/secret-scan.yml` | CI: gitleaks over commits after the fork point and original-asset check |
| `.github/workflows/build.yml` | CI: build and ctest on Windows, macOS, Linux; ASan smoke job with client under xvfb; artifacts |
| `.github/workflows/hygiene.yml` | CI: ARCHITECTURE.md inventory, repository hygiene, content case |
| `.github/workflows/codeql.yml` | CodeQL: `c-cpp` (manual linux-x64 build of the three executables) and `actions`; push, PR, weekly; alerts under `build/` (vcpkg headers) filtered out |
| `.github/workflows/dependency-graph.yml` | Submits resolved vcpkg ports to the dependency graph on push to `main` |

### `config`

| path | purpose |
|---|---|
| `config/README.md` | Config and secrets policy |
| `config/bv2.example.cfg` | Default client/server config with empty secrets |

### `content`

| path | purpose |
|---|---|
| `content/README.txt` | Where game data goes (originals removed) |

### `content-seed`

| path | purpose |
|---|---|
| `content-seed/bv2.sql` | Client SQLite schema and defaults (empty MasterServers) |
| `content-seed/master.sql` | Master server SQLite schema (recovered from code) |
| `content-seed/web.sql` | Master server web game-list schema |

### `content/LaunchScript`

| path | purpose |
|---|---|
| `content/LaunchScript/CTF.cfg` | Example dedicated-server launch script (CTF; map is a placeholder) |

### `content/languages`

| path | purpose |
|---|---|
| `content/languages/en.lang` | English strings (default and only language) |

### `docs`

| path | purpose |
|---|---|

### `docs/analysis`

| path | purpose |
|---|---|
| `docs/analysis/01_SYSTEM_OVERVIEW.md` | Code analysis: 01 SYSTEM OVERVIEW |
| `docs/analysis/02_DATA_STRUCTURES.md` | Code analysis: 02 DATA STRUCTURES |
| `docs/analysis/03_DATA_FLOW.md` | Code analysis: 03 DATA FLOW |
| `docs/analysis/ALGORITHM_01-Networking.md` | Code analysis: ALGORITHM 01 Networking |
| `docs/analysis/ALGORITHM_02-Movement-Collision-Interpolation.md` | Code analysis: ALGORITHM 02 Movement Collision Interpolation |
| `docs/analysis/ALGORITHM_03-Weapons-Hitscan-Projectiles.md` | Code analysis: ALGORITHM 03 Weapons Hitscan Projectiles |
| `docs/analysis/ALGORITHM_04-GameModes-Spawning-Rounds.md` | Code analysis: ALGORITHM 04 GameModes Spawning Rounds |
| `docs/analysis/FORK_jmainguy-modern.md` | Code analysis: FORK jmainguy modern |
| `docs/analysis/KEY_QUESTIONS.md` | Code analysis: KEY QUESTIONS |
| `docs/analysis/README.md` | Index of the code analysis |

### `docs/decisions`

| path | purpose |
|---|---|
| `docs/decisions/0001-ninja-generator-on-all-presets.md` | ADR 0001 |
| `docs/decisions/0002-compile-out-libcurl.md` | ADR 0002 |
| `docs/decisions/0003-keep-opengl-2.1-then-sdl-gpu.md` | ADR 0003 |
| `docs/decisions/0004-project-name-roboviolence2.md` | ADR 0004 |
| `docs/decisions/0005-runtime-main-data-root.md` | ADR 0005 |
| `docs/decisions/0006-sdl3-miniaudio-glad-platform-layer.md` | ADR 0006 |
| `docs/decisions/0008-display-name-robo-violence-2.md` | ADR 0008 |
| `docs/decisions/0009-rename-internal-identifiers-continuously.md` | ADR 0009 |
| `docs/decisions/0010-bv2-asset-compat-and-gui-freeze.md` | ADR 0010 |
| `docs/decisions/0011-gettext-po-translations.md` | ADR 0011 |
| `docs/decisions/README.md` | ADR format and index |

### `docs/roadmap`

| path | purpose |
|---|---|
| `docs/roadmap/README.md` | Roadmap index: phases, step status and closing rules |
| `docs/roadmap/_template-step.md` | Template for step files |
| `docs/roadmap/possible-new-scope.md` | Findings from closed steps that may change later phases, pending owner decision |

### `docs/roadmap/phase-a-modern-portable-build`

| path | purpose |
|---|---|
| `docs/roadmap/phase-a-modern-portable-build/README.md` | Phase A handoff and step index |
| `docs/roadmap/phase-a-modern-portable-build/phase-a-manual-test.md` | Manual gameplay and master-listing checklist per OS |
| `docs/roadmap/phase-a-modern-portable-build/step0-fork-rename-and-asset-removal.md` | Scope file for step0 |
| `docs/roadmap/phase-a-modern-portable-build/step1-baseline-and-legacy-removal.md` | Scope file for step1 |
| `docs/roadmap/phase-a-modern-portable-build/step2-cmake-build.md` | Scope file for step2 |
| `docs/roadmap/phase-a-modern-portable-build/step3-dependency-upgrades.md` | Scope file for step3 |
| `docs/roadmap/phase-a-modern-portable-build/step4-64bit-and-cross-platform.md` | Scope file for step4 |
| `docs/roadmap/phase-a-modern-portable-build/step5-code-hygiene-and-ci.md` | Scope file for step5 |

### `docs/roadmap/phase-b-security-infrastructure-anti-cheat`

| path | purpose |
|---|---|
| `docs/roadmap/phase-b-security-infrastructure-anti-cheat/README.md` | Phase B index: goal, steps, done-when, risks |
| `docs/roadmap/phase-b-security-infrastructure-anti-cheat/step0-supply-chain-security.md` | Scope file for step0 |
| `docs/roadmap/phase-b-security-infrastructure-anti-cheat/step1-playtest-builds-and-build-guides.md` | Scope file for step1 |
| `docs/roadmap/phase-b-security-infrastructure-anti-cheat/step2-cross-os-playtest.md` | Scope file for step2 |
| `docs/roadmap/phase-b-security-infrastructure-anti-cheat/step3-test-harness.md` | Scope file for step3 |
| `docs/roadmap/phase-b-security-infrastructure-anti-cheat/step4-packet-hygiene.md` | Scope file for step4 |
| `docs/roadmap/phase-b-security-infrastructure-anti-cheat/step5-crash-fixes.md` | Scope file for step5 |
| `docs/roadmap/phase-b-security-infrastructure-anti-cheat/step6-sql-prepared-statements.md` | Scope file for step6 |
| `docs/roadmap/phase-b-security-infrastructure-anti-cheat/step7-medium-low-defects.md` | Scope file for step7 |
| `docs/roadmap/phase-b-security-infrastructure-anti-cheat/step8-server-validator.md` | Scope file for step8 |
| `docs/roadmap/phase-b-security-infrastructure-anti-cheat/step9-anti-cheat-checks.md` | Scope file for step9 |
| `docs/roadmap/phase-b-security-infrastructure-anti-cheat/step10-server-credentials.md` | Scope file for step10 |
| `docs/roadmap/phase-b-security-infrastructure-anti-cheat/step11-server-containers.md` | Scope file for step11 |
| `docs/roadmap/phase-b-security-infrastructure-anti-cheat/step12-remote-admin.md` | Scope file for step12 |
| `docs/roadmap/phase-b-security-infrastructure-anti-cheat/step13-localisation.md` | Scope file for step13 |

### `engine/babonet`

| path | purpose |
|---|---|
| `engine/babonet/CMakeLists.txt` | Target babonet |
| `engine/babonet/README.md` | Module README: babonet |

### `engine/babonet/include`

| path | purpose |
|---|---|
| `engine/babonet/include/CThread.h` | Thread wrapper |
| `engine/babonet/include/baboNet.h` | babonet public API (bb_* functions) |
| `engine/babonet/include/cMSstruct.h` | Master-server protocol structs and message IDs |
| `engine/babonet/include/md5class.h` | CMD5: MD5 hex-digest wrapper |

### `engine/babonet/src`

| path | purpose |
|---|---|
| `engine/babonet/src/CThread.cpp` | Thread wrapper |
| `engine/babonet/src/MD5.h` | RFC 1321 MD5 header |
| `engine/babonet/src/baboNet.cpp` | babonet public API (bb_* functions) |
| `engine/babonet/src/cClient.cpp` | babonet client connection (TCP and UDP) |
| `engine/babonet/src/cClient.h` | babonet client connection (TCP and UDP) |
| `engine/babonet/src/cConnection.cpp` | babonet outgoing connection state machine |
| `engine/babonet/src/cConnection.h` | babonet outgoing connection state machine |
| `engine/babonet/src/cDNSquery.cpp` | babonet asynchronous DNS lookup |
| `engine/babonet/src/cDNSquery.h` | babonet asynchronous DNS lookup |
| `engine/babonet/src/cIncConnection.cpp` | babonet incoming connection handshake |
| `engine/babonet/src/cIncConnection.h` | babonet incoming connection handshake |
| `engine/babonet/src/cPacket.cpp` | babonet TCP packet |
| `engine/babonet/src/cPacket.h` | babonet TCP packet |
| `engine/babonet/src/cPeer.cpp` | babonet UDP peer with reliability acks |
| `engine/babonet/src/cPeer.h` | babonet UDP peer with reliability acks |
| `engine/babonet/src/cPeer2Peer.cpp` | babonet UDP peer-to-peer manager |
| `engine/babonet/src/cPeer2Peer.h` | babonet UDP peer-to-peer manager |
| `engine/babonet/src/cServer.cpp` | babonet server: listener and client set |
| `engine/babonet/src/cServer.h` | babonet server: listener and client set |
| `engine/babonet/src/cUDPpacket.cpp` | babonet UDP packet |
| `engine/babonet/src/cUDPpacket.h` | babonet UDP packet |
| `engine/babonet/src/cUDPserver.cpp` | babonet UDP server |
| `engine/babonet/src/cUDPserver.h` | babonet UDP server |
| `engine/babonet/src/global.h` | babonet shared types and constants |
| `engine/babonet/src/main.cpp` | Program entry: window, engine init, main loop |
| `engine/babonet/src/md5c.c` | RFC 1321 MD5 implementation |
| `engine/babonet/src/md5class.cpp` | CMD5: MD5 hex-digest wrapper |
| `engine/babonet/src/socket_compat.h` | send() with EINTR retry; SIGPIPE is ignored in bb_init |

### `engine/dko`

| path | purpose |
|---|---|
| `engine/dko/CMakeLists.txt` | Target dko |
| `engine/dko/README.md` | Module README: dko |

### `engine/dko/include`

| path | purpose |
|---|---|
| `engine/dko/include/dko.h` | dko: public API of the .DKO model loader |

### `engine/dko/src`

| path | purpose |
|---|---|
| `engine/dko/src/CFace.cpp` | dko: geometry and collision octree |
| `engine/dko/src/CFace.h` | dko: geometry and collision octree |
| `engine/dko/src/CHUNKINF.H` | Autodesk 3DS chunk definitions (3DS import) |
| `engine/dko/src/COctree.cpp` | dko: geometry and collision octree |
| `engine/dko/src/COctree.h` | dko: geometry and collision octree |
| `engine/dko/src/CdkoAnimation.cpp` | dko: model data structures |
| `engine/dko/src/CdkoAnimation.h` | dko: model data structures |
| `engine/dko/src/CdkoMaterial.cpp` | dko: model data structures |
| `engine/dko/src/CdkoMaterial.h` | dko: model data structures |
| `engine/dko/src/CdkoMesh.cpp` | dko: model data structures |
| `engine/dko/src/CdkoMesh.h` | dko: model data structures |
| `engine/dko/src/CdkoModel.cpp` | dko: model data structures |
| `engine/dko/src/CdkoModel.h` | dko: model data structures |
| `engine/dko/src/Readme.txt` | Notes on the DKO exporter and format |
| `engine/dko/src/dko-chunk-info.txt` | DKO file chunk layout |
| `engine/dko/src/dko.cpp` | dko: public API of the .DKO model loader |
| `engine/dko/src/dkoInner.h` | dko: internal declarations |
| `engine/dko/src/eHierarchic.cpp` | dko: hierarchy and texture helpers |
| `engine/dko/src/eHierarchic.h` | dko: hierarchy and texture helpers |
| `engine/dko/src/ePTexture.cpp` | dko: hierarchy and texture helpers |
| `engine/dko/src/ePTexture.h` | dko: hierarchy and texture helpers |

### `engine/zeven`

| path | purpose |
|---|---|
| `engine/zeven/CMakeLists.txt` | Targets zeven_core and zeven_client |
| `engine/zeven/README.md` | Module README: zeven |

### `engine/zeven/include`

| path | purpose |
|---|---|
| `engine/zeven/include/CDkoObject.h` | DKO model instance wrapper |
| `engine/zeven/include/CFont.h` | Bitmap font class |
| `engine/zeven/include/CMatrix.h` | Engine copy of the shared math/string type |
| `engine/zeven/include/CParticle.h` | Particle class |
| `engine/zeven/include/CString.h` | Engine copy of the shared math/string type |
| `engine/zeven/include/CSystemVariable.h` | Typed configuration variable classes |
| `engine/zeven/include/CVector.h` | Engine copy of the shared math/string type |
| `engine/zeven/include/dikeys.h` | DIK_* key IDs (DirectInput scancodes) used by dki, key binds and the game |
| `engine/zeven/include/dkc.h` | dkc: high-resolution timer and platform helpers |
| `engine/zeven/include/dkf.h` | dkf: bitmap font rendering |
| `engine/zeven/include/dkgl.h` | dkgl: OpenGL context and state |
| `engine/zeven/include/dki.h` | dki: keyboard, mouse and input polling |
| `engine/zeven/include/dkp.h` | dkp: particle system |
| `engine/zeven/include/dks.h` | dks: sound and music (FMOD) |
| `engine/zeven/include/dksvar.h` | dksvar: configuration variable registry |
| `engine/zeven/include/dksvardef.h` | dksvar: variable definition macros |
| `engine/zeven/include/dkt.h` | dkt: texture loading (TGA) and binding |
| `engine/zeven/include/dkw.h` | dkw: window creation and events |
| `engine/zeven/include/glheaders.h` | The only OpenGL include: glad, then GLU |
| `engine/zeven/include/platform.h` | Platform macros (BV2_PLATFORM_*, BV2_POSIX), INT4/UINT4, POSIX includes and Win32 type shims; force-included |

### `engine/zeven/src`

| path | purpose |
|---|---|
| `engine/zeven/src/CDkoObject.cpp` | DKO model instance wrapper |
| `engine/zeven/src/CFont.cpp` | Bitmap font class |
| `engine/zeven/src/CMatrix.cpp` | Engine copy of the shared math/string type |
| `engine/zeven/src/CParticle.cpp` | Particle class |
| `engine/zeven/src/CString.cpp` | Engine copy of the shared math/string type |
| `engine/zeven/src/CSystemVariable.cpp` | Typed configuration variable classes |
| `engine/zeven/src/CVector.cpp` | Engine copy of the shared math/string type |
| `engine/zeven/src/dkc.cpp` | dkc: high-resolution timer and platform helpers |
| `engine/zeven/src/dkf.cpp` | dkf: bitmap font rendering |
| `engine/zeven/src/dkfi.h` | Internal header of the matching dk module |
| `engine/zeven/src/dkgl.cpp` | dkgl: OpenGL context and state |
| `engine/zeven/src/dkgli.h` | Internal header of the matching dk module |
| `engine/zeven/src/dki.cpp` | dki: keyboard, mouse and input polling |
| `engine/zeven/src/dkp.cpp` | dkp: particle system |
| `engine/zeven/src/dkpi.h` | Internal header of the matching dk module |
| `engine/zeven/src/dks.cpp` | dks: sound and music (FMOD) |
| `engine/zeven/src/dksvar.cpp` | dksvar: configuration variable registry |
| `engine/zeven/src/dksvari.h` | Internal header of the matching dk module |
| `engine/zeven/src/dkt.cpp` | dkt: texture loading (TGA) and binding |
| `engine/zeven/src/dkt_console.cpp` | No-op dkt for the headless server |
| `engine/zeven/src/dkti.h` | Internal header of the matching dk module |
| `engine/zeven/src/dkw.cpp` | dkw: window creation and events |

### `engine/zeven/third_party/glad`

| path | purpose |
|---|---|
| `engine/zeven/third_party/glad/README.md` | How the glad loader was generated |

### `engine/zeven/third_party/glad/include/KHR`

| path | purpose |
|---|---|
| `engine/zeven/third_party/glad/include/KHR/khrplatform.h` | Khronos platform header (glad dependency) |

### `engine/zeven/third_party/glad/include/glad`

| path | purpose |
|---|---|
| `engine/zeven/third_party/glad/include/glad/gl.h` | Generated glad header |

### `engine/zeven/third_party/glad/src`

| path | purpose |
|---|---|
| `engine/zeven/third_party/glad/src/gl.c` | Generated glad loader (GL 2.1 compatibility, GL_EXT_bgra) |

### `game`

| path | purpose |
|---|---|
| `game/CMakeLists.txt` | Targets bv2dedicated (explicit CONSOLE file list) and bv2 (client) |
| `game/README.md` | Module README: game |

### `game/src`

| path | purpose |
|---|---|
| `game/src/AccountManager.cpp` | Account create/login/clan/friend requests over the master connection |
| `game/src/AccountManager.h` | Account create/login/clan/friend requests over the master connection |
| `game/src/Button.cpp` | Menu button widget |
| `game/src/Button.h` | Menu button widget |
| `game/src/CAStar.cpp` | A* path finding grid |
| `game/src/CAStar.h` | A* path finding grid |
| `game/src/CAStar_FindPath.cpp` | A* search routine |
| `game/src/CAccount.cpp` | Account UI tab |
| `game/src/CAccount.h` | Account UI tab |
| `game/src/CBrowser.cpp` | Server browser UI |
| `game/src/CBrowser.h` | Server browser UI |
| `game/src/CClans.cpp` | Clans UI tab |
| `game/src/CClans.h` | Clans UI tab |
| `game/src/CControl.cpp` | Generic UI control (button, label, list, text box) of the newer menu |
| `game/src/CControl.h` | Generic UI control (button, label, list, text box) of the newer menu |
| `game/src/CCredit.cpp` | Credits tab |
| `game/src/CCredit.h` | Credits tab |
| `game/src/CCurl.cpp` | HTTP requests through libcurl (compiled out in Phase A) |
| `game/src/CCurl.h` | HTTP requests through libcurl (compiled out in Phase A) |
| `game/src/CCurlStub.cpp` | CCurl stand-in when BV2_WITH_HTTP is OFF |
| `game/src/CEditor.cpp` | Map editor UI (newer) |
| `game/src/CEditor.h` | Map editor UI (newer) |
| `game/src/CFriends.cpp` | Friends list UI and requests |
| `game/src/CFriends.h` | Friends list UI and requests |
| `game/src/CHost.cpp` | Host-game UI |
| `game/src/CHost.h` | Host-game UI |
| `game/src/CLava.cpp` | Lava rendering and effect |
| `game/src/CLava.h` | Lava rendering and effect |
| `game/src/CListener.cpp` | Listener interface for CControl events |
| `game/src/CListener.h` | Listener interface for CControl events |
| `game/src/CLobby.cpp` | Lobby and menu tab host |
| `game/src/CLobby.h` | Lobby and menu tab host |
| `game/src/CMainTab.cpp` | Main menu tab |
| `game/src/CMainTab.h` | Main menu tab |
| `game/src/CMaster.cpp` | Master-server client: server list, remote admin, account manager packets |
| `game/src/CMaster.h` | Master-server client: server list, remote admin, account manager packets |
| `game/src/CMaterial.cpp` | Material/texture state wrapper |
| `game/src/CMaterial.h` | Material/texture state wrapper |
| `game/src/CMenuManager.cpp` | Manager for the newer menu controls |
| `game/src/CMenuManager.h` | Manager for the newer menu controls |
| `game/src/CMesh.cpp` | Mesh container |
| `game/src/CMesh.h` | Mesh container |
| `game/src/CMeshBuilder.cpp` | Mesh builder |
| `game/src/CMeshBuilder.h` | Mesh builder |
| `game/src/CNews.cpp` | News feed UI |
| `game/src/CNews.h` | News feed UI |
| `game/src/COption.cpp` | Options UI |
| `game/src/COption.h` | Options UI |
| `game/src/CPanel.h` | UI panel container |
| `game/src/CPathNode.cpp` | Path node for A* |
| `game/src/CPathNode.h` | Path node for A* |
| `game/src/CPing.cpp` | Server ping measurement |
| `game/src/CPing.h` | Server ping measurement |
| `game/src/CPlayers.cpp` | Players list UI |
| `game/src/CPlayers.h` | Players list UI |
| `game/src/CProfile.cpp` | Profile UI tab |
| `game/src/CProfile.h` | Profile UI tab |
| `game/src/CRain.cpp` | Rain weather effect |
| `game/src/CRain.h` | Rain weather effect |
| `game/src/CRegisterClan.cpp` | Clan registration UI |
| `game/src/CRegisterClan.h` | Clan registration UI |
| `game/src/CSnow.cpp` | Snow weather effect |
| `game/src/CSnow.h` | Snow weather effect |
| `game/src/CStats.cpp` | Player statistics UI |
| `game/src/CStats.h` | Player statistics UI |
| `game/src/CStatus.cpp` | Online status reporting |
| `game/src/CStatus.h` | Online status reporting |
| `game/src/CSurvey.cpp` | Survey dialog |
| `game/src/CSurvey.h` | Survey dialog |
| `game/src/CUrlData.cpp` | URL-encoded request bodies (MD5/base64 fields), shared by both CCurl builds |
| `game/src/CUserLogin.cpp` | Account login UI |
| `game/src/CUserLogin.h` | Account login UI |
| `game/src/CVertexBuffer.cpp` | Vertex buffer wrapper |
| `game/src/CVertexBuffer.h` | Vertex buffer wrapper |
| `game/src/CWeather.cpp` | Weather selection and rendering |
| `game/src/CWeather.h` | Weather selection and rendering |
| `game/src/Choice.cpp` | Menu choice (drop-down/spinner) widget |
| `game/src/Choice.h` | Menu choice (drop-down/spinner) widget |
| `game/src/Client.cpp` | Client: connection, join handshake, send/receive state |
| `game/src/Client.h` | Client: connection, join handshake, send/receive state |
| `game/src/ClientRecv.cpp` | Client: handlers for every server-to-client packet |
| `game/src/ClientRender.cpp` | Client: in-game HUD and view rendering |
| `game/src/ConfirmPass.cpp` | Password confirmation dialog |
| `game/src/ConfirmPass.h` | Password confirmation dialog |
| `game/src/ConnectFailed.h` | Dialog: connection failed |
| `game/src/Console.cpp` | In-game console: commands, log, remote console |
| `game/src/Console.h` | In-game console: commands, log, remote console |
| `game/src/Control.cpp` | Menu control base class |
| `game/src/Control.h` | Menu control base class |
| `game/src/ControlListener.cpp` | Menu control listener interface |
| `game/src/ControlListener.h` | Menu control listener interface |
| `game/src/CreateGame.h` | Create-game screen |
| `game/src/Credits.h` | Credits screen |
| `game/src/Dialog.cpp` | Menu dialog base |
| `game/src/Dialog.h` | Menu dialog base |
| `game/src/Editor.cpp` | Map editor core |
| `game/src/Editor.h` | Map editor core |
| `game/src/EditorDialogs.cpp` | Map editor dialogs |
| `game/src/EditorDialogs.h` | Map editor dialogs |
| `game/src/EditorTools.cpp` | Map editor tools |
| `game/src/EditorTools.h` | Map editor tools |
| `game/src/Extended.h` | Skin and color chooser screen |
| `game/src/FastDelegate.h` | Third-party fast C++ delegate header |
| `game/src/FileIO.cpp` | File and memory-buffer reader/writer with fixed-width types |
| `game/src/FileIO.h` | File and memory-buffer reader/writer with fixed-width types |
| `game/src/Game.cpp` | Game state: players, projectiles, items, flags, round and mode logic (shared by client and server) |
| `game/src/Game.h` | Game state: players, projectiles, items, flags, round and mode logic (shared by client and server) |
| `game/src/GameProjectile.cpp` | Game: projectile and explosion simulation |
| `game/src/GameRender.cpp` | Game: world rendering |
| `game/src/GameShowStats.cpp` | Game: scoreboard and end-of-round stats drawing |
| `game/src/GameSpawn.cpp` | Game: spawn point selection and respawn |
| `game/src/GameVar.cpp` | All client and server variables (cl_*, sv_*, r_*, s_*), registered through dksvar, plus loaded textures and sounds |
| `game/src/GameVar.h` | All client and server variables (cl_*, sv_*, r_*, s_*), registered through dksvar, plus loaded textures and sounds |
| `game/src/Helper.cpp` | Misc helpers (colors, text, math) |
| `game/src/Helper.h` | Misc helpers (colors, text, math) |
| `game/src/Host.h` | Host-game settings struct |
| `game/src/IncorrectName.h` | Dialog: invalid player name |
| `game/src/IncorrectPassword.h` | Dialog: wrong server password |
| `game/src/IntroScreen.cpp` | Intro screen |
| `game/src/IntroScreen.h` | Intro screen |
| `game/src/JoinGame.h` | Join-game screen |
| `game/src/Key.cpp` | Key binding entry |
| `game/src/Key.h` | Key binding entry |
| `game/src/KeyManager.cpp` | Key bindings manager |
| `game/src/KeyManager.h` | Key bindings manager |
| `game/src/Label.cpp` | Menu label widget |
| `game/src/Label.h` | Menu label widget |
| `game/src/MainMenu.h` | Main menu screen |
| `game/src/Map.cpp` | Map data, .bvm loading and saving, collision, rendering resources |
| `game/src/Map.h` | Map data, .bvm loading and saving, collision, rendering resources |
| `game/src/MapRender.cpp` | Map rendering (tiles, walls, dirt, optional 3D model map) |
| `game/src/MemIO.cpp` | In-memory buffer reader/writer |
| `game/src/MemIO.h` | In-memory buffer reader/writer |
| `game/src/Menu.h` | Menu system core (old menu) |
| `game/src/MenuSetup.cpp` | Menu construction |
| `game/src/MessageDialog.cpp` | Generic message dialog |
| `game/src/MessageDialog.h` | Generic message dialog |
| `game/src/Minibot.cpp` | Minibot drone weapon entity |
| `game/src/NoGameRunning.h` | Dialog: no game running |
| `game/src/NoMapSelected.h` | Dialog: no map selected |
| `game/src/OptionMenu.h` | Options screen |
| `game/src/Password.h` | Password prompt dialog |
| `game/src/Player.cpp` | Player entity: movement, weapons, hits, rendering |
| `game/src/Player.h` | Player entity: movement, weapons, hits, rendering |
| `game/src/PlayerUpdate.cpp` | Player: per-tick update (physics, firing, timers) |
| `game/src/Quit.h` | Quit confirmation dialog |
| `game/src/RemoteAdminPackets.h` | Wire structs for the remote-admin protocol |
| `game/src/ReportGen.cpp` | XML server report generation |
| `game/src/ReportGen.h` | XML server report generation |
| `game/src/Scene.cpp` | Top-level scene: main loop, menus, game and editor switching, rendering setup |
| `game/src/Scene.h` | Top-level scene: main loop, menus, game and editor switching, rendering setup |
| `game/src/SceneNet.cpp` | Scene: starts and stops client and server, hosting and joining |
| `game/src/Server.cpp` | Authoritative server: connections, joins, tick, map and mode control |
| `game/src/Server.h` | Authoritative server: connections, joins, tick, map and mode control |
| `game/src/ServerCTF.cpp` | Server: capture-the-flag rules |
| `game/src/ServerClose.h` | Dialog shown when the server closes the connection |
| `game/src/ServerRecv.cpp` | Server: handlers for every client-to-server packet |
| `game/src/ServerSnD.cpp` | Server: search-and-destroy rules |
| `game/src/Weapon.cpp` | Weapon definitions and stats |
| `game/src/Weapon.h` | Weapon definitions and stats |
| `game/src/Write.cpp` | Chat input widget |
| `game/src/Write.h` | Chat input widget |
| `game/src/Writting.cpp` | Chat input handling |
| `game/src/Writting.h` | Chat input handling |
| `game/src/WrongVersion.h` | Dialog: client/server version mismatch |
| `game/src/Zeven.h` | Umbrella include for engine headers and common types |
| `game/src/changes-log.txt` | Upstream change log |
| `game/src/main.cpp` | Program entry: window, engine init, main loop |
| `game/src/netPacket.h` | Wire structs and message IDs (raw memcpy'd structs) |
| `game/src/screengrab.cpp` | Screenshot capture |
| `game/src/screengrab.h` | Screenshot capture |
| `game/src/tinyxml.cpp` | TinyXML (third-party XML parser) |
| `game/src/tinyxml.h` | TinyXML (third-party XML parser) |
| `game/src/tinyxmlerror.cpp` | TinyXML (third-party XML parser) |
| `game/src/tinyxmlparser.cpp` | TinyXML (third-party XML parser) |

### `masterserver`

| path | purpose |
|---|---|
| `masterserver/CMakeLists.txt` | Target bv2master |
| `masterserver/README.md` | Module README: master server |

### `masterserver/src`

| path | purpose |
|---|---|
| `masterserver/src/MasterClient.cpp` | Master: connected-client record (reconstructed) |
| `masterserver/src/MasterClient.h` | Master: connected-client record (reconstructed; see masterserver/README.md) |
| `masterserver/src/cBV2game.cpp` | Master: game list entry |
| `masterserver/src/cBV2game.h` | Master: game list entry |
| `masterserver/src/cMSstruct.h` | Master protocol structs (older subset of engine/babonet/include/cMSstruct.h) |
| `masterserver/src/cMasterServer.cpp` | Master: server registry, bans, DB access |
| `masterserver/src/cMasterServer.h` | Master: server registry, bans, DB access |
| `masterserver/src/cNetManager.cpp` | Master: network loop and packet handling |
| `masterserver/src/cNetManager.h` | Master: network loop and packet handling |
| `masterserver/src/main.cpp` | Program entry: window, engine init, main loop |

### `tools`

| path | purpose |
|---|---|
| `tools/CMakeLists.txt` | Database seeding (bv2_seed_db_tool, bv2.db, master.db, web.db) |
| `tools/FORK_BASE` | First commit after upstream; secret scans start here |
| `tools/check-architecture.ps1` | Fails if ARCHITECTURE.md and the tracked files disagree (Windows) |
| `tools/check-architecture.sh` | Fails if ARCHITECTURE.md and the tracked files disagree (Unix) |
| `tools/check-encoding.py` | Fails on non-UTF-8, BOM or CR in tracked text files |
| `tools/check-original-assets.py` | Fails if a tracked file matches an original-asset hash |
| `tools/original-assets.sha256` | SHA-256 of every removed original file |
| `tools/convert-encoding.py` | One-off UTF-8/LF converter used in step 1.4 |
| `tools/placeholder-manifest.tsv` | Files the game loads at startup and their formats, for the placeholder generator |
| `tools/gen-placeholder-content.py` | Writes placeholder maps, textures, a readable font atlas, sounds and models for dev and CI |
| `tools/gen-placeholder-icon.py` | Writes the placeholder Windows application icon at build time |
| `tools/check-hygiene.py` | Fails on spaces in paths, tracked ignored files, files over 5 MB, bad encoding |
| `tools/seed_db.cpp` | Creates a SQLite DB from SQL files (build helper) |
| `tools/setup-dev.ps1` | Activates hooks, checks tools (Windows) |
| `tools/setup-dev.sh` | Activates hooks, checks tools (Unix) |
