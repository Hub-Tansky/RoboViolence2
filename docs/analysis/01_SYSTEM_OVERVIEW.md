# 01 — System Overview

> Scope: the whole repository (`BaboViolent2/`, `Engine/`, `MasterServer/`).
> Every claim carries a `[VERIFY: path:line]` tag, relative to the repo root.
> Source files are CRLF / ISO-8859-1 (French comments); line numbers below are real file line numbers.

---

## 1. What the project is

BaboViolent 2 is a top-down multiplayer shooter (the "babo" is a ball) released as GPLv3 source by bitHeads inc. in 2012.
[VERIFY: BaboViolent2/Code/main.cpp:2-9]

The repo contains three deliverables:

| Deliverable | Entry point | Built by |
|---|---|---|
| Windows game client (+ listen server, map editor) | `WinMain` | Visual Studio solution (`BaboViolent2.sln`) |
| Linux dedicated server `bv2dedicated` | `main` (under `#ifdef CONSOLE`) | root `Makefile` |
| Master server (server browser / ban list) | `MasterServer/Source/src/main.cpp` | autotools in `MasterServer/Source` |

- Dedicated server entry point: [VERIFY: BaboViolent2/Code/main.cpp:455]
- Windows client entry point: [VERIFY: BaboViolent2/Code/main.cpp:677]
- The root Makefile builds the dedicated server with `-D LINUX64 -D CONSOLE -D _PRO_` from `BaboViolent2/Code/*.cpp`: [VERIFY: Makefile:5]
- README: only the `ProDebug` config has been tested; `make` in the root builds the Linux x64 dedicated server: [VERIFY: README.txt:4-10]

## 2. Compile-time variants (macro magic)

Most behaviour differences come from three preprocessor symbols. Read the code with these in mind.

| Macro | Meaning | Evidence |
|---|---|---|
| `CONSOLE` | Headless dedicated server: no rendering, no client, no editor, no menus | `Scene` only declares `client`/`editor`/menus when `CONSOLE` is not defined [VERIFY: BaboViolent2/Code/Scene.h:55-81] |
| `_PRO_` | "Pro" ruleset/protocol: version 2.11.00 instead of 2.10.00, server-side spread, minibot, hash check | [VERIFY: BaboViolent2/Code/Server.h:30-36] [VERIFY: BaboViolent2/Code/Client.h:22-26] |
| `_DX_` | Unfinished Direct3D renderer | README says DX "might not compile" [VERIFY: README.txt:5] |

The Linux Makefile defines `_PRO_`, so a stock Linux dedicated server speaks the **Pro** protocol (`GAME_VERSION_SV 21100`) and will reject non-Pro clients (`21000`) at the version check.
[VERIFY: Makefile:5] [VERIFY: BaboViolent2/Code/Server.h:33] [VERIFY: BaboViolent2/Code/ClientRecv.cpp:256]

## 3. Module map

```
┌──────────────────────────────────────────────────────────────────────────┐
│                         BaboViolent2/Code (game)                         │
│                                                                          │
│  main.cpp ──► Scene ──┬──► Server ──► Game (isServerGame=true)            │
│                       │      │           ├── Player[32] ── Weapon         │
│                       │      │           ├── Map (cells, spawns, flags)   │
│                       │      │           └── Projectile*                  │
│                       ├──► Client ──► Game (isServerGame=false)           │
│                       ├──► Editor            (client build only)          │
│                       └──► menus / CMainTab  (client build only)          │
│  Console ── dksvar-backed GameVar (gameVar.*)                             │
│  CMaster ── master-server link + remote-admin over P2P UDP                │
└──────────────┬───────────────────────────────────────────────────────────┘
               │ links against
┌──────────────▼───────────────────────────────────────────────────────────┐
│ Engine/babonet   — TCP/UDP + P2P networking (bb_* API)                    │
│ Engine/DukZeven  — dkc timer, dksvar config, dkw window, dkgl GL,         │
│                    dki input, dkt textures, dkf fonts, dkp particles,     │
│                    dks sound (FMOD), CString/CVector/CMatrix              │
│ Engine/dko       — .DKO 3D model loader, ray/sphere intersection          │
└──────────────────────────────────────────────────────────────────────────┘
```

