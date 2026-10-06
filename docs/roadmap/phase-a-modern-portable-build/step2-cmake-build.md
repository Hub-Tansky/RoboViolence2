# Step 2: Target layout and CMake build

**Depends on:** [Step 1](step1-baseline-and-legacy-removal.md). **Next:** [step3-dependency-upgrades.md](step3-dependency-upgrades.md). **Index:** [README.md](README.md)

## Scope

| Field | Value |
|---|---|
| Goal | The code lives in the AI-ready layout, and one CMake project builds `bv2dedicated` and `bv2master` on Windows, macOS and Linux, with the `bv2` client target defined |
| In scope | Tasks 2.0–2.8 below |
| Out of scope | Replacing FMOD, DirectInput, the Win32 window or SDL1 (Step 3); fixing warnings (Step 4); CI (Step 5); splitting the game into client/server/sim libraries ([future-phases.md](../future-phases.md) §C.1) |
| Allowed paths | Everything moved in 2.0; new `CMakeLists.txt` files, `CMakePresets.json`, `vcpkg.json`, `engine/*/README.md`, `game/README.md`, `masterserver/README.md`, `engine/zeven/include/platform.h`; `#include` lines and platform `#if` lines in sources; `AGENTS.md`, `ARCHITECTURE.md`, `README.md` |
| Inputs | `AGENTS.md`, `ARCHITECTURE.md`, [README.md](README.md) "Target repository layout", the root `Makefile`, `BaboViolent2/Code/BaboViolent2.vcxproj` |
| Deliverables | Moved tree; CMake build; per-module READMEs; old build files deleted |
| Definition of done | "Acceptance checks" below all pass |

## Context

Before this step there were three unrelated builds:

- the root `Makefile`, which only builds the Linux dedicated server as `.so` libraries `mv`'d into `Content/`,
- a VS2010 Win32-only solution,
- autotools for the master server.

This step replaces all three with one CMake project. It also moves the code into the target layout **once**, before Steps 3 and 4 edit the files heavily, so later history stays clean.

## Tasks

### 2.0 Move to the target layout (`git mv` only; no content changes in this commit)

| From | To |
|---|---|
| `Engine/babonet/Code/*.cpp,*.c` / `Engine/babonet/inc` + public headers | `engine/babonet/src/` / `engine/babonet/include/` |
| `Engine/DukZeven/Code` / `Engine/DukZeven/inc` | `engine/zeven/src/` / `engine/zeven/include/` |
| `Engine/dko/Code` / `Engine/dko/inc` | `engine/dko/src/` / `engine/dko/include/` |
| `BaboViolent2/Code` | `game/src/` |
| `BaboViolent2/inc/{baboNet.h,dk*.h}` | the owning `engine/*/include/` (dedupe against the existing `inc/` copies) |
| `BaboViolent2/inc/cMSstruct.h` | `engine/babonet/include/` or a shared `protocol/` header; it's shared by game and master |
| `BaboViolent2/Content/main` (languages, LaunchScript; assets were removed in Step 0) | `content/` |
| `MasterServer/Source/src` | `masterserver/src/` |

Then:

- Update the `#include` paths in a **separate** commit.
- Delete the empty `BaboViolent2/`, `Engine/` and `MasterServer/` directories.
- Game code opens files as `main/...` (for example `"main/models/Antena.DKO"`). Keep that working by treating the content root's parent as the data root: either mount `content/` as `main/` in the runtime layout (see 2.6), or search-and-replace the `main/` prefix. Choose one and record it in `docs/decisions/`.

### 2.1 Project skeleton

- A top-level `CMakeLists.txt` (CMake ≥ 3.25, `CMAKE_CXX_STANDARD 17`, `CMAKE_CXX_EXTENSIONS OFF`, `project(BaboViolent2 LANGUAGES C CXX)`, plus `OBJCXX` on Apple if needed).
- Add a `CMakeLists.txt` in each of `engine/babonet`, `engine/zeven`, `engine/dko`, `game` and `masterserver`.
- Options:
  - `BV2_BUILD_CLIENT`: default ON on all three OSes.
  - `BV2_BUILD_SERVER`: default ON.
  - `BV2_BUILD_MASTER`: default ON.
