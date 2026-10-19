# 0026: Screenshot goldens are Linux llvmpipe frames compared within a tolerance

- Status: Accepted
- Date: 2026-10-19
- Affects: [Project item Phase T7](https://github.com/users/Hub-Tansky/projects/1), `tests/screenshots.py`, `tests/golden/`, `game/src/screengrab.cpp`

## Context
- Autotest (ADR 0025) checks console text and scenes, not what is drawn.
- Reading `GL_FRONT` after the swap gives a black frame under Xvfb, so the P-key path can't feed tests.
- Rendering differs per GPU and driver; macOS and Windows CI have no shared software renderer with Linux.

## Decision
- Client console commands `screenshot <name>` (the next frame, read from the back buffer before the swap, to `<pref dir>/screenshots/<name>.bmp`), `menutab <tab>` and `scoreboard 0|1` let autotest scripts reach and capture a screen.
- Goldens are PNGs from the Linux client on placeholders under Xvfb (Mesa llvmpipe), one autotest script beside each: `tests/golden/<name>.txt` and `.png`. ctest `screenshot_<name>` runs on Linux only.
- A screenshot passes when at most 0.5 % of its pixels differ by more than 24 in a channel. A pass deletes the client's pref dir; a failure keeps `<name>-actual.png` and `<name>-diff.png` for the CI artifact.
- Regenerate a golden with `tests/screenshots.py <bv2> <scratch> <name> --update` after an intended visual change (also the version text in the menu).

## Consequences
- GUI regressions on Linux fail CI; macOS and Windows rendering stays untested.
- No image library: `screenshots.py` reads the client's BMP and writes and reads its own 8-bit RGB PNGs with `zlib`.
- Rejected: per-OS goldens (three sets to keep in step, and no CI display on Windows without Mesa); exact matching (rasterizer rounding across Mesa versions).