- `Scene` owns one `Server*`, and in client builds one `Client*` and `Editor*`: [VERIFY: BaboViolent2/Code/Scene.h:52-60]
- `Server` and `Client` each hold their own `Game*`: [VERIFY: BaboViolent2/Code/Server.h:61-62] [VERIFY: BaboViolent2/Code/Client.h:60-61]
- A listen server ("host") creates **two** `Game` objects, one for the server and one for the local client, and the client connects to `127.0.0.1`: [VERIFY: BaboViolent2/Code/SceneNet.cpp:36-60]
- The Engine libraries are built as separate static libs by the Makefile (`baboNet`, `dko`, `DukZeven console`): [VERIFY: Makefile:9-16]

### Engine subsystems (DukZeven)

Prefix → responsibility, taken from the shutdown sequence in `WinMain`:
[VERIFY: BaboViolent2/Code/main.cpp:926-935]

| Prefix | Responsibility | Evidence |
|---|---|---|
| `dkc` | Fixed-step clock | [VERIFY: Engine/DukZeven/Code/dkc.cpp:123-178] |
| `dksvar` | Console variables / config file | [VERIFY: Engine/DukZeven/Code/dksvar.cpp:36-46] |
| `dkw` | Window + main loop | [VERIFY: BaboViolent2/Code/main.cpp:897] |
| `dkgl` | OpenGL context | [VERIFY: BaboViolent2/Code/main.cpp:789] |
| `dki` | Input | [VERIFY: BaboViolent2/Code/main.cpp:775] |
| `dkt` / `dko` / `dkp` | Textures / 3D models / particles | [VERIFY: BaboViolent2/Code/main.cpp:813-819] |
| `dks` | Sound (FMOD) | [VERIFY: BaboViolent2/Code/main.cpp:822] |
| `bb_*` | babonet networking | [VERIFY: BaboViolent2/Code/main.cpp:834] |

## 4. Process lifecycle

### 4.1 Dedicated server (`CONSOLE`)

```
main()
 ├─ dksvarInit / LoadConfig("main/bv2.cfg") / SaveConfig      [main.cpp:489-491]
 ├─ dkcInit(30)            → fixed 30 Hz simulation            [main.cpp:495]
 ├─ bb_init(); require bb_getVersion()=="4.0"                  [main.cpp:498-515]
 ├─ new Console / CMaster / Scene                              [main.cpp:518-525]
 ├─ CMainLoopConsole.start()  (thread: timer → console/scene update) [main.cpp:528-531]
 ├─ argv[1] → "execute <file>"                                 [main.cpp:610-615]
 └─ loop: std::cin.getline → lock() → console->sendCommand → unlock() [main.cpp:618-645]
```

- Config load/save: [VERIFY: BaboViolent2/Code/main.cpp:489-491]
- 30 fps fixed step: [VERIFY: BaboViolent2/Code/main.cpp:495]
- babonet version gate "4.0": [VERIFY: BaboViolent2/Code/main.cpp:505-506]
- Simulation thread body: [VERIFY: BaboViolent2/Code/main.cpp:327-387]
- stdin command loop with lock/unlock: [VERIFY: BaboViolent2/Code/main.cpp:618-627]

**Threading model.** Two threads share all game state. Mutual exclusion is a hand-rolled spin handshake on two plain `bool`s (`locked`, `internalLock`) with 1 ms sleeps; there is no mutex or atomic. The game thread only checks `locked` *between* whole update batches, so a console command never runs mid-tick.
[VERIFY: BaboViolent2/Code/main.cpp:360-377] [VERIFY: BaboViolent2/Code/main.cpp:418-451]

> Caveat: the flags are non-atomic, non-volatile `bool`s, so the compiler/CPU is allowed to cache or reorder them. It works in practice on x86 with these sleeps, but it is not a correct synchronisation primitive.

### 4.2 Windows client

```
WinMain()
 ├─ dksvar config, language file check                        [main.cpp:683-692]
 ├─ dkcInit(30)                                               [main.cpp:699]
 ├─ dkwInit (window) → dkiInit → dkglCreateContext → dkt/dko/dkp → dksInit (FMOD) → bb_init
 ├─ new CLobby / Console / CStatus / CMaster / Scene           [main.cpp:862-875]
 ├─ UpdateLauncher()   (copy _Bv2Launcher.exe over Bv2Launcher.exe) [main.cpp:145-193, 883]
 └─ while (dkwMainLoop());  → MainLoopInterface::paint()      [main.cpp:897]
```

`paint()` runs `N` fixed updates (input → console → scene) then one render and `SwapBuffers`:
[VERIFY: BaboViolent2/Code/main.cpp:203-258]

## 5. The fixed timestep