- Add `vcpkg.json` with a partial dependency list (sqlite3, curl); Step 3 completes it. The presets use the toolchain file from `$env{VCPKG_ROOT}`.

### 2.2 Targets

| Target | Type | Sources | Notes |
|---|---|---|---|
| `zeven_core` | static | `engine/zeven/src/{dkc,dksvar,CSystemVariable,CString,CVector,CMatrix}.cpp` | Used by every executable |
| `zeven_client` | static | `engine/zeven/src/{dkw,dki,dkgl,dkt,dkf,CFont,dkp,CParticle,dks}.cpp` | Client only; Step 3 rewrites the platform parts |
| `babonet` | static | `engine/babonet/src/*` | Move `CThread.{cpp,h}` from `game/src` into babonet, which removes the engine→game dependency |
| `dko` | static | `engine/dko/src/*` | Drop its private `CVector.cpp` if identical to zeven's, and link `zeven_core` |
| `bv2` | executable (`WIN32` on Windows, `MACOSX_BUNDLE` on macOS) | Full `game/src` list | Source list from the `ClCompile` entries in `BaboViolent2.vcxproj` |
| `bv2dedicated` | executable | Server subset, `CONSOLE` defined | **Explicit list**, not `*.cpp`. Exclude editor, menu and render files that are fully `#ifndef CONSOLE` |
| `bv2master` | executable | `masterserver/src/*` | Links `babonet`, `zeven_core` and sqlite3 |
| `bv2_seed_db` | custom command | `content-seed/bv2.sql` → `${runtime}/bv2.db` | Generates the DB (Step 1 §1.2); `master.sql` works the same way for `bv2master` |

Declare include directories per target (`target_include_directories(... PUBLIC include PRIVATE src)`). No global `include_directories`.

### 2.3 Platform header

- Create `engine/zeven/include/platform.h`, which defines:
  - `BV2_PLATFORM_WINDOWS`, `BV2_PLATFORM_MACOS` or `BV2_PLATFORM_LINUX`,
  - `BV2_POSIX` on macOS and Linux.
- Merge `LinuxHeader.h`, `linux_types.h` and `platform_types.h` into it.
- Replace every `LINUX64` / `WIN32` / `_WIN32` test (`grep -rn "LINUX64\|defined(WIN32)\|#ifdef WIN32"`).
- `#ifdef LINUX64` branches mean "POSIX" (macOS was never a target), so most become `BV2_POSIX`.
- Windows: define `_WIN32_WINNT=0x0A00` and `WINVER=0x0A00`. The minimum is Windows 10/11 APIs; Windows 11 is the supported floor.

### 2.4 Presets

`CMakePresets.json`, Ninja generator on every preset ([ADR 0001](../../decisions/0001-ninja-generator-on-all-presets.md)):

- `win-x64-msvc` (Ninja + MSVC from Visual Studio 2022+, `toolset`/`architecture` strategy `external`; run from a Developer PowerShell); optional `win-arm64-msvc`, and optional `win-x64-vs` (Visual Studio generator, for a `.sln`).
- `macos-arm64` (`CMAKE_OSX_ARCHITECTURES=arm64`, deployment target 12.0); optional `macos-universal`.
- `linux-x64` (GCC ≥ 13 or Clang ≥ 17); **required**.
- The same presets with `-asan` added (ASan + UBSan; `/fsanitize=address` on MSVC). This replaces VLD.
- Matching `buildPresets` for each.

### 2.5 Linux prerequisites

