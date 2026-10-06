# Step 4: 64-bit, cross-platform correctness and local config

**Status:** DONE (2026-10-06)

**Depends on:** [Step 2](step2-cmake-build.md); can overlap [Step 3](step3-dependency-upgrades.md), but coordinate engine file edits with it. **Index:** [README.md](README.md)

## Scope

| Field | Value |
|---|---|
| Goal | x64 and arm64 builds on Windows 11, macOS and Linux are free of truncation, signedness and format warnings; packets have fixed layouts; the game finds its content and keeps user config and secrets outside the install and repo |
| In scope | Tasks 4.1–4.6 below |
| Out of scope | Packet validation, player-slot binding and other security fixes ([Phase B](../phase-b-security-infrastructure-anti-cheat/README.md) steps 4–7), even where the same lines are touched; UTF-8 conversion (Step 5); raising warnings to `-Wextra` (Step 5) |
| Allowed paths | `engine/**`, `game/src/**`, `masterserver/src/**`, `config/**`, `CMakeLists.txt` files, `ARCHITECTURE.md`, `AGENTS.md` |
| Inputs | `AGENTS.md`, `ARCHITECTURE.md`, [../../analysis/02_DATA_STRUCTURES.md](../../analysis/02_DATA_STRUCTURES.md), [../../analysis/KEY_QUESTIONS.md](../../analysis/KEY_QUESTIONS.md) Q-S2, Q12 |
| Deliverables | Fixed-width `netPacket.h` with size asserts; one `GAME_VERSION` bump; portable paths; the layered config loader |
| Definition of done | "Acceptance checks" below pass on all three OSes |

## Context

The Windows client was only ever built as 32-bit, with all warnings off (`-w`). The Linux server was 64-bit but never had its warnings reviewed. macOS was never a target.

The targets are now x64 (MSVC, LLP64; GCC/Clang, LP64) and arm64 (macOS, **unsigned `char`**; optional Windows and Linux arm64). Network messages are raw structs `memcpy`'d on the wire (`game/src/netPacket.h`), so type sizes and signedness are protocol concerns. Protocol compatibility isn't required.

## Tasks

### 4.1 Warnings on and triage

- Remove `-w`. Use `-Wall` / `/W3`.
- Fix these categories first:
  - pointer ↔ `int`/`long` casts (babonet, the `CControl` UI callbacks);
  - `long` assumed to be 32-bit (64-bit on Linux and macOS, 32-bit on Windows);
  - `char` signedness: `playerID` is a `char` (`netPacket.h:40`). Use `int8_t` / `uint8_t` explicitly;
  - `sizeof(long)` or `sizeof(void*)` inside file formats (`.bvm` loader `Map.cpp`, `.DKO` loader, the `dkt.cpp` TGA reader);
  - `-Wformat` mismatches in `sprintf` / `printf` (90 raw string calls in game code).
- List the remaining warnings for Step 5.

### 4.2 Wire and file formats

- `netPacket.h`:
  - use fixed-width types;
  - wrap the structs in `#pragma pack(push, 1)`;
  - add a `static_assert(sizeof(net_xxx) == N)` per struct.
- Keep the quantisation (positions `int16_t` ×100, velocities `int8_t` ×10).
- `cMSstruct.h` (master ↔ game server): apply the same treatment.
- Bump `GAME_VERSION_SV/CL` (`Server.h`, `Client.h`) **once** for Phase A.
- `.bvm` and `.DKO`: read fields explicitly with fixed-width reads, not by `fread` into structs. Every original map (local data, step 0 §0.5) and every placeholder map must still load.
- Add a `static_assert` that the target is little-endian.

### 4.3 Win32-only APIs outside the engine

- Find them with `grep -rn "GetTickCount\|timeGetTime\|_snprintf\|stricmp\|strnicmp\|MessageBox\|RegOpenKey\|ShellExecute\|_mkdir\|Sleep(" game/src masterserver/src`.
- Replace them with `std::chrono`, `std::filesystem`, `std::this_thread::sleep_for`, a `strcasecmp` shim in `platform.h`, `SDL_ShowSimpleMessageBox` and `SDL_OpenURL`.