- `dkcInit(30)` sets `perSeconde = 1/30`: [VERIFY: Engine/DukZeven/Code/dkc.cpp:98]
- `dkcGetElapsedf()` returns that **constant**, not the measured frame time: [VERIFY: Engine/DukZeven/Code/dkc.cpp:47-50]
- `dkcUpdateTimer()` accumulates real time and returns how many 1/30 s steps to run: [VERIFY: Engine/DukZeven/Code/dkc.cpp:154-164]

Consequences used everywhere in the game code:

1. Every `update(float delay)` receives `delay = 1/30 s` exactly.
2. "Frames" are a unit of time: **30 frames = 1 second.** The server counts ping in frames and converts with `ping * 33` ms: [VERIFY: BaboViolent2/Code/Server.cpp:1084]
3. There is no cap on the number of catch-up steps per call, so a long stall causes a burst of catch-up updates (a "spiral of death" is possible under sustained overload): [VERIFY: Engine/DukZeven/Code/dkc.cpp:159-164]

## 6. Scene: the top-level state machine

`Scene::update` order (client build):
[VERIFY: BaboViolent2/Code/Scene.cpp:172-317]

1. `master->update()` (master server + P2P + remote admin): [VERIFY: BaboViolent2/Code/Scene.cpp:183]
2. Intro screen / survey gating (client only): [VERIFY: BaboViolent2/Code/Scene.cpp:196-240]
3. `server->update(delay)`; `disconnect()` if `needToShutDown`: [VERIFY: BaboViolent2/Code/Scene.cpp:249-257]
4. `client->update(delay)`; SFX volume ducked by `viewShake`: [VERIFY: BaboViolent2/Code/Scene.cpp:264-284]
5. `editor->update(delay)`: [VERIFY: BaboViolent2/Code/Scene.cpp:292-300]
6. Menus, then particles `dkpUpdate`: [VERIFY: BaboViolent2/Code/Scene.cpp:303-315]

Session transitions (`SceneNet.cpp`):

| Action | What it creates | Evidence |
|---|---|---|
| `host(map)` | `Server(new Game(map))` + `Client(new Game())`, then `client->join("127.0.0.1", sv_port)` | [VERIFY: BaboViolent2/Code/SceneNet.cpp:36-74] |
| `dedicate(map)` | `Server` only | [VERIFY: BaboViolent2/Code/SceneNet.cpp:77-101] |
| `join(ip,port,pw)` | `Client(new Game())` | [VERIFY: BaboViolent2/Code/SceneNet.cpp:105-131] |
| `edit(cmd)` | `Editor(map, font, w, h)` | [VERIFY: BaboViolent2/Code/SceneNet.cpp:135-151] |
| `disconnect()` | deletes client, server, editor; client-only reloads `sv_*` config | [VERIFY: BaboViolent2/Code/SceneNet.cpp:155-179] |

## 7. Authority model (one paragraph)

The server is authoritative for **hits, damage, deaths, spawns, projectiles, flags, rounds**.
The client is authoritative for **its own movement**: it integrates and collides its own babo and sends position + velocity; the server applies only a speed sanity check.
See `03_DATA_FLOW.md` and `ALGORITHM_02` for details.
- Client-side movement integration + collision: [VERIFY: BaboViolent2/Code/PlayerUpdate.cpp:249-284] [VERIFY: BaboViolent2/Code/Game.cpp:630-633]
- Server accepts client coord frames after a velocity/frame-count check: [VERIFY: BaboViolent2/Code/ServerRecv.cpp:807-854]
- Server-side hit resolution: [VERIFY: BaboViolent2/Code/Game.cpp:1221-1287]

## 8. Supporting programs in the repo

| Path | Role | Evidence |
|---|---|---|
| `MasterServer/Source/src` | Stand-alone master server: fixed-step loop around `cNetManager`, SQLite ban list + web "Games" table | [VERIFY: MasterServer/Source/src/main.cpp:36-61] [VERIFY: MasterServer/Source/src/cMasterServer.cpp:165] |
| `MasterServer/compiled_linux_x86.zip` | Prebuilt binary of the official master | [VERIFY: MasterServer/Readme.md:1-5] |
| `BaboViolent2/Bv2Launcher`, `Bv2RemoteAdmin`, `Bv2UpdateServer` | Launcher, wxWidgets remote-admin tool, update server (not analysed in depth) | directory listing |

## 9. Verification checklist

- [x] Entry points located and read (`main.cpp` both variants)
- [x] Macro variants enumerated from `Server.h`, `Client.h`, `Scene.h`, `Makefile`
- [x] Timer semantics read from `dkc.cpp`
- [x] Scene update order read line by line
- [ ] Bv2Launcher / RemoteAdmin / UpdateServer: **not analysed** (out of scope for this pass)
