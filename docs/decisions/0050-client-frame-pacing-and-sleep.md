# 0050: Client frame pacing follows vsync and window state; the display sleeps outside play

- Status: Accepted
- Date: 2026-11-10
- Affects: ROBO-197, `engine/zeven/src/dkw.cpp`, `engine/zeven/include/dkw.h`, `game/src/FramePacing.h`, `game/src/main.cpp`

## Context
- On macOS the client used about 85% CPU, 91% GPU and kept the Mac awake (owner's Activity Monitor, 2026-11-10).
- `dkwMainLoop` never waits (`engine/zeven/src/dkw.cpp:484` at 54678e0); only a blocking buffer swap paces it. `cl_enableVSync` existed (default true, `game/src/GameVar.cpp:603`) but `dkglEnableVsync` (`engine/zeven/src/dkgl.cpp:31`) had no caller, and the macOS GL swap interval defaults to 0.
- SDL3 disables the screen saver while the video subsystem runs.

## Decision
- **vsync is the frame pacer.** `main.cpp` applies `cl_enableVSync` before the first frame and whenever the console changes it. No fps cap in this change.
- **Window state throttles drawing** (`framePacing::decide`, `game/src/FramePacing.h`). Minimized, hidden or occluded: no render, `paint()` runs at the 30 Hz fixed step so the game and network keep ticking. Unfocused but visible: at most 30 fps. The fixed-step update rate never changes.
- **The display may sleep** (`SDL_EnableScreenSaver`) unless a game is on screen in the focused window.

## Consequences
- With vsync off, a focused window still draws uncapped, as players who turn it off expect. A `cl_maxFPS` is a separate item if needed.
- In a game the Mac still shows "Preventing Sleep: Yes"; in the menu and in the background it doesn't.
- Rejected: a sleep in `dkwMainLoop` for every frame. It adds input latency and fights vsync.
- Rejected: a 10 Hz hidden tick. `dkcUpdateTimer` drops a backlog over 5 steps (about 166 ms), and macOS App Nap can stretch a background sleep past that; 33 ms leaves a margin.
