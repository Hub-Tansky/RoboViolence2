# game

The game: client, listen server, dedicated server and map editor from one codebase. `CONSOLE` selects the headless server (`bv2dedicated`); `bv2` is the client.

- **Entry:** `src/main.cpp`; top-level loop in `src/Scene.cpp`, hosting in `src/SceneNet.cpp`.
- **Simulation:** `Game.cpp` (shared state), `Server*.cpp` (authoritative), `Client*.cpp`, `Player*.cpp`, `Map*.cpp`, `Weapon.cpp`.
- **Wire format:** `src/netPacket.h` (raw structs). Changing a layout needs a `GAME_VERSION_SV/CL` bump (`Server.h`, `Client.h`).
- **Variables:** `GameVar.cpp` registers every `sv_*`/`cl_*` variable.
- **Depends on:** `babonet`, `dko`, `zeven_core` (`zeven_client` for `bv2`), SQLite, libcurl (`BV2_WITH_HTTP`).
- **Autotest:** `bv2 --autotest <script> [--timeout <s>]` runs console commands, `wait <s>`, `expect-console <text>` and `expect-scene intro|menu|loading|game|editor`, one per line; exits 0 only on a pass, 1 on a failed expect, bad script or early quit or init failure, 2 on timeout ([ADR 0025](../docs/decisions/0025-client-autotest-mode.md)). Examples: `tests/autotest/`. `tests/e2e/e2e.py` drives several clients against a dedicated server and master; `refreshgames` requests the master list. `screenshot <name>`, `menutab <tab>` and `scoreboard 0|1` feed the screenshot goldens in `tests/golden/` ([ADR 0026](../docs/decisions/0026-screenshot-goldens.md)).
- **Server file list:** `game/CMakeLists.txt` lists the files that produce code under `CONSOLE`. A file that is empty there (menus, editor, rendering) is left out.

## Gotchas

- Sixteen old-menu `.cpp` files (`MainMenu.cpp`, `Menu.cpp`, `CreateGame.cpp`, ...) were not in the Visual Studio project and are not built. Treat them as dead.
- The package (runtime dir) provides `main/` ([ADR 0005](../docs/decisions/0005-runtime-main-data-root.md)); the build creates `build/<preset>/runtime/`.
- The client finds the player's BV2 data (`BV2_DATA_DIR`, saved folder, auto-detect, first-run download in `DataDirPicker.cpp` and `DataDownload.cpp`, [ADR 0040](../docs/decisions/0040-first-run-downloads-original-data.md)); launch scripts, languages and `bv2.example.cfg` come from the package via `bv2::contentFile` ([ADR 0016](../docs/decisions/0016-player-data-lookup-and-package-content.md)). "Use placeholders" is remembered in `data-dir.txt`, Options → Game assets changes the folder ([ADR 0041](../docs/decisions/0041-game-assets-option-remembers-placeholders.md)); `--placeholders` skips the box for one run.
- UBSan on the dedicated server reports member calls on a null `Server` (`Console.cpp:833`, `Server.cpp:1476`); Step 4 fixes them.
- Analysis: [../docs/analysis/README.md](https://github.com/Hub-Tansky/RoboViolence2/blob/fc0aff7c5aa946406c7c110e8e7c2d972a7daeb8/docs/analysis/README.md), defects: [../docs/analysis/KEY_QUESTIONS.md](https://github.com/Hub-Tansky/RoboViolence2/blob/fc0aff7c5aa946406c7c110e8e7c2d972a7daeb8/docs/analysis/KEY_QUESTIONS.md).
