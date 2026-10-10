# Step 2: Cross-OS play-test

**Status:** IN PROGRESS

**Depends on:** [Step 1](step1-playtest-builds-and-build-guides.md); the Windows run also on [step 2a](step2a-windows-10-target.md). **Next:** [step2a-windows-10-target.md](step2a-windows-10-target.md). **Index:** [README.md](README.md)

## Scope

| Field | Value |
|---|---|
| Goal | The owner has played DM, TDM and CTF on macOS, Windows and Linux, including one cross-OS game, and every row of the manual test passes |
| In scope | Tasks 2.1–2.6 below |
| Out of scope | Security fixes (steps 4–7); visual polish, HiDPI (PNS-18); anything that changes the interface (**[GUI]**); bugs that don't block a game (record them in `possible-new-scope.md`) |
| Allowed paths | `docs/roadmap/phase-a-modern-portable-build/phase-a-manual-test.md` (Results only), `docs/build/**`, `engine/zeven/src/**`, `game/src/**` (blocking fixes only), `game/src/Client.cpp` (owner-approved 2026-10-08: task 2.4), `game/src/Map.cpp` (owner-approved 2026-10-10: task 2.5), `tests/**`, `ARCHITECTURE.md`, `docs/roadmap/**`, `docs/analysis/KEY_QUESTIONS.md` |
| Inputs | `AGENTS.md`, `ARCHITECTURE.md`, earlier review records `reviews/step*.md`, [phase-a-manual-test.md](../phase-a-modern-portable-build/phase-a-manual-test.md), `docs/build/` |
| Deliverables | Filled Results table; one commit per blocking fix, each with a test where possible; issues filed for the rest |
| Definition of done | See "Acceptance checks". All must pass. `ARCHITECTURE.md` is updated |

## Context

- The Phase A manual test was never run on any OS (PNS-1).
- The Windows client has never been started; the macOS client has run only with placeholders. OGG music, HiDPI and a two-machine game are unverified (PR #7).
- The owner runs the tests with their own BV2 data (`BV2_DATA_DIR`); agents can't. Agents fix what the owner reports.

- Owner play-test, macOS arm64 with original data (2026-10-08): the game runs smoothly. Mid-match weapon choice applies on respawn (original behaviour). Fullscreen and resolution apply after a restart (original behaviour). Screenshot (P) and stats (L) do nothing on macOS and Linux: `game/src/Client.cpp:444` compiles them for Windows only (`#ifdef BV2_PLATFORM_WINDOWS`).

- The owner's only Windows machine is Windows 10 Pro 22H2 (2026-10-10). The Windows run happens there, after step 2a makes 22H2 the supported floor.

- macOS packages are probably limited to the CI runner's macOS (14+), not the documented 12+: vcpkg ports ignore the deployment target (PNS-26). Ask testers for their macOS version.

- Test machines (owner): MacBook Air M3 2024 (macOS 27.0.1), Windows 10 Pro 22H2 (needs [step 2a](step2a-windows-10-target.md) first), Linux Mint 22.3 Cinnamon (Ubuntu 24.04 base, matching the Linux reference).

- Owner, 2026-10-10: task 2.2, the Windows client run, the Linux re-test of 2.5 and the Results rows are deferred → PNS-31; the step closes without them.

## Tasks

### 2.1 Owner: single-OS runs

- The owner runs the manual test on each OS with the step 1 artifacts or a local build, and reports failures with console logs.

- **Linux done (owner, 2026-10-10, Linux Mint 22.3 Cinnamon, CI package of 285e15b):** the game runs with original data; terminal shows only `Cannot open texutre file: main/textures/sky.tga` (the original data has no `sky.tga`, harmless). Found the wheel bug in 2.5.

### 2.2 Owner: cross-OS game

- A Linux `bv2dedicated` and `bv2master` with a macOS and a Windows client in one game. Record it in Notes.

### 2.3 Agent: blocking fixes

- Fix each failure that stops a game from starting, connecting, playing a round or downloading a map. One commit per fix, as `step2.3: <summary>`.
- Non-blocking findings go to `possible-new-scope.md` with the owner's log extract.

### 2.4 Screenshot and stats keys on macOS and Linux (owner request)

- Remove the `#ifdef BV2_PLATFORM_WINDOWS` around P (screenshot) and L (screenshot + stats text) at `game/src/Client.cpp:444–455`. The save paths already use `bv2::userFile` (`game/src/screengrab.cpp:70–86`), which creates `screenshots/` in the pref dir. This restores the Windows behaviour on macOS and Linux; it adds no new control.
- Check that `SaveScreenGrab` reads the framebuffer at pixel size on HiDPI (Retina: points vs pixels).
- Proof is manual, because it needs a GL context: the owner presses P and L in a game and the `.bmp`/`.txt` files appear. Unit-test the BMP writer if it can be separated from GL.
- **Done (owner, 2026-10-10, macOS 27.0.1 on a MacBook Air M3, local build of 56a0d08):** P and L save `.bmp` and `.txt` files in `~/Library/Application Support/BaboViolent2/bv2/screenshots/`; the `.bmp` shows the full game window.
- **Done (owner, 2026-10-10, Linux Mint 22.3, CI package):** P and L save correct files in `~/.local/share/BaboViolent2/bv2/screenshots/`.

### 2.5 Mouse wheel zooms the map behind an open menu (owner report, Linux)

- Owner, Linux Mint 22.3 (2026-10-10): in a game, pause → main menu → Options; scrolling the options list with the wheel also zooms the map behind it. Not seen on macOS.
- Cause: `Map::update` zooms on `dkiGetMouseWheelVel()` for spectators and the editor (`game/src/Map.cpp`) without checking whether a menu or the console has the input.
- Fix: `wheelUsedByUi()` skips the zoom while the pause, main or options menu or the console is open. Proof is manual (needs a GL window): the owner repeats the steps on Linux; the map no longer zooms.

### 2.6 Record results

- Fill the Results rows (OS version, commit, date, tester, pass per column, notes).

## Critical files

- `docs/roadmap/phase-a-modern-portable-build/phase-a-manual-test.md`
- `engine/zeven/src/dkw.cpp`, `dki.cpp`, `dks.cpp` (likely sites of Windows issues)

## Acceptance checks

- Every Results cell for Linux, macOS and Windows is a pass, and Notes records the cross-OS game.
- `ctest --test-dir build/<preset> --output-on-failure` passes on all three OSes (CI).
- `tools/review.sh --full <preset>` passes, and the fresh-context review record `reviews/step2.md` and the raw reviewer report `reviews/step2-report.md` (with the skill's load line) exist ([REVIEW.md](../../../REVIEW.md) section 5).
- `ARCHITECTURE.md` lists every file added, moved or removed in this step.
- The secret scan (`gitleaks git --staged` / CI) is clean.
