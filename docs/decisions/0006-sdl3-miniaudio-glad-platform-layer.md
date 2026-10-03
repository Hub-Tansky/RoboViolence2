# 0006: SDL3, miniaudio and glad behind the unchanged dk* APIs

- Status: Accepted
- Date: 2026-10-03
- Affects: [step 3](../refactoring/step3-dependency-upgrades.md)

## Context
- The platform modules were a Win32/WGL window, DirectInput, FMOD 3 and an SDL 1.2 path that never had a client `main` (`game/src/main.cpp` had only `WinMain` for the client).
- The game calls `dkw*`, `dki*`, `dks*`, `dkgl*` from about 90 sites; key binds in `bv2.cfg` are legacy PC scancodes (`k_moveUp 17`).

## Decision
Rewrite the modules behind their headers: `dkw`/`dki` on SDL3 (SDL scancodes map to the same `DIK_*` IDs, `dikeys.h`), `dks` on miniaudio, GL entry points from a generated glad (GL 2.1 compatibility) committed under `engine/zeven/third_party/glad`. The window API no longer exposes `HWND`/`HDC`; `dkglSwapBuffers()` replaces `SwapBuffers`, `dkwShowMessage()` replaces `MessageBox`.
- `dkwGetResolution()` is the drawable size in pixels; mouse coordinates are scaled to it.
- Sound handles are channel ints with a serial, so a stale `dksStopSound` cannot stop a reused slot. `FSOUND_SAMPLE` became `DksSound`.
- OGG music needs `stb_vorbis.c` (vcpkg `stb`); without it `.ogg` does not play and the build says so.

## Consequences
- One window, input and audio path on Windows, macOS and Linux; no FMOD or DirectInput.
- The server still needs GLU (headers and library) because `dko` calls it; `dkt` is stubbed there.
- Rejected: glad from vcpkg (no port with a pinned GL 2.1 + compat profile); keeping `HWND` types in the API (leaks Win32 into SDL code).