### 4.4 Paths and content location

- Use `/` separators everywhere.
- **Linux is case-sensitive.** Normalise every content reference in code and `.cfg` to the real file names under `content/`. Add a one-off script `tools/check-content-case.*` that greps the literal paths in `game/src` and checks each one exists with exact case.
- Content root search order:
  1. `BV2_DATA_DIR` env var;
  2. macOS `SDL_GetBasePath()` (`.app/Contents/Resources/`);
  3. `<exe>/../share/bv2/` (Linux and Windows install);
  4. next to the executable;
  5. the working directory.
- Linux packaging (AppImage, Flatpak, `.deb`) is a future phase. Make sure the search order already supports FHS installs.

### 4.5 User data, local config and secrets (implements the step 1 §1.2 policy)

- The user-writable dir is `SDL_GetPrefPath("BaboViolent2", "bv2")`, which resolves to:
  - `%APPDATA%` on Windows,
  - `~/Library/Application Support` on macOS,
  - `$XDG_DATA_HOME` on Linux.
- Saved `bv2.cfg`, downloaded maps, screenshots, logs and the generated `bv2.db` go there. Nothing is written into the install dir, the bundle or the repo.
- Config load order, later entries overriding earlier ones:
  1. `main/bv2.example.cfg` (defaults from the repo);
  2. pref-path `bv2.cfg` (created on first run from the example);
  3. `config/local/*.cfg` or `--config <file>`, for servers;
  4. environment: `BV2_MASTER_SERVERS`, `BV2_ACCOUNT_URL`, `BV2_SV_PASSWORD`, `BV2_ADMIN_PASS`.
- The master server list and `AccountURL` now come from this config. The seeded `bv2.db` no longer supplies them (it's empty since step 1). Endpoints that aren't configured disable the related feature gracefully (no master list, no account login).
- Dedicated server: `bv2dedicated --config /etc/bv2/server.cfg` or env vars, so production secrets come from the deployment's secret store.
- Never log secrets: mask `*pass*` variables in console echo and `dksvar` dumps.

### 4.6 Platform specifics

- **Windows 11+:**
  - an application manifest declaring Windows 10/11 `supportedOS` and PerMonitorV2 DPI awareness;
  - UTF-8 active code page (`<activeCodePage>UTF-8</activeCodePage>`) for paths.
- **macOS:**
  - `Info.plist` (high-DPI, minimum 12.0);
  - ad-hoc codesign for local runs;
  - notarisation belongs to a future phase.
- **Linux:**
  - `.desktop` file and icon for installs;
  - set `SDL_HINT_APP_ID` (Wayland app id);
  - test on Mesa (Intel/AMD) and on the NVIDIA proprietary driver if available.

## Critical files

- `game/src/{netPacket.h,Server.h,Client.h,Map.cpp,Map.h,main.cpp,GameVar.cpp,CMaster.cpp}`, `cMSstruct.h`
- `engine/dko/src/*`, `engine/zeven/src/{dkt,dksvar,CSystemVariable}.cpp`, `engine/zeven/include/platform.h`
- `engine/babonet/src/*`, `config/bv2.example.cfg`

## Acceptance checks

- The `win-x64-msvc`, `macos-arm64` and `linux-x64` builds have **zero** warnings in the §4.1 categories, and all `static_assert`s pass on each.
- Every original map (local data) and every placeholder map loads on all three OSes. `tools/check-content-case.*` passes.
- Cross-play between Windows 11, macOS arm64 and Linux x64 clients and servers, in all combinations. Scores, positions and weapon IDs are correct; check `playerID` ≥ 16 and negative velocities.
- With a read-only install dir, the game runs and keeps settings across restarts in the pref path.
- `BV2_MASTER_SERVERS=host:port` makes the master list appear; unset, the game runs without it.
- `git status` after playing shows no new or modified files. `ARCHITECTURE.md` is updated; the secret scan is clean.
