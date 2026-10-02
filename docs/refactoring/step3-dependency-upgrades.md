# Step 3: Dependency upgrades and platform layer

**Depends on:** [Step 2](step2-cmake-build.md). **Coordinate with:** [Step 4](step4-64bit-and-cross-platform.md), which touches the same engine files. **Index:** [README.md](README.md)

## Scope

| Field | Value |
|---|---|
| Goal | Every dependency comes from `vcpkg.json`; the client runs on Windows 11, macOS and Linux through SDL3 + miniaudio + glad behind the unchanged `dk*` APIs |
| In scope | Tasks 3.1–3.10 below |
| Out of scope | Protocol and struct changes (Step 4); prepared-statement SQL fixes and other security fixes ([future-phases.md](future-phases.md) §A); renderer rewrite (§F, [ADR 0003](../decisions/0003-keep-opengl-2.1-then-sdl-gpu.md)); babonet defect R7 unless it blocks testing |
| Allowed paths | `vcpkg.json`, `CMakeLists.txt` files, `engine/zeven/**`, `engine/babonet/src/**` (portability only), `game/src/{main.cpp,CCurl.cpp,CCurlStub.cpp,CStatus.cpp,CFriends.cpp}` and call sites that break because of API changes, `ARCHITECTURE.md`, `AGENTS.md` |
| Inputs | `AGENTS.md`, `ARCHITECTURE.md`, [../analysis/01_SYSTEM_OVERVIEW.md](../analysis/01_SYSTEM_OVERVIEW.md) (engine subsystems), [../analysis/KEY_QUESTIONS.md](../analysis/KEY_QUESTIONS.md) Q1, R11, R14, ADRs [0002](../decisions/0002-compile-out-libcurl.md) and [0003](../decisions/0003-keep-opengl-2.1-then-sdl-gpu.md) |
| Deliverables | The vcpkg manifest; rewritten `dkw`, `dki`, `dks`, `dkc` and GL loading; `CCurl` stub behind `BV2_WITH_HTTP` |
| Definition of done | "Acceptance checks" below pass on all three OSes |

## Context

The vendored dependencies are 15–20 years old:

- SQLite 3.3.10
- libcurl 7.16.3
- FMOD 3.75 (Windows) and FMOD Studio 1.09 (Linux, `USE_FMODEX`)
- `glext.h` v29
- DirectInput 8
- a Win32/WGL window and an SDL 1.2 window

This step rewrites the thin DukZeven platform modules behind their **existing public headers**, so the ~47k lines of game code keep calling the same `dk*` API.

## Tasks

### 3.1 `vcpkg.json`

Pin the `builtin-baseline`. Dependencies:

- `sqlite3`
- `sdl3`, with the `x11`, `wayland`, `alsa` and `pulseaudio` features on Linux as needed
- `miniaudio`
- `glad`, with GL 2.1 compatibility + the needed extensions; or generate glad2 sources into `engine/zeven/third_party/`
- `stb` (optional, see 3.8)

The client-only packages sit behind a vcpkg feature `client`, so server-only builds (for example a Linux server container) stay small. `curl` sits behind a vcpkg feature `http`, enabled only with `BV2_WITH_HTTP` (3.3).

No libcurl in the default build ([ADR 0002](../decisions/0002-compile-out-libcurl.md)).

### 3.2 SQLite 3.3.10 → current 3.x

- Users: `game/src/{GameVar,Scene,Server,ServerRecv,CMaster}.cpp` and `masterserver/src/cMasterServer.cpp`.
- The API is source-compatible; link `unofficial::sqlite3::sqlite3`.
- Don't rewrite queries here.

### 3.3 libcurl: compile out behind `BV2_WITH_HTTP`

Its only backend (the account/ladder server) is gone; see [ADR 0002](../decisions/0002-compile-out-libcurl.md).

- Add the CMake option `BV2_WITH_HTTP`, default **OFF**.
- OFF: build `game/src/CCurlStub.cpp` instead of `CCurl.cpp`. Same `CCurl.h` API; every request finishes at once with 0 bytes received. Remove `#pragma comment(lib, "libcurl.lib")` (old `main.cpp:49`).
- Check that `CStatus` and `CFriends` handle an empty response; the server auth path already does (old `Server.cpp:605`).
- ON (not built in CI for Phase A): vcpkg `curl` 8.x via the `http` feature, TLS through Schannel on Windows and OpenSSL on macOS/Linux, `XFERINFOFUNCTION` instead of removed options, `curl_global_init` once, certificate verification **on**.

### 3.4 Audio: FMOD → miniaudio (`dks`)

- Rewrite `engine/zeven/src/dks.cpp` (44 FMOD calls) behind the unchanged `dks.h`, then delete the FMOD code.
- Map concepts: `ma_engine` for the global engine; `ma_sound` per sample with `MA_SOUND_FLAG_DECODE` for effects; streaming for music.
- Use `ma_sound_set_position` if `dks` exposes positional playback; otherwise pan and volume.
- Backends are chosen automatically: WASAPI on Windows, Core Audio on macOS, PipeWire, PulseAudio or ALSA on Linux. Verify on a PipeWire desktop (Ubuntu 24.04 default).
- Required formats are in the sounds section of `docs/assets/ASSET-INVENTORY.md` (WAV effects, OGG music). **OGG needs `stb_vorbis`**.

