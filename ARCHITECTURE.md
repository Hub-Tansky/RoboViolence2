# Architecture

Robo Violence 2: unofficial fork of BaboViolent 2, a top-down multiplayer shooter (C++, GPLv3). State: Phases A and B done; Phase C done, first release pending; Phase T steps 1–3 done. Backlog, roadmap and bugs: the [GitHub Project](https://github.com/users/Hub-Tansky/projects/1) (private) and [issues](https://github.com/Hub-Tansky/RoboViolence2/issues).

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
| Assets | The original assets are removed and blocked by hash (`tools/check-original-assets.py`). Only `content/languages/` (English strings, template, guide) and `content/LaunchScript/` remain; the build generates placeholders (`tools/gen-placeholder-content.py`). |
| Encoding | UTF-8 without BOM, LF; `tools/check-encoding.py`. Some comments hold U+FFFD where upstream lost accents. |
| Supply chain | Dependency graph fed by vcpkg (`dependency-graph.yml`, on push to `main`); Dependabot alerts and security updates on; Dependabot version updates for GitHub Actions (`.github/dependabot.yml`); CodeQL `c-cpp` and `actions` (`codeql.yml`, not required). Dependabot and OSV can't read vcpkg purls; `osv-vcpkg.yml` (weekly) queries OSV by each port's upstream commit (`tools/osv-vcpkg.py`). Bump the vcpkg baseline monthly, or sooner when it reports a port. |
| Website | roboviolence.com is built and deployed from [Hub-Tansky/rv2-website](https://github.com/Hub-Tansky/rv2-website) ([ADR 0036](docs/decisions/0036-website-in-rv2-website-repository.md)); its tests read the release file names from `build.yml`. |
| Hosting | Server images from `packaging/docker/Dockerfile` (GHCR on `main`), runbook [docs/ops/runbook.md](docs/ops/runbook.md). |
| Known defects | [docs/analysis/KEY_QUESTIONS.md](https://github.com/Hub-Tansky/RoboViolence2/blob/fc0aff7c5aa946406c7c110e8e7c2d972a7daeb8/docs/analysis/KEY_QUESTIONS.md). |
| Decisions | [docs/decisions/README.md](docs/decisions/README.md): 0001 Ninja, 0002 libcurl compiled out, 0003 OpenGL 2.1 kept, 0004 project name (superseded by 0008), 0005 `main/` data root, 0006 platform layer, 0007 data root, pref dir and config layers, 0008 display name "Robo Violence 2", 0009 internal renaming, 0010 BV2 asset compatibility and GUI freeze, 0011 gettext PO translations, 0012 Windows 10 22H2 minimum, 0013 rulesets, 0014 libsodium HMAC, 0015 admin HTTP API, 0016 player data lookup and package content, 0017 playtest pull agent, 0018 staging pull deploy, 0019 in-game name and fork credits, 0020 package and release format, 0021 one-argument CString is text, 0022 advertised server address, 0023 step review subagent in the repo (superseded by 0037), 0024 GitHub Project is the backlog, 0025 client autotest mode, 0026 screenshot goldens, 0029 static website (moved to rv2-website), 0033 first-run data dialog recommends the BV2 zip (superseded by 0040), 0034 player data retention and privacy policy, 0035 website Tailwind and agent files (moved to rv2-website), 0036 website in rv2-website, 0037 review rubric single repo copy, 0038 dead account pages hidden, admin and ban secrets hardened, 0039 Widescreen (Stretch) shows more map, 0040 first-run download of the original data. |

### Open items

- **Master server:** `MasterClient` is reconstructed (the original class is missing from the source release); its timeout is an assumption. See [masterserver/README.md](masterserver/README.md).
- **GLU:** still the system library (headers and link) for `dko`, `zeven_client` and the game, so the headless server needs GLU dev packages; `dkt` is stubbed there. The renderer itself is unchanged fixed-function GL 2.1 (ADR 0003).
- **OGG music** needs `stb_vorbis.c` (vcpkg `stb`, found in CI builds); the placeholders have no music, and playback was never heard.
- **HiDPI:** `dkwGetResolution()` is the pixel size; the UI math assumes nothing else. Untested on a Retina display.
- **Dead code:** 16 old-menu `.cpp` files in `game/src` are not built (see [game/README.md](game/README.md)).
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
| `.dockerignore` | Docker build context: sources only (no `.git`, builds, docs) |
| `AGENTS.md` | Canonical agent and contributor instructions; work tracking in the GitHub Project |
| `ARCHITECTURE.md` | This file: summary and file inventory |
| `CMakeLists.txt` | Root CMake project: options, runtime layout, placeholder content, subdirectories, install layout (binaries, `share/bv2/main` as `runtime/main` holds it, `.desktop`, icons; `bv2master` in component `master`), which the `.deb` packages |
| `CMakePresets.json` | Presets: linux-x64, macos-arm64, win-x64-msvc (Debug), -asan and -release variants (Ninja); macOS uses the overlay triplet in `tools/vcpkg-triplets/` |
| `LICENSE.txt` | GPLv3 text (code only) |
| `README.md` | Project overview, fork and asset-removal statement |
| `REVIEW.md` | Definition of done: `tools/review.sh` checks, reviewer checklist, defined terms, forbidden shortcuts, fresh-context PR review (`.claude/agents/`) and its record in the PR description |
| `SECURITY.md` | Security policy: supported versions, private vulnerability reporting, response timeline |
| `docs/decisions/0007-data-root-pref-dir-config-layers.md` | ADR 0007 |
| `game/src/Paths.cpp` | Package and data roots (`contentFile` for the package's own content), saved and detected BV2 data folder (`originalDataDir` first), per-user pref dir, layered config loading (release settings like the master list skip `bv2.cfg`, ADR 0028), map and DB path helpers (ADR 0016) |
| `game/src/Paths.h` | Interface of `Paths.cpp` (namespace `bv2`) |
| `packaging/linux/roboviolence2.desktop` | Linux desktop entry |
| `packaging/make-package.py` | Builds the per-OS package from a Release `runtime/` (links resolved, macOS bundle re-signed, licences), plus the Windows installer (showing `LICENSE.txt` reflowed one paragraph per line), the macOS `.dmg` with only the app and Applications, or the Linux `.deb` (ADR 0020, 0030, 0031) |
| `packaging/make_deb.py` | Builds the Linux `.deb` from the build's `cmake --install` (stripped, without the master): client and `bv2dedicated` moved to `/usr/games`, launcher pointed there, licences, copyright and changelog added; Depends from `dpkg-shlibdeps` plus `libgl1`, `curl` and `unzip` (ADR 0031, 0040) |
| `packaging/discord_release_post.py` | Prints the Discord webhook payload of the release job: one embed with the release page, the player downloads and the website, an optional ping of the opt-in role and no other mention (ADR 0032) |
| `packaging/windows/roboviolence2.iss` | Inno Setup script for the Windows installer (per-user, unsigned); shows the reflowed licence from `make-package.py`; privacy policy link and the opt-out task for the public master (ADR 0034) |
| `packaging/scripts/run-client.cmd` | Package script: starts the client (Windows) |
| `packaging/scripts/run-client.sh` | Package script: starts the client (macOS bundle or Linux binary) |
| `packaging/scripts/run-master.cmd` | Package script: starts the master server (Windows) |
| `packaging/scripts/run-master.sh` | Package script: starts the master server (Unix) |
| `packaging/scripts/run-server.cmd` | Package script: starts the dedicated server, default launch script CTF (Windows) |
| `packaging/scripts/run-server.sh` | Package script: starts the dedicated server, default launch script CTF (Unix) |
| `packaging/macos/Info.plist.in` | macOS bundle `Info.plist` template (names the icon) |
| `packaging/macos/dmg-background.png` | `.dmg` window background: arrow from the app to Applications ([ADR 0030](docs/decisions/0030-app-icon-and-dmg-layout.md)) |
| `packaging/macos/dmg-background@2x.png` | The same background for Retina displays |
| `packaging/macos/dmg-layout.json` | `.dmg` window size, icon centres and caption line, read by `tools/gen-app-icon.py` and `packaging/make-package.py` |
| `packaging/macos/requirements.txt` | dmgbuild and its dependencies, pinned with hashes, for the `.dmg` layout (CI venv) |
| `packaging/icons/roboviolence2.icns` | macOS application icon (bundle and `.dmg` volume), from `tools/gen-app-icon.py` |
| `packaging/icons/roboviolence2.ico` | Windows application icon (`bv2.exe` and the installer), 16-256 px |
| `packaging/icons/hicolor/16x16/apps/roboviolence2.png` | Linux hicolor icon, 16 px (installed as is) |
| `packaging/icons/hicolor/24x24/apps/roboviolence2.png` | Linux hicolor icon, 24 px (installed as is) |
| `packaging/icons/hicolor/32x32/apps/roboviolence2.png` | Linux hicolor icon, 32 px (installed as is) |
| `packaging/icons/hicolor/48x48/apps/roboviolence2.png` | Linux hicolor icon, 48 px (installed as is) |
| `packaging/icons/hicolor/64x64/apps/roboviolence2.png` | Linux hicolor icon, 64 px (installed as is) |
| `packaging/icons/hicolor/128x128/apps/roboviolence2.png` | Linux hicolor icon, 128 px (installed as is); compiled into the Linux client as its window icon |
| `packaging/icons/hicolor/256x256/apps/roboviolence2.png` | Linux hicolor icon, 256 px (installed as is) |
| `packaging/icons/hicolor/512x512/apps/roboviolence2.png` | Linux hicolor icon, 512 px (installed as is) |
| `packaging/docker/Dockerfile` | Server images: one Ubuntu 24.04 build, targets `server` (`bv2dedicated`, launch script as argument, healthcheck on `BV2_HEALTH_PORT`) and `master` (`bv2master`), non-root, pref dir volume (step 11) |
| `packaging/docker/master-entrypoint.sh` | Master image entrypoint: seeds `master.db` and `web.db` into the volume, then runs `bv2master` |
| `packaging/docker/Caddyfile` | Caddy site: TLS for the game servers' admin APIs, CTF at `/`, TDM at `/tdm/` (`BV2_ADMIN_HOST`, default localhost) (step 12); `/deployed` serves the staging commit (phase T step 2) |
| `packaging/docker/compose.example.yml` | Compose example: master, CTF (3333) and TDM (3334) game servers and the admin TLS proxy on one host, secrets from an env file outside the repo; `BV2_IMAGE_TAG` pins both images to a commit |
| `packaging/windows/bv2.manifest` | Windows manifest for `bv2`, `bv2dedicated` and `bv2master`: PerMonitorV2 DPI, UTF-8 code page, Windows 10/11 |
| `packaging/windows/bv2.rc.in` | Windows resource script template: embeds `packaging/icons/roboviolence2.ico` |
| `tests/CMakeLists.txt` | ctest targets: netPacket, config, fileio, credentials, join password, admin login, admin API, babonet large message, babonet ping with the LAN port taken, the two `SqlStatement.h` copies identical, crash fixes, validator, missing music (client builds), app icon files and wiring, the built-in window icon (Linux client builds), the hicolor install (Linux), the `.deb` build (Linux), dedicated-server smoke, in-process server harness (`bv2server_lib` + fake babonet), packet hygiene, corpus replay, crash-input tests for `crash-*.bin` under ASan (none at present), master server, autotest runner and (with Xvfb on Linux, or on macOS) the client on autotest scripts; with Xvfb on Linux the e2e scenarios (`e2e_*`) and screenshots (`screenshot_*`); on Windows also the non-ASCII install path smoke and the no-VC++-runtime-DLL check |
| `tests/check_windows_deps.py` | Windows: fails if `bv2`, `bv2dedicated` or `bv2master` needs a Visual C++ runtime DLL |
| `tests/corpus/clsv_admin_request/valid.bin` | Replay corpus: valid `clsv_admin_request` payload (`gen_corpus`) |
| `tests/corpus/clsv_gameversion_accepted/valid.bin` | Replay corpus: valid `clsv_gameversion_accepted` payload (`gen_corpus`) |
| `tests/corpus/clsv_map_list_request/valid.bin` | Replay corpus: valid `clsv_map_list_request` payload (`gen_corpus`) |
| `tests/corpus/clsv_map_request/valid.bin` | Replay corpus: valid `clsv_map_request` payload (`gen_corpus`) |
| `tests/corpus/clsv_pickup_request/valid.bin` | Replay corpus: valid `clsv_pickup_request` payload (`gen_corpus`) |
| `tests/corpus/clsv_player_shoot/valid.bin` | Replay corpus: valid `clsv_player_shoot` payload (`gen_corpus`) |
| `tests/corpus/clsv_pong/valid.bin` | Replay corpus: valid `clsv_pong` payload (`gen_corpus`) |
| `tests/corpus/clsv_spawn_request/valid.bin` | Replay corpus: valid `clsv_spawn_request` payload (`gen_corpus`) |
| `tests/corpus/clsv_svcl_chat/valid.bin` | Replay corpus: valid `clsv_svcl_chat` payload (`gen_corpus`) |
| `tests/corpus/clsv_svcl_player_change_name/valid.bin` | Replay corpus: valid `clsv_svcl_player_change_name` payload (`gen_corpus`) |
| `tests/corpus/clsv_svcl_player_coord_frame/valid.bin` | Replay corpus: valid `clsv_svcl_player_coord_frame` payload (`gen_corpus`) |
| `tests/corpus/clsv_svcl_player_info/valid.bin` | Replay corpus: valid `clsv_svcl_player_info` payload (`gen_corpus`) |
| `tests/corpus/clsv_svcl_player_projectile/valid.bin` | Replay corpus: valid `clsv_svcl_player_projectile` payload (`gen_corpus`) |
| `tests/corpus/clsv_svcl_player_shoot_melee/valid.bin` | Replay corpus: valid `clsv_svcl_player_shoot_melee` payload (`gen_corpus`) |
| `tests/corpus/clsv_svcl_player_update_skin/valid.bin` | Replay corpus: valid `clsv_svcl_player_update_skin` payload (`gen_corpus`) |
| `tests/corpus/clsv_svcl_team_request/valid.bin` | Replay corpus: valid `clsv_svcl_team_request` payload (`gen_corpus`) |
| `tests/corpus/clsv_svcl_vote_request/valid.bin` | Replay corpus: valid `clsv_svcl_vote_request` payload (`gen_corpus`) |
| `tests/corpus/clsv_vote/valid.bin` | Replay corpus: valid `clsv_vote` payload (`gen_corpus`) |
| `tests/corpus/svcl_console/valid.bin` | Replay corpus: valid `svcl_console` payload (`gen_corpus`) |
| `tests/corpus/svcl_play_sound/valid.bin` | Replay corpus: valid `svcl_play_sound` payload (`gen_corpus`) |
| `tests/corpus/clsv_admin_request/fixed-format-string.bin` | Regression input (R23: admin login as a format string), fixed in step 4; replayed like `valid.bin` |
| `tests/corpus/clsv_map_request/fixed-unterminated.bin` | Regression input (R24: map name without `\0`), fixed in step 4; replayed like `valid.bin` |
| `tests/corpus/clsv_svcl_chat/fixed-format-string.bin` | Regression input (R21: chat text as a format string), fixed in step 4; replayed like `valid.bin` |
| `tests/corpus/clsv_svcl_team_request/fixed-playerid-out-of-range.bin` | Regression input (Q-S2: `playerID` 100), fixed in step 4; replayed like `valid.bin` |
| `tests/corpus/svcl_console/fixed-unterminated.bin` | Regression input (R22: console text without `\0`), fixed in step 4; replayed like `valid.bin` |
| `tests/corpus_types.h` | Client-to-server message types, valid samples and known-defect inputs for the corpus, replay and fuzz tests |
| `tests/gen_corpus.cpp` | Writes `tests/corpus/<type>/valid.bin` and the known-defect inputs from `netPacket.h` structs |
| `tests/harness/fake_babonet.cpp` | Test `bb_*` API: queued connects and messages in, captured sends out, client disconnects queued by the test (a server-side `bb_serverDisconnectClient` raises no event, like babonet); an object file, so it wins over babonet's `baboNet.cpp` |
| `tests/harness/server_harness.cpp` | In-process dedicated server for tests: start, connect, join, deliver, tick |
| `tests/harness/server_harness.h` | Harness API |
| `tests/e2e/e2e.py` | E2E scenarios (Phase T4): `bv2dedicated` with the admin API, `bv2master` and `bv2 --autotest` clients under its own Xvfb; join/leave, password, chat, kick, ban, master list, CTF capture, a silent connection dropped by the join timeout (ctest `e2e_<scenario>`) |
| `tests/screenshots.py` | Phase T7: runs `tests/golden/<name>.txt` and compares the frame with `<name>.png` within a tolerance; keeps actual and diff PNGs on failure, `--update` writes the golden (ADR 0026) |
| `tests/golden/main-menu.txt` | Screenshot script: main menu, news tab (ctest `screenshot_main-menu`) |
| `tests/golden/main-menu.png` | Golden for `screenshot_main-menu` |
| `tests/golden/options.txt` | Screenshot script: main menu, options tab (ctest `screenshot_options`) |
| `tests/golden/options.png` | Golden for `screenshot_options` |
| `tests/golden/scoreboard.txt` | Screenshot script: host the placeholder map, hold the scoreboard (ctest `screenshot_scoreboard`) |
| `tests/golden/scoreboard.png` | Golden for `screenshot_scoreboard` |
| `tests/autotest_client.py` | Shared by `run_autotest.py`, `screenshots.py` and `e2e/e2e.py`: starts Xvfb with `-displayfd`, builds the `bv2 --autotest` command line and env, and runs a client with a fresh pref dir (`run_client`) |
| `tests/autotest/fail.txt` | Autotest script that never passes: the client exits 1 (ctest `autotest_fail`) |
| `tests/autotest/menu-host.txt` | Autotest script: menu, `help` output, host and join the placeholder map (ctest `autotest_menu_host`) |
| `tests/autotest/quit.txt` | Autotest script that quits before its last expect: the client exits 1, not 0 (ctest `autotest_quit`) |
| `tests/autotest/timeout.txt` | Autotest script that outlasts `--timeout`: the client exits 2 (ctest `autotest_timeout`) |
| `tests/autotest/usage.txt` | Autotest script: bad arguments to `screenshot`, `menutab` and `scoreboard` print their usage (ctest `autotest_usage`) |
| `tests/run_autotest.py` | Runs `bv2 --autotest <script>` with a fresh pref dir, windowed, on placeholders (under its own Xvfb on Linux without a display, so the exit code is the client's); checks the exit code and the expected result text |
| `tests/smoke_server.py` | Starts `bv2dedicated` headless, runs the CTF script, types a `sayall` with printf specifiers, quits; checks exit code, server creation and `bv2.cfg`. `--install-under` runs it from a non-ASCII path (Windows UTF-8 code page) |
| `tests/test_admin_api.cpp` | Step 12: the admin API refuses a wrong token, limits a burst, runs a command, returns the status, audits with secrets masked (not on Windows) |
| `tests/test_admin_login.cpp` | Step 10: admin login only with HMACs over the connection's challenge; no user, password or MD5 on the wire |
| `tests/test_autotest.cpp` | Phase T3: autotest runner without a window: parsing, commands, waits, one match per expect, fail, timeout and early-quit reports (ADR 0025) |
| `tests/test_babonet.cpp` | R7: a 10000-byte message over a loopback babonet connection arrives over several reads and intact |
| `tests/test_babonet_ping.cpp` | PNS-56: a server-browser ping to a game server on this host is answered within 1 s while another process holds babonet's LAN port |
| `tests/test_browsable_game.cpp` | A server browser row owns a copy of its lobby game's row (the lobby frees its own) |
| `tests/test_cstring.cpp` | Phase C step 5: one-argument `CString` is text (arrays, `char*`, literals, direct and implicit); two or more arguments format (ADR 0021) |
| `tests/test_view_geometry.cpp` | Widescreen (Stretch) projects at the window's own aspect, Bars at 4:3; the camera's side margin widens with the view, and a narrow or short map centres it |
| `tests/test_config.cpp` | dksvar config layering, transient values not saved, secrets masked, `dksvarGetValue` |
| `tests/test_credentials.cpp` | Step 10: the credential HMAC against RFC 4231; proofs check only with their own password, label and nonce |
| `tests/test_crash_fixes.cpp` | Steps 5 and 7: one check per fixed defect (KEY_QUESTIONS Part D) on the in-process server |
| `tests/test_dks.cpp` | R20: `dksPlayMusic` on a missing file 200 times; a sound deleted after `dksShutDown` (teardown) (ASan) |
| `tests/test_example_cfg.py` | `config/bv2.example.cfg` names a new player as `GameVar.cpp` does |
| `tests/test_window_icon.cpp` | `dkwSetWindowIcon` accepts the Linux client's compiled-in icon (`AppIcon.h`) and refuses bytes that aren't a PNG |
| `tests/test_make_deb.py` | `make_deb.py` builds an installable `.deb` from the build's install: `share/bv2/main` equals `runtime/main`, layout, modes, root ownership, no master, version mapping (`-rc1` sorts before the release), Depends, launcher path (ctest `make_deb`, Linux client builds with dpkg-dev) |
| `tests/test_installer_licence.py` | The Windows installer's reflowed licence keeps every word and paragraph of `LICENSE.txt`, one line per paragraph except centred headings (ctest `installer_licence`) |
| `tests/test_discord_release_post.py` | The release post links only files the release job uploads, pings the given role and nothing else, marks a pre-release when the job says so (ctest `discord_release_post`) |
| `tests/test_app_icon.py` | The icon files hold every size each OS needs; the exe, installer, bundle, `.desktop` and `.dmg` layout point at them |
| `tests/test_app_name.py` | The menu bar, Finder/Dock bundle name, exe description, installer and `.desktop` entry all say "Robo Violence 2" |
| `tests/test_fileio.cpp` | `FileIO` byte widths for `.bvm` data and the widths the `.DKO` loader relies on; `bv2ReadBytes` zero-fills a short read; fixed-string length prefix round trip; unterminated `CString::loadFromFile` (overflows under ASan without its bound); `getString` keeps 255 characters (R16); `MemIO` reads past its end as zero; `getNextToken` on a single token (R30) |
| `tests/test_lang.cpp` | Step 13: PO parser (entries that change the English printf conversions or whose key English lacks are skipped; raw control bytes rejected), English fallback per key, OS locale matching (`ll_CC`, `ll`, `.po` before `.lang`), PO without glyphs skipped |
| `tests/test_lang_check.py` | Step 13: `tools/lang-check.py` fails on changed conversions, unknown keys, repeated keywords, tabs and a stale `en.pot`; warns about missing glyphs |
| `tests/test_join_password.cpp` | Step 10: a client joins a passworded server only with the HMAC over its own challenge; no password or MD5 of it on the wire |
| `tests/test_bans.cpp` | Server bans: 0.0.1's binary banlist moves to `banlist.tsv` and still refuses the IP; a ban past `sv_banDays` lets it in and stays in the file (ADR 0034) |
| `tests/test_player_privacy.cpp` | The player info relayed to everyone and the player list a new client gets carry no IP; the server keeps it (ADR 0034) |
| `tests/test_join_timeout.cpp` | A connection that never answers the join handshake loses its slot after `Server::JOIN_TIMEOUT`; a client past the handshake keeps it while loading |
| `tests/test_data_download.cpp` | ADR 0040: `firstMismatch` names a wrong or missing file; `replaceDir` keeps the old install when the move fails and replaces it otherwise, at `originalDataDir`, which auto-detect then finds; the browser zip's name is one auto-detect knows; the extract argv and the hash paths agree on where `main/` lands and the zip's folder names the pinned commit; the generated list holds the 324 `Content/main` hashes (ctest `data_download`, client builds) |
| `tests/test_paths.cpp` | Phase C step 1: package vs data root, `contentFile`, `findDataIn` (upstream layout, `main/` itself, Windows Extract All nesting), saved folder, auto-detect, `BV2_DATA_DIR`; the built-in master list vs `bv2.cfg`, `BV2_MASTER_SERVERS` and `none` (ADR 0028) |
| `tests/test_osv_vcpkg.py` | Phase C step 4: `tools/osv-vcpkg.py` maps SPDX download locations to upstream repository and tag (offline) |
| `tests/test_packet_hygiene.cpp` | Step 4 checks: sender binding (Q-S1), index ranges with controls (Q-S2), map names (Q-S6), skin and sound relay (Q-S7), `SV_CHANGE` filter (Q-S4), the handshake gate, `checkPacket` per message type and size, the client's `checkServerMessage` on values the server sends |
| `tests/test_retention.cpp` | `BanList`: binary banlist moved once, saved on every change, line format, matching by age without dropping, expired bans dropped on load; admin log lines older than 90 days dropped (ADR 0034) |
| `tests/test_replay.cpp` | Replays the corpus through the in-process server: no crash, expected replies; each `crash-*.bin` (none at present) is its own ctest under ASan |
| `tests/fuzz_server.cpp` | Random-mutation fuzzer for one message type on the in-process server; writes the crashing input to `<out>/<type>/last.bin` (CI job `fuzz`) |
| `tests/test_server_harness.cpp` | The in-process server starts on the placeholder CTF map, accepts a client and sends it the game state |
| `tests/test_validator.cpp` | Steps 8, 9: the step 8 kicks, `sv_antiCheat` levels, and each step 9 rule against a cheat and against normal (lagged) play |
| `tests/test_master.cpp` | In-process master on port 10207 with a scratch `master.db`: register, list, heartbeat timeout (simulated 61 s), removal by `KILL_SERV`, SQL injection in names, maps, nicks and IPs, the ban list with LIKE filters (Q-S3), the ban secret, the advertised address and a reconnect; runs on Windows since R28 was fixed |
| `tests/test_netpacket.cpp` | Byte-level layout of the packed wire structs |
| `tests/test_staging_deploy.sh` | Phase T step 2: staging deploy helpers: main-only run selection, `/status` player count, CTF and TDM player counts and verify, failures per commit, infrastructure stages, env-file mode and readonly repository (ctest `staging_deploy`) |
| `tests/test_health_watch.sh` | Container health watcher: which container states are problems, missing services from `GAME_SERVERS`, Docker not answering, two-run confirmation before an alert or recovery, a whole run against a stub `docker`, no check during a deploy but a stale lock ignored (ctest `health_watch`, Unix) |
| `tests/test_playtest_agent.sh` | Phase T step 1: playtest agent helpers: main-only run selection (fork PR from `main` rejected), artifact digest, attempt limit, env-file mode and no export, launch script map swap (ctest `playtest_agent`, Unix with `jq`) |
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
| `.github/ISSUE_TEMPLATE/bug_report.yml` | Issue form: bug report (part, OS, version, game data, logs); labels `bug`, `needs-triage` |
| `.github/ISSUE_TEMPLATE/config.yml` | Issue chooser: no blank issues; security reports go to private vulnerability reporting |
| `.github/ISSUE_TEMPLATE/feature_request.yml` | Issue form: feature request with acceptance criteria; labels `enhancement`, `needs-triage` |
| `.github/dependabot.yml` | Dependabot version updates: GitHub Actions and the server images' Docker base, weekly |

### `.claude`

| path | purpose |
|---|---|
| `.claude/agents/rv2-thermo-nuclear-review.md` | Claude Code subagent: fresh-context PR-closing review (REVIEW.md section 5) |
| `.claude/skills/rv2-thermo-nuclear-review/SKILL.md` | Claude Code skill: the only copy of the maintainability rubric the review subagent loads; its `Rubric:` hash line is checked by `tools/review.sh` ([ADR 0037](docs/decisions/0037-review-rubric-single-repo-copy.md)) |

### `.github/actions/setup-vcpkg`

| path | purpose |
|---|---|
| `.github/actions/setup-vcpkg/action.yml` | Composite action: runner's vcpkg at the pinned baseline, host packages, vcpkg binary cache; used by every workflow that builds |

### `.github/workflows`

| path | purpose |
|---|---|
| `.github/workflows/secret-scan.yml` | CI: gitleaks over commits after the fork point and original-asset check |
| `.github/workflows/build.yml` | CI: Debug and Release build and ctest, Release package on Windows, macOS, Linux (`roboviolence2-<preset>` artifacts); `package-check` of the Linux package in a clean `ubuntu:24.04`; `deb-check`: the `.deb` installs with apt, a non-root user starts the server and removal leaves nothing, in clean `ubuntu:24.04` and `debian:13`; `package-smoke` (not required): Windows and macOS packages start server, master and client (Mesa llvmpipe on Windows); ASan smoke job with client under xvfb; Linux jobs upload the actual and diff PNGs of failed `screenshot_*` tests (`screenshots-<preset>`); `release` job: every run checks the release files are there; a `v*` tag publishes them (GitHub release, pre-release for `v0.*` and `v*-*`) and posts to Discord; `fuzz` job (not required, pushes to main): 2 min per client message type, uploads crash inputs |
| `.github/workflows/hygiene.yml` | CI: ARCHITECTURE.md inventory, repository hygiene, content case; `review` (`tools/review.sh`, given the PR description) on PRs, rerun on description edits |
| `.github/pull_request_template.md` | PR description: item, goal, out of scope, Allowed paths (read by `tools/review.sh`), tests |
| `.github/workflows/codeql.yml` | CodeQL: `c-cpp` (manual linux-x64 build of the three executables) and `actions`; push, PR, weekly; alerts under `build/` (vcpkg headers) filtered out |
| `.github/workflows/dependency-graph.yml` | Submits resolved vcpkg ports to the dependency graph on push to `main` |
| `.github/workflows/osv-vcpkg.yml` | Weekly OSV check of the vcpkg ports by upstream commit (`tools/osv-vcpkg.py`); not required |
| `.github/workflows/containers.yml` | CI: builds both server images, checks non-root, runs the compose example (healthy, both passworded servers listed by the master under one `BV2_PUBLIC_ADDRESS`), checks both answer the server browser's UDP ping, checks a remote `status` of CTF and TDM over TLS and `/deployed`, pushes multi-arch `:<sha>` and `:latest` to GHCR from `main` and weekly |

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
| `content/LaunchScript/CTF.cfg` | Example dedicated-server launch script (CTF, port 3333; map is a placeholder) |
| `content/LaunchScript/TDM.cfg` | Dedicated-server launch script (TDM, port 3334; map is a placeholder); the compose example's `tdm` service |

### `content/languages`

| path | purpose |
|---|---|
| `content/languages/README.md` | Translator guide: add a language from `en.pot`, how the game picks one, the glyph limit |
| `content/languages/en.lang` | English strings: the source and per-key fallback for every language |
| `content/languages/en.pot` | Translation template generated from `en.lang` (`tools/lang-to-pot.py`) |

### `docs`

| path | purpose |
|---|---|

### `docs/build`

| path | purpose |
|---|---|
| `docs/build/README.md` | Download vs build, game data, playing a local game, tests |
| `docs/build/linux.md` | Linux build and run guide |
| `docs/build/macos.md` | macOS build and run guide |
| `docs/build/windows.md` | Windows build and run guide |

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
| `docs/decisions/0012-windows-10-22h2-minimum.md` | ADR 0012 |
| `docs/decisions/0013-r3-restore-normal-and-pro-rulesets.md` | ADR: `sv_serverType` 0 plays the Normal rules again (R3) |
| `docs/decisions/0014-libsodium-hmac-for-credentials.md` | ADR: libsodium for the credential HMAC and nonces (step 10) |
| `docs/decisions/0015-admin-http-api-behind-tls-proxy.md` | ADR: remote admin over an HTTP API behind Caddy TLS; legacy UDP remote admin refused (step 12) |
| `docs/decisions/0016-player-data-lookup-and-package-content.md` | ADR: client finds or asks for the player's BV2 data; launch scripts, languages and default config come from the package (Phase C step 1) |
| `docs/decisions/0017-playtest-pull-agent.md` | ADR: main builds tested on the owner's Mac by a pull agent against a disposable local server; no self-hosted runner (phase T step 1) |
| `docs/decisions/0018-staging-pull-deploy.md` | ADR: staging host deploys the newest main images by pull, pinned to a commit, only with no players; `/deployed`; multi-arch images (phase T step 2); amendment: production runs the same agent by hand (`RV2_DEPLOY_LABEL=prod`, `--sha`, no cron) |
| `docs/decisions/0019-in-game-name-and-fork-credits.md` | ADR: in-game name, fork credits, "LOADING" splash, survey removed (Phase C step 2) |
| `docs/decisions/0020-package-and-release-format.md` | ADR: package, installer, `.dmg` and release format; engine-only public releases (Phase C step 3) |
| `docs/decisions/0021-cstring-one-argument-is-text.md` | ADR: one-argument `CString` copies text; two or more arguments format (Phase C step 5) |
| `docs/decisions/0022-advertised-server-address.md` | ADR: the master lists `sv_publicAddress` only from private sources; `KILL_SERV` by connection (Phase C step 6) |
| `docs/decisions/0023-step-review-subagent-in-repo.md` | ADR: step-closing review subagent and rubric tracked in `.claude/` |
| `docs/decisions/0024-github-project-is-the-backlog.md` | ADR: the private GitHub Project is the backlog; roadmap files removed; PR carries scope and review record |
| `docs/decisions/0025-client-autotest-mode.md` | ADR: the client runs scripted console commands and expectations (`--autotest`) with exit codes 0/1/2 and a watchdog |
| `docs/decisions/0026-screenshot-goldens.md` | ADR: Linux llvmpipe screenshot goldens compared within a tolerance; the `screenshot`, `menutab` and `scoreboard` commands |
| `docs/decisions/0027-several-game-servers-per-host.md` | ADR: several game servers per host (CTF, TDM): one image, `BV2_HEALTH_PORT`, one env file, admin APIs at `/` and `/tdm/`, deploy waits for both |
| `docs/decisions/0028-built-in-public-master.md` | ADR: `cl_masterServers` defaults to `master.roboviolence.com:10207`; `bv2.cfg` neither saves nor sets it; `none` turns it off |
| `docs/decisions/0030-app-icon-and-dmg-layout.md` | ADR: committed application icon per OS (exe, bundle, Linux window icon); `.dmg` with only the app and Applications |
| `docs/decisions/0031-linux-deb-package.md` | ADR: Linux `.deb` with the client and `bv2dedicated` in `/usr/games`, beside the `.tar.gz`; reverses ADR 0020's rejection |
| `docs/decisions/0032-discord-release-post.md` | ADR: the release job posts each release to Discord `#announcements` through a webhook secret, pinging only the opt-in role |
| `docs/decisions/0033-first-run-dialog-recommends-bv2-zip.md` | ADR: the first-run data dialog recommends the original BV2 zip, shows its URL and the folder to choose, and has a Download button (amends 0016) |
| `docs/decisions/0034-player-data-retention-and-privacy-policy.md` | ADR: the server never relays player IPs, bans (`banlist.tsv`) expire after `sv_banDays`, `admin-audit.log` after 90 days; privacy policy page, installer link and opt-out |
| `docs/decisions/0036-website-in-rv2-website-repository.md` | ADR: the website, its tests, tooling and Pages workflow live in Hub-Tansky/rv2-website; its tests read `build.yml` from here |
| `docs/decisions/0037-review-rubric-single-repo-copy.md` | ADR: the review rubric has one repo copy with a unique name; the gate checks its hash |
| `docs/decisions/0038-dead-accounts-hidden-admin-and-ban-secrets-hardened.md` | ADR: dead account pages hidden; 15-character admin and master ban secrets, per-IP admin lockout, HMAC-signed master bans |
| `docs/decisions/0039-widescreen-stretch-shows-more-map.md` | ADR: the gameplay, scope and intro projections use their viewport's aspect; Stretch shows more map sideways (Hor+), Bars stays 4:3 |
| `docs/decisions/0040-first-run-downloads-original-data.md` | ADR: the first-run box downloads the original data of a pinned upstream commit with the OS's curl and tar/unzip, checks the hashes and installs it in `original` beside the pref dir; supersedes 0033 |
| `docs/decisions/README.md` | ADR format and index |

### `docs/ops`

| path | purpose |
|---|---|
| `docs/ops/release.md` | Cutting a release: pick the commit, manual install test per OS, tag, notes header template, production re-pin, Discord publish |
| `docs/ops/runbook.md` | Hosting the server images: secrets, start, stop, upgrade, logs, console, ports, backups, production (pinned manual deploy, failover) |

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

### `engine/credentials`

| path | purpose |
|---|---|
| `engine/credentials/CMakeLists.txt` | Target credentials |
| `engine/credentials/README.md` | Module README: credentials |

### `engine/credentials/include`

| path | purpose |
|---|---|
| `engine/credentials/include/Credentials.h` | Credential proofs: HMAC(password, label + nonce) for join and admin, signed packets for master bans, `MIN_SECRET_LENGTH` |

### `engine/credentials/src`

| path | purpose |
|---|---|
| `engine/credentials/src/Credentials.cpp` | HMAC-SHA256, nonces and constant-time checks (libsodium, ADR 0014, 0038) |

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
| `engine/zeven/include/dkw.h` | dkw: window creation and events, window icon |
| `engine/zeven/include/glheaders.h` | The only OpenGL include: glad, then GLU |
| `engine/zeven/include/platform.h` | Platform macros (BV2_PLATFORM_*, BV2_POSIX), INT4/UINT4, `bv2ReadBytes` (checked `fread` for the engine loaders and `FileIO`), POSIX includes and Win32 type shims; force-included |

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
| `engine/zeven/src/dkw.cpp` | dkw: window creation and events, window icon |

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
| `game/CMakeLists.txt` | Targets `bv2server_lib` (the CONSOLE file list without `main.cpp`; also linked by the tests), bv2dedicated (`main.cpp` + `bv2server_lib`) and bv2 (client, with the per-OS application icon; on Linux `bv2_app_icon` generates `AppIcon.h`); `bv2_original_hashes` generates `OriginalHashes.h` from `tools/original-assets.sha256` (ADR 0040) |
| `game/README.md` | Module README: game |

### `game/src`

| path | purpose |
|---|---|
| `game/src/AccountManager.cpp` | Account create/login/clan/friend requests over the master connection |
| `game/src/AccountManager.h` | Account create/login/clan/friend requests over the master connection |
| `game/src/AdminApi.cpp` | Admin HTTP API: non-blocking listener on the game thread, bearer token, per-source budget, connection cap, refused scene-rebuilding commands, audit log with secrets masked, pruned hourly to 90 days (step 12, ADR 0034) |
| `game/src/AdminApi.h` | Admin HTTP API (`POST /command`, `GET /status`) behind a TLS proxy, ADR 0015 |
| `game/src/Autotest.cpp` | `--autotest` script runner: parse, run commands, waits, `expect-console`, `expect-scene`, result report (ADR 0025) |
| `game/src/Autotest.h` | Autotest runner API; exit codes 0 pass, 1 fail, 2 timeout |
| `game/src/Button.cpp` | Menu button widget |
| `game/src/Button.h` | Menu button widget |
| `game/src/CAStar.cpp` | A* path finding grid |
| `game/src/CAStar.h` | A* path finding grid |
| `game/src/CAStar_FindPath.cpp` | A* search routine |
| `game/src/CAccount.cpp` | Account UI tab (hidden, ADR 0038) |
| `game/src/CAccount.h` | Account UI tab (hidden, ADR 0038) |
| `game/src/CBrowser.cpp` | Server browser UI |
| `game/src/CBrowser.h` | Server browser UI |
| `game/src/CClans.cpp` | Clans UI tab |
| `game/src/CClans.h` | Clans UI tab |
| `game/src/CControl.cpp` | Generic UI control (button, label, list, text box) of the newer menu |
| `game/src/CControl.h` | Generic UI control (button, label, list, text box) of the newer menu |
| `game/src/CCredit.cpp` | Credits tab: Robo Violence 2 fork notice, then the original credits (ADR 0019) |
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
| `game/src/CProfile.cpp` | Appearance UI tab (its sub-tab row is hidden, ADR 0038) |
| `game/src/CProfile.h` | Appearance UI tab |
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
| `game/src/ClientRecv.cpp` | Client: handlers for every server-to-client packet, after `packet::checkServerMessage` |
| `game/src/ClientRender.cpp` | Client: in-game HUD and view rendering |
| `game/src/ConfirmPass.cpp` | Password confirmation dialog |
| `game/src/ConfirmPass.h` | Password confirmation dialog |
| `game/src/ConnectFailed.h` | Dialog: connection failed |
| `game/src/Console.cpp` | In-game console: commands, log, remote console; feeds console lines to a running autotest; `refreshgames` asks the master for the game list; `screenshot`, `menutab` and `scoreboard` for screenshot tests (ADR 0026) |
| `game/src/Console.h` | In-game console: commands, log, remote console |
| `game/src/Control.cpp` | Menu control base class |
| `game/src/Control.h` | Menu control base class |
| `game/src/ControlListener.cpp` | Menu control listener interface |
| `game/src/ControlListener.h` | Menu control listener interface |
| `game/src/CreateGame.h` | Create-game screen |
| `game/src/Credits.h` | Credits screen |
| `game/src/DataDirPicker.cpp` | Client start-up: saved or detected BV2 data folder, else a first-run message box: "Download and play" runs `DataDownload`'s `curl` and `tar`/`unzip` argv (`SDL_CreateProcess`) with a progress window, or placeholders; a failure box offers retry, the browser or placeholders (ADR 0016, 0040) |
| `game/src/DataDirPicker.h` | Interface of `DataDirPicker.cpp` |
| `game/src/DataDownload.cpp` | First-run download without SDL: the pinned upstream zip and the browser fallback URL, the per-OS `curl` and `tar`/`unzip` argv and where `main/` lands, SHA-256 of a file (libsodium), the hash check against the compiled-in list, and a folder swap that keeps the old folder if it fails (ADR 0040) |
| `game/src/DataDownload.h` | Interface of `DataDownload.cpp`; `AssetHash` |
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
| `game/src/Game.h` | Game state: players, projectiles, items, flags, round and mode logic (shared by client and server); `forceStats` holds the scoreboard |
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
| `game/src/IntroScreen.cpp` | "LOADING" splash after start-up (ADR 0019), the loading frame Scene and Map present, the per-frame set-up `Scene::render` starts with, and `view::current` |
| `game/src/IntroScreen.h` | Interface of `IntroScreen.cpp`; `presentLoadingFrame` is the one "LOADING" frame Scene and Map show while they load; `beginFrame` is the shared clear/viewport/projection set-up |
| `game/src/JoinGame.h` | Join-game screen |
| `game/src/Key.cpp` | Key binding entry |
| `game/src/Key.h` | Key binding entry |
| `game/src/KeyManager.cpp` | Key bindings manager |
| `game/src/KeyManager.h` | Key bindings manager |
| `game/src/Label.cpp` | Menu label widget |
| `game/src/Label.h` | Menu label widget |
| `game/src/Lang.cpp` | Reads `.lang` and PO language files into the font's encoding; picks the file for the OS locales (ADR 0011) |
| `game/src/Lang.h` | Language file API: `readLang`, `parsePo`, `readPo`, `pickFile`, `load` (English, then the chosen file) |
| `game/src/MainMenu.h` | Main menu screen |
| `game/src/Map.cpp` | Map data, .bvm loading and saving, collision, rendering resources |
| `game/src/Map.h` | Map data, .bvm loading and saving, collision, rendering resources |
| `game/src/MapRender.cpp` | Map rendering (tiles, walls, dirt, optional 3D model map) |
| `game/src/MemIO.cpp` | In-memory buffer reader/writer |
| `game/src/PacketCheck.cpp` | What each side accepts from the wire: client message sizes and `playerID` offsets, server message sizes, index fields and string ends |
| `game/src/PacketCheck.h` | Interface of `PacketCheck.cpp` (`packet::clientRule`, `knownWeapon`, `checkServerMessage`) |
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
| `game/src/ReportGen.cpp` | XML server report generation |
| `game/src/ReportGen.h` | XML server report generation |
| `game/src/Retention.cpp` | Retention of personal data on disk: `BanList` (load with the 0.0.1 binary banlist moved once, add, remove, match by `sv_banDays`, atomic save), admin log stamp and pruning (ADR 0034) |
| `game/src/Retention.h` | Retention of personal data on disk: `retention::BanList`, `utcStamp`, `expireLog`, `AUDIT_DAYS` (ADR 0034) |
| `game/src/Scene.cpp` | Top-level scene: main loop, menus, game and editor switching, rendering setup |
| `game/src/Scene.h` | Top-level scene: main loop, menus, game and editor switching, rendering setup |
| `game/src/SceneNet.cpp` | Scene: starts and stops client and server, hosting and joining; kick, ban and unban through `Server::bans` (ADR 0034) |
| `game/src/Server.cpp` | Authoritative server: connections, joins, tick, map and mode control; bans in `Server::bans` (`retention::BanList`, ADR 0034) |
| `game/src/Server.h` | Authoritative server: connections, joins, tick, map and mode control |
| `game/src/ServerCTF.cpp` | Server: capture-the-flag rules |
| `game/src/ServerClose.h` | Dialog shown when the server closes the connection |
| `game/src/ServerRecv.cpp` | Server: handlers for every client-to-server packet, after the pre-dispatch check `Server::checkPacket`; a wrong join password frees the slot; relayed player info and the player list carry no IP (ADR 0034) |
| `game/src/ServerSnD.cpp` | Server: search-and-destroy rules |
| `game/src/ServerValidator.cpp` | Server cheat checks: spawn weapon whitelist, speed/frame kick, step 9 rules (weapon, movement, rate limits) with a decaying score |
| `game/src/ServerValidator.h` | Server cheat checks (`Server::validator`), asked by the `ServerRecv.cpp` handlers |
| `game/src/SqlStatement.h` | Prepared SQLite statement with bound parameters (the game's copy) |
| `game/src/ViewGeometry.h` | ([ADR 0039](docs/decisions/0039-widescreen-stretch-shows-more-map.md)) The 3D viewport inside the window (`view::rect`: whole window, or 4:3 between bars; `view::current` for this frame, defined in `IntroScreen.cpp`), the aspect the gameplay, scope and intro projections use, and the camera's margins at the map edges |
| `game/src/Weapon.cpp` | Weapon definitions and stats |
| `game/src/Weapon.h` | Weapon definitions and stats |
| `game/src/Write.cpp` | Chat input widget |
| `game/src/Write.h` | Chat input widget |
| `game/src/Writting.cpp` | Chat input handling |
| `game/src/Writting.h` | Chat input handling |
| `game/src/WrongVersion.h` | Dialog: client/server version mismatch |
| `game/src/Zeven.h` | Umbrella include for engine headers and common types |
| `game/src/changes-log.txt` | Upstream change log |
| `game/src/main.cpp` | Program entry: window, engine init, main loop; `--autotest` startup, per-step hook and watchdog (ADR 0025); the dedicated server line-buffers stdout; saves a requested screenshot before the swap |
| `game/src/netPacket.h` | Wire structs and message IDs (raw memcpy'd structs) |
| `game/src/screengrab.cpp` | Screenshot capture (P and L keys, `screenshot` command): the next frame, from the back buffer before the swap |
| `game/src/screengrab.h` | Screenshot capture |
| `game/src/tinyxml.cpp` | TinyXML (third-party XML parser) |
| `game/src/tinyxml.h` | TinyXML (third-party XML parser) |
| `game/src/tinyxmlerror.cpp` | TinyXML (third-party XML parser) |
| `game/src/tinyxmlparser.cpp` | TinyXML (third-party XML parser) |

### `masterserver`

| path | purpose |
|---|---|
| `masterserver/CMakeLists.txt` | Target bv2master and, for tests, `bv2master_lib` (the master without `main.cpp`) |
| `masterserver/README.md` | Module README: master server |

### `masterserver/src`

| path | purpose |
|---|---|
| `masterserver/src/MasterClient.cpp` | Master: connected-client record (reconstructed) |
| `masterserver/src/MasterClient.h` | Master: connected-client record (reconstructed; see masterserver/README.md) |
| `masterserver/src/SqlStatement.h` | Prepared SQLite statement with bound parameters (the master's copy) |
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
| `tools/CMakeLists.txt` | Database seeding (bv2_seed_db_tool, master.db, web.db) |
| `tools/osv-vcpkg.py` | Checks the installed vcpkg ports against OSV by upstream commit (PNS-22) |
| `tools/FORK_BASE` | First commit after upstream; secret scans start here |
| `tools/check-architecture.ps1` | Fails if ARCHITECTURE.md and the tracked files disagree (Windows) |
| `tools/check-architecture.sh` | Fails if ARCHITECTURE.md and the tracked files disagree (Unix) |
| `tools/check-encoding.py` | Fails on non-UTF-8, BOM or CR in tracked text files |
| `tools/check-original-assets.py` | Fails if a tracked file matches an original-asset hash |
| `tools/original-assets.sha256` | SHA-256 of every removed original file; its `Content/main` entries are compiled into the client as `OriginalHashes.h` (ADR 0040) |
| `tools/convert-encoding.py` | One-off UTF-8/LF converter used in step 1.4 |
| `tools/placeholder-manifest.tsv` | Files the game loads at startup and their formats, for the placeholder generator |
| `tools/gen-placeholder-content.py` | Writes placeholder maps, textures, a readable font atlas, sounds and models for dev and CI |
| `tools/gen-app-icon.py` | Renders the application icons and the `.dmg` background into `packaging/` (by hand; numpy, Pillow) |
| `tools/check-hygiene.py` | Fails on spaces in paths, tracked ignored files, files over 5 MB, bad encoding, failed translation check |
| `tools/lang-check.py` | Fails when `en.pot` is stale, a `.po` is invalid, has unknown keys or changes printf conversions; warns about missing glyphs; lists untranslated keys |
| `tools/lang-to-pot.py` | Generates `content/languages/en.pot` from `en.lang` |
| `tools/seed_db.cpp` | Creates a SQLite DB from SQL files (build helper) |
| `tools/playtest/README.md` | Playtest agent: install, env file, commands, files, uninstall |
| `tools/playtest/agent.sh` | Playtest pull agent (macOS): fetches and verifies successful `main` packages, tests the client against a disposable local server (placeholders, or original data from `BV2_DATA_DIR` on `RV2_PLAYTEST_MAP`) or staging, reports and notifies (ADR 0017) |
| `tools/playtest/com.roboviolence2.playtest.plist` | LaunchAgent template for the playtest agent (every 30 min, script in `~/Dev/RoboViolence2/tools/automatic_tests/`) |
| `tools/staging/cloud-init.yml` | Staging host bootstrap (cloud-init, Ubuntu 24.04): Docker, `deploy.sh`, `rv2-staging-setup` for the env file, cron and first deploy |
| `tools/staging/README.md` | Staging deploy agent: host needs, install, env file, cron, commands, files |
| `tools/staging/health-watch.sh` | Container health watcher beside `deploy.sh` (cron, every minute): ntfy when a service of the stack is unhealthy, stopped, restarting or missing for two runs in a row, and when all are well again; skips while a deploy runs |
| `tools/staging/deploy.sh` | Staging deploy agent (Linux, cron; production runs it by hand with `RV2_DEPLOY_LABEL=prod`): newest successful main `containers` build, pinned to its commit, deployed only with no players on CTF and TDM, rollback, `/deployed` (ADR 0018) |
| `tools/review.sh` | Review gate (REVIEW.md section 1): all checks, scope against the PR's Allowed paths, anti-gaming, review gate on the PR's review record; `--full` builds and tests |
| `tools/setup-dev.ps1` | Activates hooks, checks tools (Windows) |
| `tools/setup-dev.sh` | Activates hooks, checks tools (Unix); prints the apt line for building and the package names for running a package |
| `tools/vcpkg-triplets/arm64-osx.cmake` | vcpkg overlay triplet: macOS arm64 ports built for macOS 12 (PNS-26) |

