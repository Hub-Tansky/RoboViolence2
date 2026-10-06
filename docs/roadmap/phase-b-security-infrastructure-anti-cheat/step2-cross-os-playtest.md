# Step 2: Cross-OS play-test

**Status:** TODO

**Depends on:** [Step 1](step1-playtest-builds-and-build-guides.md). **Next:** [step3-test-harness.md](step3-test-harness.md). **Index:** [README.md](README.md)

## Scope

| Field | Value |
|---|---|
| Goal | The owner has played DM, TDM and CTF on macOS, Windows and Linux, including one cross-OS game, and every row of the manual test passes |
| In scope | Tasks 2.1–2.4 below |
| Out of scope | Security fixes (steps 4–7); visual polish, HiDPI (PNS-18); anything that changes the interface (**[GUI]**); bugs that don't block a game (record them in `possible-new-scope.md`) |
| Allowed paths | `docs/roadmap/phase-a-modern-portable-build/phase-a-manual-test.md` (Results only), `docs/build/**`, `engine/zeven/src/**`, `game/src/**` (blocking fixes only), `tests/**`, `ARCHITECTURE.md`, `docs/roadmap/**`, `docs/analysis/KEY_QUESTIONS.md` |
| Inputs | `AGENTS.md`, `ARCHITECTURE.md`, [phase-a-manual-test.md](../phase-a-modern-portable-build/phase-a-manual-test.md), `docs/build/` |
| Deliverables | Filled Results table; one commit per blocking fix, each with a test where possible; issues filed for the rest |
| Definition of done | See "Acceptance checks". All must pass. `ARCHITECTURE.md` is updated |

## Context

- The Phase A manual test was never run on any OS (PNS-1).
- The Windows client has never been started; the macOS client has run only with placeholders. OGG music, HiDPI and a two-machine game are unverified (PR #7).
- The owner runs the tests with their own BV2 data (`BV2_DATA_DIR`); agents can't. Agents fix what the owner reports.

## Tasks

### 2.1 Owner: single-OS runs

- The owner runs the manual test on each OS with the step 1 artifacts or a local build, and reports failures with console logs.

### 2.2 Owner: cross-OS game

- A Linux `bv2dedicated` and `bv2master` with a macOS and a Windows client in one game. Record it in Notes.

### 2.3 Agent: blocking fixes

- Fix each failure that stops a game from starting, connecting, playing a round or downloading a map. One commit per fix, as `step2.3: <summary>`.
- Non-blocking findings go to `possible-new-scope.md` with the owner's log extract.

### 2.4 Record results

- Fill the Results rows (OS version, commit, date, tester, pass per column, notes).

## Critical files

- `docs/roadmap/phase-a-modern-portable-build/phase-a-manual-test.md`
- `engine/zeven/src/dkw.cpp`, `dki.cpp`, `dks.cpp` (likely sites of Windows issues)

## Acceptance checks

- Every Results cell for Linux, macOS and Windows is a pass, and Notes records the cross-OS game.
- `ctest --test-dir build/<preset> --output-on-failure` passes on all three OSes (CI).
- `ARCHITECTURE.md` lists every file added, moved or removed in this step.
- The secret scan (`gitleaks git --staged` / CI) is clean.