### 3.5 Window + GL context: `dkw` → SDL3

- There's one implementation in `engine/zeven/src/dkw.cpp`. Delete the Win32 `CreateWindow`/WGL path (around old `dkw.cpp:583`) and the SDL 1.2 `SDL_SetVideoMode` path (around old `dkw.cpp:659`).
- Request an **OpenGL 2.1 compatibility** context. Immediate-mode `glBegin` is used in 22 game files.
  - macOS: the legacy 2.1 context.
  - Linux: Mesa gives a compatibility profile on X11 and Wayland.
  - Windows: vendor drivers.
- `WinMain` becomes a single `main()` with SDL3 main handling. Windows keeps the `WIN32` subsystem.
- Keep the main-loop contract: `dkw` drives the fixed 30 Hz update through `dkc`.
- HiDPI: use the pixel size for `glViewport` (Retina, Windows scaling, Wayland fractional scaling).
- Linux: SDL picks Wayland or X11. Test both (`SDL_VIDEO_DRIVER=x11` / `wayland`).

### 3.6 Input: `dki` → SDL3

- Replace DirectInput 8 (old `dki.cpp:202`) with SDL3 keyboard, mouse and gamepad events.
- Keep the `dki.h` key-code constants and add a scancode → dki table, so `bv2.cfg` binds stay valid.
- Mouse: relative mode for aiming.
- Text input: `SDL_EVENT_TEXT_INPUT` (UTF-8; convert to Latin-1 before handing it to the bitmap font, which renders by byte value, until `dkf` decodes UTF-8).

### 3.7 GL loading: `glext.h` v29 → glad

OpenGL 2.1 stays for Phase A; renderer isolation and the SDL_GPU move come later ([ADR 0003](../decisions/0003-keep-opengl-2.1-then-sdl-gpu.md)).

- Collect the extensions in use with `grep -rhn "gl[A-Z][a-zA-Z]*ARB\|wglGetProcAddress\|glXGetProcAddress" engine game/src`.
- Generate glad for GL 2.1 compat + those extensions, and load through `SDL_GL_GetProcAddress`.
- GLU stays for now (`gluSphere` / `gluLookAt` in `CUserLogin.cpp` and elsewhere). Link `OpenGL::GLU` (`libglu1-mesa` on Linux, `OpenGL.framework` on macOS). Define `GL_SILENCE_DEPRECATION` on macOS.

### 3.8 Textures (`dkt`)

- The TGA loader stays. Only switch to `stb_image` if TGA parsing breaks on 64-bit (see Step 4, struct reads).

### 3.9 Threads and timer

- `CThread`, now in babonet: back it with `std::thread`, `std::mutex` and `std::condition_variable`.
- Replace the console-thread `bool` spin-lock (old `main.cpp:418-451`) with `std::atomic` / `std::mutex`. This fixes **R11**.
- `dkc`: use `std::chrono::steady_clock`. Keep `dkcGetElapsedf()` returning `1/30`. Cap catch-up steps (for example at 5), which fixes **R14**.

### 3.10 babonet sockets

- Fix only portability:
  - `socklen_t`
  - `MSG_NOSIGNAL` on Linux / `SO_NOSIGPIPE` on macOS
  - `fcntl` vs `ioctlsocket`
  - `WSAStartup` once
  - `EINTR` handling

## Critical files

- `vcpkg.json`, `engine/zeven/CMakeLists.txt`
- `engine/zeven/src/{dkw,dki,dks,dkc,dkgl,dkt}.cpp` and their headers
- `game/src/{main.cpp,CCurl.cpp,CCurlStub.cpp}`
- `engine/babonet/src/{cClient,cServer,cConnection,cPeer2Peer,CThread}.cpp`

## Acceptance checks

- `cmake --build --preset <p>` builds `bv2`, `bv2dedicated` and `bv2master` for `win-x64-msvc`, `macos-arm64` and `linux-x64`.
- `grep -rn "FSOUND_\|FMOD\|DirectInput\|SDL_SetVideoMode\|wglCreateContext" engine game` returns nothing.
- With the default options, no libcurl or OpenSSL library is linked into `bv2`, `bv2dedicated` or `bv2master` (check `ldd` / `otool -L` / `dumpbin /dependents`).
- Manual on **Windows 11, macOS and Linux (both X11 and Wayland)**:
  - the client reaches the main menu;
  - key binds and chat input work;
  - effects and music play;
  - hosting a listen server works ([../analysis/KEY_QUESTIONS.md](../analysis/KEY_QUESTIONS.md) Q7);
  - joining a separate `bv2dedicated` and downloading a map works.
- The `-asan` preset run has no new ASan reports in `dkw`, `dki`, `dks` or `dkc`.
- `ARCHITECTURE.md` is updated (dependency list, platform layer summary); the secret scan is clean.
