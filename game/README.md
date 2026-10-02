# game

The game: client, listen server, dedicated server and map editor from one codebase. `CONSOLE` selects the headless server (`bv2dedicated`); `bv2` is the client.

- **Entry:** `src/main.cpp`; top-level loop in `src/Scene.cpp`, hosting in `src/SceneNet.cpp`.
- **Simulation:** `Game.cpp` (shared state), `Server*.cpp` (authoritative), `Client*.cpp`, `Player*.cpp`, `Map*.cpp`, `Weapon.cpp`.
- **Wire format:** `src/netPacket.h` (raw structs). Changing a layout needs a `GAME_VERSION_SV/CL` bump (`Server.h`, `Client.h`).
- **Variables:** `GameVar.cpp` registers every `sv_*`/`cl_*` variable.
- **Depends on:** `babonet`, `dko`, `zeven_core` (`zeven_client` for `bv2`), SQLite, libcurl (`BV2_WITH_HTTP`).
- **Server file list:** `game/CMakeLists.txt` lists the files that produce code under `CONSOLE`. A file that is empty there (menus, editor, rendering) is left out.

## Gotchas

- Sixteen old-menu `.cpp` files (`MainMenu.cpp`, `Menu.cpp`, `CreateGame.cpp`, ...) were not in the Visual Studio project and are not built. Treat them as dead.
- Run from a runtime dir containing `main/` ([ADR 0005](../docs/decisions/0005-runtime-main-data-root.md)); the build creates `build/<preset>/runtime/`.
- UBSan on the dedicated server reports member calls on a null `Server` (`Console.cpp:833`, `Server.cpp:1476`); Step 4 fixes them.
- Analysis: [../docs/analysis/README.md](../docs/analysis/README.md), defects: [../docs/analysis/KEY_QUESTIONS.md](../docs/analysis/KEY_QUESTIONS.md).
