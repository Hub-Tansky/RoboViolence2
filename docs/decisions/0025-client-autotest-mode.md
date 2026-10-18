# 0025: Test the real client through a scripted autotest mode

- Status: Accepted
- Date: 2026-10-18
- Affects: [Project item Phase T3](https://github.com/users/Hub-Tansky/projects/1), `game/src/Autotest.cpp`, `game/src/main.cpp`

## Context
- CI only knew whether the client was still alive after 10 s (`.github/workflows/build.yml` `smoke`, `package-smoke`); an init failure blocks in a message box (`game/src/main.cpp`, `dkwShowMessage`), so "alive" proved little.
- Everything a player can do from the menu is reachable as a console command (`game/src/Console.cpp` `sendCommand`), and every console line goes through `Console::add`.

## Decision
`bv2 --autotest <script> [--timeout <s>]` runs one step per frame tick: console commands, `wait`, `expect-console`, `expect-scene`. The client prints console lines and one result line to stdout and exits 0 only when the script passes, 1 (an expect unmet when the budget ends, a bad script, or the client quitting or failing init first) or 2 (a `wait` outlasts the budget, or a watchdog thread fires 30 s after it because the loop or init hung).

## Consequences
- CI, the playtest agent and E2E scenarios (T4) can assert client behaviour; screenshots (T7) add a step.
- The runner is plain C++ (`Autotest.h`), unit-tested without a window (`tests/test_autotest.cpp`).
- Rejected: a separate test binary linking the client (duplicates `main.cpp` init); asserting on log text after a fixed sleep (slow and flaky); input injection (no stable widget IDs, and the GUI is frozen, ADR 0010).
