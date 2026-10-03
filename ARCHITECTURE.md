# Architecture

RoboViolence 2: unofficial fork of BaboViolent 2, a top-down multiplayer shooter (C++, GPLv3). State after Step 1: sources cleaned, no build system yet.

## Summary

| Topic | Fact |
|---|---|
| Deliverables | Client, dedicated server (`CONSOLE` build), master server. Only the root `Makefile` (Linux server) exists; it can't build until Steps 2 and 3. |
| Platforms | Targets: Linux, macOS, Windows 11. Phase A plan: [docs/refactoring/README.md](docs/refactoring/README.md). |
| Modules | `BaboViolent2/Code` game; `Engine/babonet` networking (`bb_*`); `Engine/DukZeven` engine utilities (`dkc dkf dkgl dki dkp dks dksvar dkt dkw`); `Engine/dko` model loader; `MasterServer/Source` master server. |
| Tick | Fixed 30 Hz: every `update(float delay)` gets `1/30`; "frames" are a time unit (30 = 1 s). |
| Network | TCP, raw structs `memcpy`'d from `BaboViolent2/Code/netPacket.h`. The server is authoritative for hits, damage, spawns, projectiles and flags; clients for their own movement. `playerID` (slot) differs from `babonetID` (connection). Protocol version `GAME_VERSION_SV/CL` = 21100 (Pro). |
| Variants | `CONSOLE` = headless server. Direct3D, non-Pro and VLD code were removed. |
| Config and secrets | Tracked: `config/*.example.cfg` with empty secrets, `content-seed/*.sql` for generated DBs. Real values stay local; gitleaks, hooks and CI enforce it ([config/README.md](config/README.md)). |
| Assets | The original assets are removed and blocked by hash ([docs/ASSETS-LICENSE.md](docs/ASSETS-LICENSE.md)). Only `languages/en.lang` and `LaunchScript/` remain in `BaboViolent2/Content/main/`. |
| Encoding | UTF-8 without BOM, LF; `tools/check-encoding.py`. Some comments hold U+FFFD where upstream lost accents. |
| Known defects | [docs/analysis/KEY_QUESTIONS.md](docs/analysis/KEY_QUESTIONS.md). |
| Decisions | [docs/decisions/README.md](docs/decisions/README.md): 0001 Ninja, 0002 libcurl compiled out, 0003 OpenGL 2.1 kept, 0004 project name. |

### Open items from Step 1

- Master server: forked babonet/dkc copies were deleted; it uses `Engine/` and is unverified until Step 2 builds it. Its `CClient`/`CServer` differ from `Engine/babonet` `cClient`/`cServer` only by case; rename before building both on macOS or Windows. Its `cMSstruct.h` is an older subset of `BaboViolent2/inc/cMSstruct.h`.
- `BaboViolent2/Code/bv2.bmp` and `icon1.ico` are original artwork used by the Windows resource; replace in §H.
- Vendored `glext.h` copies remain (`BaboViolent2/Code`, `Engine/DukZeven/{Code,inc}`, `Engine/dko/Code/gl`); Step 3 replaces them.
- The Windows project (`BaboViolent2.vcxproj`) still carries old defines (`_PRO_`, `_DX_`, `_MD5CODESEG_`); Step 2 replaces it.

## File inventory

One row per tracked file. `tools/check-architecture.sh` fails when this list and `git ls-files` differ. Assets: per-file catalogue is [docs/assets/ASSET-INVENTORY.md](docs/assets/ASSET-INVENTORY.md); replacement asset folders get one row per directory.


### `.`

| path | purpose |
|---|---|
| `.editorconfig` | Editor settings: UTF-8, LF |
| `.git-blame-ignore-revs` | Commits ignored by git blame (encoding conversion) |
| `.gitattributes` | Line-ending and binary attributes |
| `.gitignore` | Ignore rules (OS, editors, builds, secrets, original assets) |
| `.gitleaks.toml` | gitleaks rules: default plus config secrets, public IPs, master-server rows, hosts |
| `AGENTS.md` | Canonical agent and contributor instructions |
| `ARCHITECTURE.md` | This file: summary and file inventory |
| `CLAUDE.md` | Imports AGENTS.md for Claude Code |
| `LICENSE.txt` | GPLv3 text (code only) |
| `Makefile` | Root Makefile: Linux dedicated server (g++ plus Engine sub-makes) |
| `README.md` | Project overview, fork and asset-removal statement |

### `.githooks`

| path | purpose |
|---|---|
| `.githooks/pre-commit` | Pre-commit: identity, gitleaks, original-asset and encoding checks |
| `.githooks/pre-push` | Pre-push: remote and author identity check |

### `.github/workflows`

| path | purpose |
|---|---|
| `.github/workflows/secret-scan.yml` | CI: gitleaks over commits after the fork point and original-asset check |

### `BaboViolent2/Code`