- vcpkg builds most packages. The host still needs system packages for SDL3/GL on Linux.
- Document them in `README.md` and `tools/setup-dev.sh` (Ubuntu 24.04 package names):
  - `build-essential`, `ninja-build`, `pkg-config`, `autoconf`, `libtool`
  - `libgl1-mesa-dev`, `libglu1-mesa-dev`
  - X11 headers: `libx11-dev`, `libxext-dev`, `libxrandr-dev`, `libxcursor-dev`, `libxi-dev`, `libxss-dev`
  - `libwayland-dev`, `libxkbcommon-dev`, `wayland-protocols`
  - `libasound2-dev`, `libpulse-dev`, `libpipewire-0.3-dev`
  - `libudev-dev`, `libdbus-1-dev`

### 2.6 Runtime layout

- Nothing gets copied into `content/`.
- In development: set the debugger working directory (`VS_DEBUGGER_WORKING_DIRECTORY` for `win-x64-vs`, the launch config for Ninja presets, the Xcode scheme on macOS) to a build-tree runtime dir in which `main/` → `content/` (a symlink, or a junction on Windows) and the generated `bv2.db` sit next to each other.
- `install()` rules:
  - Windows and Linux: `bin/` + `share/bv2/main/`;
  - macOS: `.app/Contents/Resources/main/`.
- Step 4 teaches the code to find these locations.
- **Data for running:** the original assets aren't in the repo (Step 0).
  - Local manual runs: point the runtime `main/` at your own original data (`BV2_DATA_DIR`, step 0 §0.5).
  - CI and agents: `tools/gen-placeholder-content.py` (stdlib only) writes a minimal data set made for this project into the build dir: one small `.bvm` map per game mode (format: [../../analysis/02_DATA_STRUCTURES.md](../../analysis/02_DATA_STRUCTURES.md)), solid-colour TGAs and short silent WAVs under every file name listed as `required` in the asset inventory (kept outside this repository). It's never committed as data; only the generator is.

### 2.7 Module READMEs

- Add a README (≤ 40 lines) to `engine/babonet`, `engine/zeven`, `engine/dko`, `game` and `masterserver` covering:
  - purpose,
  - public headers / API prefix (`bb_*`, `dk*`),
  - dependencies,
  - which executables use it,
  - gotchas (links to `docs/analysis`).

### 2.8 Remove the old build

- Delete the root `Makefile`, the engine `Makefile`s, `BaboViolent2.vcxproj` (plus `.filters` / `.user`) and `Makefile.am`.
- Update `AGENTS.md` "Layout" and "Build", `README.md` (preset commands) and `ARCHITECTURE.md` (full inventory under the new paths).

## Things to watch

- The old server link line pulled in `-lGLU -lcrypto -lssl -lpthread -static-libstdc++`. The server needs none of GLU, crypto or ssl. Use `Threads::Threads` for threading.
- `-w` (all warnings off) was used everywhere. Keep warnings at their defaults here and don't fix them yet; Step 4 does.
- Linux file systems are case-sensitive: `#include "cClient.h"` vs `CClient.h` mismatches will surface here. Fix them to match the real file names.

## Critical files

- New: `CMakeLists.txt` (root and per module), `CMakePresets.json`, `vcpkg.json`, `engine/zeven/include/platform.h`, `engine/*/README.md`, `game/README.md`, `masterserver/README.md`
- Reference then delete: `Makefile`, `Engine/*/Code/Makefile`, `BaboViolent2.vcxproj`, `Makefile.am`

## Acceptance checks

Run these for each of `linux-x64`, `macos-arm64` and `win-x64-msvc`:

```bash
cmake --preset linux-x64
```
```bash
cmake --build --preset linux-x64 --target bv2dedicated bv2master
```

- `bv2dedicated` starts from the runtime dir with `main/bv2.cfg` (copied from `config/bv2.example.cfg`), loads a map and accepts console commands, on all three OSes.
- `bv2master` starts and creates its DB from `content-seed/master.sql`.
- `grep -rn "LINUX64" engine game masterserver` returns nothing.
- `find . -name "* *" -not -path "./.git/*"` returns nothing: no spaces in paths.
- No `Makefile`, `.vcxproj` or `Makefile.am` remains. The old `BaboViolent2/`, `Engine/` and `MasterServer/` directories are gone.
- `tools/check-architecture.sh` passes, and the secret scan is clean.