| path | purpose |
|---|---|
| `BaboViolent2/Code/AccountManager.cpp` | Account create/login/clan/friend requests over the master connection |
| `BaboViolent2/Code/AccountManager.h` | Account create/login/clan/friend requests over the master connection |
| `BaboViolent2/Code/BaboViolent2.vcxproj` | Visual Studio project; source list reference until Step 2 replaces it |
| `BaboViolent2/Code/BaboViolent2.vcxproj.filters` | Visual Studio filters for the project |
| `BaboViolent2/Code/Button.cpp` | Menu button widget |
| `BaboViolent2/Code/Button.h` | Menu button widget |
| `BaboViolent2/Code/CAStar.cpp` | A* path finding grid |
| `BaboViolent2/Code/CAStar.h` | A* path finding grid |
| `BaboViolent2/Code/CAStar_FindPath.cpp` | A* search routine |
| `BaboViolent2/Code/CAccount.cpp` | Account UI tab |
| `BaboViolent2/Code/CAccount.h` | Account UI tab |
| `BaboViolent2/Code/CBrowser.cpp` | Server browser UI |
| `BaboViolent2/Code/CBrowser.h` | Server browser UI |
| `BaboViolent2/Code/CClans.cpp` | Clans UI tab |
| `BaboViolent2/Code/CClans.h` | Clans UI tab |
| `BaboViolent2/Code/CControl.cpp` | Generic UI control (button, label, list, text box) of the newer menu |
| `BaboViolent2/Code/CControl.h` | Generic UI control (button, label, list, text box) of the newer menu |
| `BaboViolent2/Code/CCredit.cpp` | Credits tab |
| `BaboViolent2/Code/CCredit.h` | Credits tab |
| `BaboViolent2/Code/CCurl.cpp` | HTTP requests through libcurl (compiled out in Phase A) |
| `BaboViolent2/Code/CCurl.h` | HTTP requests through libcurl (compiled out in Phase A) |
| `BaboViolent2/Code/CEditor.cpp` | Map editor UI (newer) |
| `BaboViolent2/Code/CEditor.h` | Map editor UI (newer) |
| `BaboViolent2/Code/CFriends.cpp` | Friends list UI and requests |
| `BaboViolent2/Code/CFriends.h` | Friends list UI and requests |
| `BaboViolent2/Code/CHost.cpp` | Host-game UI |
| `BaboViolent2/Code/CHost.h` | Host-game UI |
| `BaboViolent2/Code/CLava.cpp` | Lava rendering and effect |
| `BaboViolent2/Code/CLava.h` | Lava rendering and effect |
| `BaboViolent2/Code/CListener.cpp` | Listener interface for CControl events |
| `BaboViolent2/Code/CListener.h` | Listener interface for CControl events |
| `BaboViolent2/Code/CLobby.cpp` | Lobby and menu tab host |
| `BaboViolent2/Code/CLobby.h` | Lobby and menu tab host |
| `BaboViolent2/Code/CMainTab.cpp` | Main menu tab |
| `BaboViolent2/Code/CMainTab.h` | Main menu tab |
| `BaboViolent2/Code/CMaster.cpp` | Master-server client: server list, remote admin, account manager packets |
| `BaboViolent2/Code/CMaster.h` | Master-server client: server list, remote admin, account manager packets |
| `BaboViolent2/Code/CMaterial.cpp` | Material/texture state wrapper |
| `BaboViolent2/Code/CMaterial.h` | Material/texture state wrapper |
| `BaboViolent2/Code/CMatrix.cpp` | Engine copy of the shared math/string type |
| `BaboViolent2/Code/CMatrix.h` | Engine copy of the shared math/string type |
| `BaboViolent2/Code/CMenuManager.cpp` | Manager for the newer menu controls |
| `BaboViolent2/Code/CMenuManager.h` | Manager for the newer menu controls |
| `BaboViolent2/Code/CMesh.cpp` | Mesh container |
| `BaboViolent2/Code/CMesh.h` | Mesh container |
| `BaboViolent2/Code/CMeshBuilder.cpp` | Mesh builder |
| `BaboViolent2/Code/CMeshBuilder.h` | Mesh builder |
| `BaboViolent2/Code/CNews.cpp` | News feed UI |
| `BaboViolent2/Code/CNews.h` | News feed UI |
| `BaboViolent2/Code/COption.cpp` | Options UI |
| `BaboViolent2/Code/COption.h` | Options UI |
| `BaboViolent2/Code/CPanel.h` | UI panel container |
| `BaboViolent2/Code/CPathNode.cpp` | Path node for A* |
| `BaboViolent2/Code/CPathNode.h` | Path node for A* |
| `BaboViolent2/Code/CPing.cpp` | Server ping measurement |
| `BaboViolent2/Code/CPing.h` | Server ping measurement |
| `BaboViolent2/Code/CPlayers.cpp` | Players list UI |
| `BaboViolent2/Code/CPlayers.h` | Players list UI |
| `BaboViolent2/Code/CProfile.cpp` | Profile UI tab |
| `BaboViolent2/Code/CProfile.h` | Profile UI tab |
| `BaboViolent2/Code/CRain.cpp` | Rain weather effect |
| `BaboViolent2/Code/CRain.h` | Rain weather effect |
| `BaboViolent2/Code/CRegisterClan.cpp` | Clan registration UI |
| `BaboViolent2/Code/CRegisterClan.h` | Clan registration UI |
| `BaboViolent2/Code/CSnow.cpp` | Snow weather effect |
| `BaboViolent2/Code/CSnow.h` | Snow weather effect |
| `BaboViolent2/Code/CStats.cpp` | Player statistics UI |
| `BaboViolent2/Code/CStats.h` | Player statistics UI |
| `BaboViolent2/Code/CStatus.cpp` | Online status reporting |
| `BaboViolent2/Code/CStatus.h` | Online status reporting |
| `BaboViolent2/Code/CString.cpp` | Engine copy of the shared math/string type |
| `BaboViolent2/Code/CString.h` | Engine copy of the shared math/string type |
| `BaboViolent2/Code/CSurvey.cpp` | Survey dialog |
| `BaboViolent2/Code/CSurvey.h` | Survey dialog |
| `BaboViolent2/Code/CThread.cpp` | Thread wrapper |
| `BaboViolent2/Code/CThread.h` | Thread wrapper |
| `BaboViolent2/Code/CUserLogin.cpp` | Account login UI |
| `BaboViolent2/Code/CUserLogin.h` | Account login UI |
| `BaboViolent2/Code/CVector.cpp` | Engine copy of the shared math/string type |
| `BaboViolent2/Code/CVector.h` | Engine copy of the shared math/string type |
| `BaboViolent2/Code/CVertexBuffer.cpp` | Vertex buffer wrapper |
| `BaboViolent2/Code/CVertexBuffer.h` | Vertex buffer wrapper |
| `BaboViolent2/Code/CWeather.cpp` | Weather selection and rendering |
| `BaboViolent2/Code/CWeather.h` | Weather selection and rendering |
| `BaboViolent2/Code/Changes Log.txt` | Upstream change log |
| `BaboViolent2/Code/Choice.cpp` | Menu choice (drop-down/spinner) widget |
| `BaboViolent2/Code/Choice.h` | Menu choice (drop-down/spinner) widget |
| `BaboViolent2/Code/Client.cpp` | Client: connection, join handshake, send/receive state |
| `BaboViolent2/Code/Client.h` | Client: connection, join handshake, send/receive state |
| `BaboViolent2/Code/ClientRecv.cpp` | Client: handlers for every server-to-client packet |
| `BaboViolent2/Code/ClientRender.cpp` | Client: in-game HUD and view rendering |
| `BaboViolent2/Code/ConfirmPass.cpp` | Password confirmation dialog |
| `BaboViolent2/Code/ConfirmPass.h` | Password confirmation dialog |
| `BaboViolent2/Code/ConnectFailed.cpp` | Dialog: connection failed |
| `BaboViolent2/Code/ConnectFailed.h` | Dialog: connection failed |
| `BaboViolent2/Code/Console.cpp` | In-game console: commands, log, remote console |
| `BaboViolent2/Code/Console.h` | In-game console: commands, log, remote console |
| `BaboViolent2/Code/Control.cpp` | Menu control base class |
| `BaboViolent2/Code/Control.h` | Menu control base class |
| `BaboViolent2/Code/ControlListener.cpp` | Menu control listener interface |
| `BaboViolent2/Code/ControlListener.h` | Menu control listener interface |
| `BaboViolent2/Code/CreateGame.cpp` | Create-game screen |
| `BaboViolent2/Code/CreateGame.h` | Create-game screen |
| `BaboViolent2/Code/Credits.cpp` | Credits screen |
| `BaboViolent2/Code/Credits.h` | Credits screen |
| `BaboViolent2/Code/Dialog.cpp` | Menu dialog base |
| `BaboViolent2/Code/Dialog.h` | Menu dialog base |
| `BaboViolent2/Code/Editor.cpp` | Map editor core |
| `BaboViolent2/Code/Editor.h` | Map editor core |
| `BaboViolent2/Code/EditorDialogs.cpp` | Map editor dialogs |
| `BaboViolent2/Code/EditorDialogs.h` | Map editor dialogs |
| `BaboViolent2/Code/EditorTools.cpp` | Map editor tools |
| `BaboViolent2/Code/EditorTools.h` | Map editor tools |
| `BaboViolent2/Code/Extended.cpp` | Skin and color chooser screen |
| `BaboViolent2/Code/Extended.h` | Skin and color chooser screen |
| `BaboViolent2/Code/FastDelegate.h` | Third-party fast C++ delegate header |
| `BaboViolent2/Code/FileIO.cpp` | File and memory-buffer reader/writer with fixed-width types |
| `BaboViolent2/Code/FileIO.h` | File and memory-buffer reader/writer with fixed-width types |
| `BaboViolent2/Code/Game.cpp` | Game state: players, projectiles, items, flags, round and mode logic (shared by client and server) |
| `BaboViolent2/Code/Game.h` | Game state: players, projectiles, items, flags, round and mode logic (shared by client and server) |
| `BaboViolent2/Code/GameProjectile.cpp` | Game: projectile and explosion simulation |
| `BaboViolent2/Code/GameRender.cpp` | Game: world rendering |
| `BaboViolent2/Code/GameShowStats.cpp` | Game: scoreboard and end-of-round stats drawing |
| `BaboViolent2/Code/GameSpawn.cpp` | Game: spawn point selection and respawn |
| `BaboViolent2/Code/GameVar.cpp` | All client and server variables (cl_*, sv_*, r_*, s_*), registered through dksvar, plus loaded textures and sounds |
| `BaboViolent2/Code/GameVar.h` | All client and server variables (cl_*, sv_*, r_*, s_*), registered through dksvar, plus loaded textures and sounds |
| `BaboViolent2/Code/Helper.cpp` | Misc helpers (colors, text, math) |
| `BaboViolent2/Code/Helper.h` | Misc helpers (colors, text, math) |
| `BaboViolent2/Code/Host.h` | Host-game settings struct |
| `BaboViolent2/Code/IncorrectName.cpp` | Dialog: invalid player name |
| `BaboViolent2/Code/IncorrectName.h` | Dialog: invalid player name |
| `BaboViolent2/Code/IncorrectPassword.cpp` | Dialog: wrong server password |
| `BaboViolent2/Code/IncorrectPassword.h` | Dialog: wrong server password |
| `BaboViolent2/Code/IntroScreen.cpp` | Intro screen |
| `BaboViolent2/Code/IntroScreen.h` | Intro screen |
| `BaboViolent2/Code/JoinGame.cpp` | Join-game screen |
| `BaboViolent2/Code/JoinGame.h` | Join-game screen |
| `BaboViolent2/Code/Key.cpp` | Key binding entry |
| `BaboViolent2/Code/Key.h` | Key binding entry |
| `BaboViolent2/Code/KeyManager.cpp` | Key bindings manager |
| `BaboViolent2/Code/KeyManager.h` | Key bindings manager |
| `BaboViolent2/Code/Label.cpp` | Menu label widget |
| `BaboViolent2/Code/Label.h` | Menu label widget |
| `BaboViolent2/Code/MainMenu.cpp` | Main menu screen |
| `BaboViolent2/Code/MainMenu.h` | Main menu screen |
| `BaboViolent2/Code/Map.cpp` | Map data, .bvm loading and saving, collision, rendering resources |
| `BaboViolent2/Code/Map.h` | Map data, .bvm loading and saving, collision, rendering resources |
| `BaboViolent2/Code/MapRender.cpp` | Map rendering (tiles, walls, dirt, optional 3D model map) |
| `BaboViolent2/Code/MemIO.cpp` | In-memory buffer reader/writer |
| `BaboViolent2/Code/MemIO.h` | In-memory buffer reader/writer |
| `BaboViolent2/Code/Menu.cpp` | Menu system core (old menu) |
| `BaboViolent2/Code/Menu.h` | Menu system core (old menu) |
| `BaboViolent2/Code/MenuSetup.cpp` | Menu construction |
| `BaboViolent2/Code/MessageDialog.cpp` | Generic message dialog |
| `BaboViolent2/Code/MessageDialog.h` | Generic message dialog |
| `BaboViolent2/Code/Minibot.cpp` | Minibot drone weapon entity |
| `BaboViolent2/Code/NoGameRunning.cpp` | Dialog: no game running |
| `BaboViolent2/Code/NoGameRunning.h` | Dialog: no game running |
| `BaboViolent2/Code/NoMapSelected.cpp` | Dialog: no map selected |
| `BaboViolent2/Code/NoMapSelected.h` | Dialog: no map selected |
| `BaboViolent2/Code/OptionMenu.cpp` | Options screen |
| `BaboViolent2/Code/OptionMenu.h` | Options screen |
| `BaboViolent2/Code/Password.cpp` | Password prompt dialog |
| `BaboViolent2/Code/Password.h` | Password prompt dialog |
| `BaboViolent2/Code/Player.cpp` | Player entity: movement, weapons, hits, rendering |
| `BaboViolent2/Code/Player.h` | Player entity: movement, weapons, hits, rendering |
| `BaboViolent2/Code/PlayerUpdate.cpp` | Player: per-tick update (physics, firing, timers) |
| `BaboViolent2/Code/Quit.cpp` | Quit confirmation dialog |
| `BaboViolent2/Code/Quit.h` | Quit confirmation dialog |
| `BaboViolent2/Code/RemoteAdminPackets.h` | Wire structs for the remote-admin protocol |
| `BaboViolent2/Code/ReportGen.cpp` | XML server report generation |
| `BaboViolent2/Code/ReportGen.h` | XML server report generation |
| `BaboViolent2/Code/ResIco.rc` | Windows resource script (icon) |
| `BaboViolent2/Code/Scene.cpp` | Top-level scene: main loop, menus, game and editor switching, rendering setup |
| `BaboViolent2/Code/Scene.h` | Top-level scene: main loop, menus, game and editor switching, rendering setup |
| `BaboViolent2/Code/SceneNet.cpp` | Scene: starts and stops client and server, hosting and joining |
| `BaboViolent2/Code/Server.cpp` | Authoritative server: connections, joins, tick, map and mode control |
| `BaboViolent2/Code/Server.h` | Authoritative server: connections, joins, tick, map and mode control |
| `BaboViolent2/Code/ServerCTF.cpp` | Server: capture-the-flag rules |
| `BaboViolent2/Code/ServerClose.cpp` | Dialog shown when the server closes the connection |
| `BaboViolent2/Code/ServerClose.h` | Dialog shown when the server closes the connection |
| `BaboViolent2/Code/ServerRecv.cpp` | Server: handlers for every client-to-server packet |
| `BaboViolent2/Code/ServerSnD.cpp` | Server: search-and-destroy rules |
| `BaboViolent2/Code/Weapon.cpp` | Weapon definitions and stats |
| `BaboViolent2/Code/Weapon.h` | Weapon definitions and stats |
| `BaboViolent2/Code/Write.cpp` | Chat input widget |
| `BaboViolent2/Code/Write.h` | Chat input widget |
| `BaboViolent2/Code/Writting.cpp` | Chat input handling |
| `BaboViolent2/Code/Writting.h` | Chat input handling |
| `BaboViolent2/Code/WrongVersion.cpp` | Dialog: client/server version mismatch |
| `BaboViolent2/Code/WrongVersion.h` | Dialog: client/server version mismatch |
| `BaboViolent2/Code/Zeven.h` | Umbrella include for engine headers and common types |
| `BaboViolent2/Code/bv2.bmp` | Windows resource bitmap (original art; replace in §H) |
| `BaboViolent2/Code/glext.h` | Vendored OpenGL extension header |
| `BaboViolent2/Code/icon1.ico` | Windows application icon (original art; replace in §H) |
| `BaboViolent2/Code/main.cpp` | Program entry: window, engine init, main loop |
| `BaboViolent2/Code/netPacket.h` | Wire structs and message IDs (raw memcpy'd structs) |
| `BaboViolent2/Code/resource3.h` | Resource IDs for ResIco.rc |
| `BaboViolent2/Code/screengrab.cpp` | Screenshot capture |
| `BaboViolent2/Code/screengrab.h` | Screenshot capture |
| `BaboViolent2/Code/tinyxml.cpp` | TinyXML (third-party XML parser) |
| `BaboViolent2/Code/tinyxml.h` | TinyXML (third-party XML parser) |
| `BaboViolent2/Code/tinyxmlerror.cpp` | TinyXML (third-party XML parser) |
| `BaboViolent2/Code/tinyxmlparser.cpp` | TinyXML (third-party XML parser) |

### `BaboViolent2/Content`

| path | purpose |
|---|---|
| `BaboViolent2/Content/README.txt` | Where game data goes (originals removed) |

### `BaboViolent2/Content/main/LaunchScript`

| path | purpose |
|---|---|
| `BaboViolent2/Content/main/LaunchScript/CTF.cfg` | Example dedicated-server launch script (CTF) |

### `BaboViolent2/Content/main/languages`

| path | purpose |
|---|---|
| `BaboViolent2/Content/main/languages/en.lang` | English strings (default and only language) |

### `BaboViolent2/inc`

| path | purpose |
|---|---|
| `BaboViolent2/inc/LinuxHeader.h` | Linux/POSIX compatibility header |
| `BaboViolent2/inc/baboNet.h` | Copy of the babonet public header |
| `BaboViolent2/inc/cMSstruct.h` | Master-server protocol structs and message IDs |
| `BaboViolent2/inc/dkc.h` | dkc: high-resolution timer and platform helpers |
| `BaboViolent2/inc/dkf.h` | dkf: bitmap font rendering |
| `BaboViolent2/inc/dkgl.h` | dkgl: OpenGL context and state |
| `BaboViolent2/inc/dki.h` | dki: keyboard, mouse and input polling |
| `BaboViolent2/inc/dko.h` | dko: public API of the .DKO model loader |
| `BaboViolent2/inc/dkp.h` | dkp: particle system |
| `BaboViolent2/inc/dks.h` | dks: sound and music (FMOD) |
| `BaboViolent2/inc/dksvar.h` | dksvar: configuration variable registry |
| `BaboViolent2/inc/dksvardef.h` | dksvar: variable definition macros |
| `BaboViolent2/inc/dkt.h` | dkt: texture loading (TGA) and binding |
| `BaboViolent2/inc/dkw.h` | dkw: window creation and events |
| `BaboViolent2/inc/platform_types.h` | Fixed-width type aliases |

### `Engine/DukZeven/Code`

| path | purpose |
|---|---|
| `Engine/DukZeven/Code/CDkoObject.cpp` | DKO model instance wrapper |
| `Engine/DukZeven/Code/CDkoObject.h` | DKO model instance wrapper |
| `Engine/DukZeven/Code/CFont.cpp` | Bitmap font class |
| `Engine/DukZeven/Code/CFont.h` | Bitmap font class |
| `Engine/DukZeven/Code/CMatrix.cpp` | Engine copy of the shared math/string type |
| `Engine/DukZeven/Code/CMatrix.h` | Engine copy of the shared math/string type |
| `Engine/DukZeven/Code/CParticle.cpp` | Particle class |
| `Engine/DukZeven/Code/CParticle.h` | Particle class |
| `Engine/DukZeven/Code/CString.cpp` | Engine copy of the shared math/string type |
| `Engine/DukZeven/Code/CString.h` | Engine copy of the shared math/string type |
| `Engine/DukZeven/Code/CSystemVariable.cpp` | Typed configuration variable classes |
| `Engine/DukZeven/Code/CSystemVariable.h` | Typed configuration variable classes |
| `Engine/DukZeven/Code/CVector.cpp` | Engine copy of the shared math/string type |
| `Engine/DukZeven/Code/CVector.h` | Engine copy of the shared math/string type |
| `Engine/DukZeven/Code/Makefile` | Makefile for the DukZeven modules |
| `Engine/DukZeven/Code/dkc.cpp` | dkc: high-resolution timer and platform helpers |
| `Engine/DukZeven/Code/dkc.h` | dkc: high-resolution timer and platform helpers |
| `Engine/DukZeven/Code/dkci.h` | Internal header of the matching dk module |
| `Engine/DukZeven/Code/dkf.cpp` | dkf: bitmap font rendering |
| `Engine/DukZeven/Code/dkf.h` | dkf: bitmap font rendering |
| `Engine/DukZeven/Code/dkfi.h` | Internal header of the matching dk module |
| `Engine/DukZeven/Code/dkgl.cpp` | dkgl: OpenGL context and state |
| `Engine/DukZeven/Code/dkgl.h` | dkgl: OpenGL context and state |
| `Engine/DukZeven/Code/dkgli.h` | Internal header of the matching dk module |
| `Engine/DukZeven/Code/dki.cpp` | dki: keyboard, mouse and input polling |
| `Engine/DukZeven/Code/dki.h` | dki: keyboard, mouse and input polling |
| `Engine/DukZeven/Code/dkii.h` | Internal header of the matching dk module |
| `Engine/DukZeven/Code/dko.h` | dko: public API of the .DKO model loader |
| `Engine/DukZeven/Code/dkp.cpp` | dkp: particle system |
| `Engine/DukZeven/Code/dkp.h` | dkp: particle system |
| `Engine/DukZeven/Code/dkpi.h` | Internal header of the matching dk module |
| `Engine/DukZeven/Code/dks.cpp` | dks: sound and music (FMOD) |
| `Engine/DukZeven/Code/dks.h` | dks: sound and music (FMOD) |
| `Engine/DukZeven/Code/dksi.h` | Internal header of the matching dk module |
| `Engine/DukZeven/Code/dksvar.cpp` | dksvar: configuration variable registry |
| `Engine/DukZeven/Code/dksvar.h` | dksvar: configuration variable registry |
| `Engine/DukZeven/Code/dksvardef.h` | dksvar: variable definition macros |
| `Engine/DukZeven/Code/dksvari.h` | Internal header of the matching dk module |
| `Engine/DukZeven/Code/dkt.cpp` | dkt: texture loading (TGA) and binding |
| `Engine/DukZeven/Code/dkt.h` | dkt: texture loading (TGA) and binding |
| `Engine/DukZeven/Code/dkti.h` | Internal header of the matching dk module |
| `Engine/DukZeven/Code/dkw.cpp` | dkw: window creation and events |
| `Engine/DukZeven/Code/dkw.h` | dkw: window creation and events |
| `Engine/DukZeven/Code/dkwi.h` | Internal header of the matching dk module |
| `Engine/DukZeven/Code/glext.h` | Vendored OpenGL extension header |

### `Engine/DukZeven/inc`

| path | purpose |
|---|---|
| `Engine/DukZeven/inc/dkc.h` | dkc: high-resolution timer and platform helpers |
| `Engine/DukZeven/inc/dkf.h` | dkf: bitmap font rendering |
| `Engine/DukZeven/inc/dkgl.h` | dkgl: OpenGL context and state |
| `Engine/DukZeven/inc/dki.h` | dki: keyboard, mouse and input polling |
| `Engine/DukZeven/inc/dko.h` | dko: public API of the .DKO model loader |
| `Engine/DukZeven/inc/dkp.h` | dkp: particle system |
| `Engine/DukZeven/inc/dks.h` | dks: sound and music (FMOD) |
| `Engine/DukZeven/inc/dksi.h` | Internal header of the matching dk module |
| `Engine/DukZeven/inc/dksvar.h` | dksvar: configuration variable registry |
| `Engine/DukZeven/inc/dksvardef.h` | dksvar: variable definition macros |
| `Engine/DukZeven/inc/dkt.h` | dkt: texture loading (TGA) and binding |
| `Engine/DukZeven/inc/dkw.h` | dkw: window creation and events |
| `Engine/DukZeven/inc/glext.h` | Vendored OpenGL extension header |
| `Engine/DukZeven/inc/linux_types.h` | Fixed-width type aliases for Linux |

### `Engine/babonet/Code`

| path | purpose |
|---|---|
| `Engine/babonet/Code/MD5.h` | RFC 1321 MD5 header |
| `Engine/babonet/Code/Makefile` | Makefile for babonet |
| `Engine/babonet/Code/baboNet.cpp` | babonet public API (bb_* functions) |
| `Engine/babonet/Code/baboNet.h` | babonet public API (bb_* functions) |
| `Engine/babonet/Code/cClient.cpp` | babonet client connection (TCP and UDP) |
| `Engine/babonet/Code/cClient.h` | babonet client connection (TCP and UDP) |
| `Engine/babonet/Code/cConnection.cpp` | babonet outgoing connection state machine |
| `Engine/babonet/Code/cConnection.h` | babonet outgoing connection state machine |
| `Engine/babonet/Code/cDNSquery.cpp` | babonet asynchronous DNS lookup |
| `Engine/babonet/Code/cDNSquery.h` | babonet asynchronous DNS lookup |
| `Engine/babonet/Code/cIncConnection.cpp` | babonet incoming connection handshake |
| `Engine/babonet/Code/cIncConnection.h` | babonet incoming connection handshake |
| `Engine/babonet/Code/cPacket.cpp` | babonet TCP packet |
| `Engine/babonet/Code/cPacket.h` | babonet TCP packet |
| `Engine/babonet/Code/cPeer.cpp` | babonet UDP peer with reliability acks |
| `Engine/babonet/Code/cPeer.h` | babonet UDP peer with reliability acks |
| `Engine/babonet/Code/cPeer2Peer.cpp` | babonet UDP peer-to-peer manager |
| `Engine/babonet/Code/cPeer2Peer.h` | babonet UDP peer-to-peer manager |
| `Engine/babonet/Code/cServer.cpp` | babonet server: listener and client set |
| `Engine/babonet/Code/cServer.h` | babonet server: listener and client set |
| `Engine/babonet/Code/cUDPpacket.cpp` | babonet UDP packet |
| `Engine/babonet/Code/cUDPpacket.h` | babonet UDP packet |
| `Engine/babonet/Code/cUDPserver.cpp` | babonet UDP server |
| `Engine/babonet/Code/cUDPserver.h` | babonet UDP server |
| `Engine/babonet/Code/global.h` | babonet shared types and constants |
| `Engine/babonet/Code/main.cpp` | Standalone CMD5 example program (not part of any build) |
| `Engine/babonet/Code/md5c.c` | RFC 1321 MD5 implementation |
| `Engine/babonet/Code/md5class.cpp` | CMD5: MD5 hex-digest wrapper |
| `Engine/babonet/Code/md5class.h` | CMD5: MD5 hex-digest wrapper |

### `Engine/babonet/inc`

| path | purpose |
|---|---|
| `Engine/babonet/inc/baboNet.h` | babonet public API (bb_* functions) |

### `Engine/dko/Code`

| path | purpose |
|---|---|
| `Engine/dko/Code/CFace.cpp` | dko: geometry and collision octree |
| `Engine/dko/Code/CFace.h` | dko: geometry and collision octree |
| `Engine/dko/Code/CHUNKINF.H` | Autodesk 3DS chunk definitions (3DS import) |
| `Engine/dko/Code/COctree.cpp` | dko: geometry and collision octree |
| `Engine/dko/Code/COctree.h` | dko: geometry and collision octree |
| `Engine/dko/Code/CVector.cpp` | Engine copy of the shared math/string type |
| `Engine/dko/Code/CVector.h` | Engine copy of the shared math/string type |
| `Engine/dko/Code/CdkoAnimation.cpp` | dko: model data structures |
| `Engine/dko/Code/CdkoAnimation.h` | dko: model data structures |
| `Engine/dko/Code/CdkoMaterial.cpp` | dko: model data structures |
| `Engine/dko/Code/CdkoMaterial.h` | dko: model data structures |
| `Engine/dko/Code/CdkoMesh.cpp` | dko: model data structures |
| `Engine/dko/Code/CdkoMesh.h` | dko: model data structures |
| `Engine/dko/Code/CdkoModel.cpp` | dko: model data structures |
| `Engine/dko/Code/CdkoModel.h` | dko: model data structures |
| `Engine/dko/Code/DKO Chunk Info.txt` | DKO file chunk layout |
| `Engine/dko/Code/Makefile` | Makefile for dko |
| `Engine/dko/Code/Readme.txt` | Notes on the DKO exporter and format |
| `Engine/dko/Code/dko.cpp` | dko: public API of the .DKO model loader |
| `Engine/dko/Code/dko.h` | dko: public API of the .DKO model loader |
| `Engine/dko/Code/dkoInner.h` | dko: internal declarations |
| `Engine/dko/Code/eHierarchic.cpp` | dko: hierarchy and texture helpers |
| `Engine/dko/Code/eHierarchic.h` | dko: hierarchy and texture helpers |
| `Engine/dko/Code/ePTexture.cpp` | dko: hierarchy and texture helpers |
| `Engine/dko/Code/ePTexture.h` | dko: hierarchy and texture helpers |
| `Engine/dko/Code/platform_types.h` | Fixed-width type aliases |

### `Engine/dko/Code/gl`

| path | purpose |
|---|---|
| `Engine/dko/Code/gl/glext.h` | Vendored OpenGL extension header |

### `Engine/dko/inc`

| path | purpose |
|---|---|
| `Engine/dko/inc/dko.h` | dko: public API of the .DKO model loader |
| `Engine/dko/inc/dkt.h` | dkt: texture loading (TGA) and binding |

### `MasterServer`

| path | purpose |
|---|---|
| `MasterServer/Readme.md` | Master server notes |

### `MasterServer/Source`

| path | purpose |
|---|---|
| `MasterServer/Source/AUTHORS` | Autotools boilerplate |
| `MasterServer/Source/COPYING` | License text shipped with the master server |
| `MasterServer/Source/ChangeLog` | Autotools boilerplate |
| `MasterServer/Source/Doxyfile` | Doxygen config |
| `MasterServer/Source/INSTALL` | Autotools boilerplate |
| `MasterServer/Source/Makefile.am` | Automake source list (replaced in Step 2) |
| `MasterServer/Source/NEWS` | Autotools boilerplate |
| `MasterServer/Source/TODO` | Autotools boilerplate |
| `MasterServer/Source/config.guess` | Autotools helper |
| `MasterServer/Source/config.h.in` | Autotools config header template |
| `MasterServer/Source/config.sub` | Autotools helper |
| `MasterServer/Source/configure.in` | Autoconf script (replaced in Step 2) |
| `MasterServer/Source/depcomp` | Autotools helper |
| `MasterServer/Source/install-sh` | Autotools helper |
| `MasterServer/Source/ltmain.sh` | Libtool helper |
| `MasterServer/Source/missing` | Autotools helper |
| `MasterServer/Source/mkinstalldirs` | Autotools helper |

### `MasterServer/Source/src`

| path | purpose |
|---|---|
| `MasterServer/Source/src/CClient.cpp` | Master: connected-client record (babonet client variant) |
| `MasterServer/Source/src/CClient.h` | Master: connected-client record (babonet client variant) |
| `MasterServer/Source/src/CServer.cpp` | Master: registered game-server record |
| `MasterServer/Source/src/Makefile.am` | Automake source list for linuxmaster (replaced in Step 2) |
| `MasterServer/Source/src/cBV2game.cpp` | Master: game list entry |
| `MasterServer/Source/src/cBV2game.h` | Master: game list entry |
| `MasterServer/Source/src/cMSstruct.h` | Master protocol structs (older subset of BaboViolent2/inc/cMSstruct.h) |
| `MasterServer/Source/src/cMasterServer.cpp` | Master: server registry, bans, DB access |
| `MasterServer/Source/src/cMasterServer.h` | Master: server registry, bans, DB access |
| `MasterServer/Source/src/cNetManager.cpp` | Master: network loop and packet handling |
| `MasterServer/Source/src/cNetManager.h` | Master: network loop and packet handling |
| `MasterServer/Source/src/cPlayer.cpp` | Master: player record |
| `MasterServer/Source/src/cPlayer.h` | Master: player record |
| `MasterServer/Source/src/cServer.h` | Master: registered game-server record |
| `MasterServer/Source/src/main.cpp` | Master server entry point |

### `config`

| path | purpose |
|---|---|
| `config/README.md` | Config and secrets policy |
| `config/bv2.example.cfg` | Default client/server config with empty secrets |

### `content-seed`

| path | purpose |
|---|---|
| `content-seed/bv2.sql` | Client SQLite schema and defaults (empty MasterServers) |
| `content-seed/master.sql` | Master server SQLite schema (recovered from code) |
| `content-seed/web.sql` | Master server web game-list schema |

### `docs`

| path | purpose |
|---|---|
| `docs/ASSETS-LICENSE.md` | Asset and name licensing status, replacement register |

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

### `docs/assets`

| path | purpose |
|---|---|
| `docs/assets/ASSET-INVENTORY.md` | Catalogue of every removed original asset |
| `docs/assets/original-assets.sha256` | SHA-256 of every removed original file |

### `docs/decisions`

| path | purpose |
|---|---|
| `docs/decisions/0001-ninja-generator-on-all-presets.md` | ADR 0001 |
| `docs/decisions/0002-compile-out-libcurl.md` | ADR 0002 |
| `docs/decisions/0003-keep-opengl-2.1-then-sdl-gpu.md` | ADR 0003 |
| `docs/decisions/0004-project-name-roboviolence2.md` | ADR 0004 |
| `docs/decisions/README.md` | ADR format and index |

### `docs/legal`

| path | purpose |
|---|---|
| `docs/legal/permission-request.md` | Draft permission request to the rights holders |

### `docs/refactoring`

| path | purpose |
|---|---|
| `docs/refactoring/README.md` | Phase A handoff and step index |
| `docs/refactoring/_template-step.md` | Template for step files |
| `docs/refactoring/future-phases.md` | Work after Phase A (sections A to H) |
| `docs/refactoring/step0-fork-rename-and-asset-removal.md` | Scope file for step0 |
| `docs/refactoring/step1-baseline-and-legacy-removal.md` | Scope file for step1 |
| `docs/refactoring/step2-cmake-build.md` | Scope file for step2 |
| `docs/refactoring/step3-dependency-upgrades.md` | Scope file for step3 |
| `docs/refactoring/step4-64bit-and-cross-platform.md` | Scope file for step4 |
| `docs/refactoring/step5-code-hygiene-and-ci.md` | Scope file for step5 |

### `tools`

| path | purpose |
|---|---|
| `tools/FORK_BASE` | First commit after upstream; secret scans start here |
| `tools/asset-inventory.py` | Generates the asset inventory and hash list (needs the originals) |
| `tools/check-architecture.ps1` | Fails if ARCHITECTURE.md and the tracked files disagree (Windows) |
| `tools/check-architecture.sh` | Fails if ARCHITECTURE.md and the tracked files disagree (Unix) |
| `tools/check-encoding.py` | Fails on non-UTF-8, BOM or CR in tracked text files |
| `tools/check-original-assets.py` | Fails if a tracked file matches an original-asset hash |
| `tools/convert-encoding.py` | One-off UTF-8/LF converter used in step 1.4 |
| `tools/setup-dev.ps1` | Activates hooks, checks tools (Windows) |
| `tools/setup-dev.sh` | Activates hooks, checks tools (Unix) |
