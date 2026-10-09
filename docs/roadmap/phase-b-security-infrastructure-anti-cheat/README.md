# Phase B: security, infrastructure, anti-cheat

**Status:** TODO

**Depends on:** Phase A ([README](../phase-a-modern-portable-build/README.md), DONE). **Rules:** [roadmap README](../README.md) (ground rules, step closing).

## Goal

Playable builds for macOS, Windows and Linux, with working servers, that the owner can download or build from `docs/build/` (step 1) and play in a real game. The server rejects forged and malformed packets without crashing, uses no unsafe SQL and sends no cleartext passwords. Anti-cheat runs through one validator. The servers ship as containers with an authenticated admin channel. The game follows the OS language. Gameplay and the BV2 interface are unchanged ([ADR 0010](../../decisions/0010-bv2-asset-compat-and-gui-freeze.md)).

Out of Phase B: the sim core, replays, server-authoritative movement, accounts, signed distribution, networking rework, renderer, modernisation, replacement assets and GUI changes. See [possible-new-scope.md](../possible-new-scope.md) PNS-12 to PNS-21.

## Steps and dependencies

| # | Scope file | Depends on | Outcome |
|---|---|---|---|
| 0 | [step0-supply-chain-security.md](step0-supply-chain-security.md) | Phase A | Dependency graph, Dependabot, OSV-Scanner, CodeQL; stale `ARCHITECTURE.md` facts fixed |
| 1 | [step1-playtest-builds-and-build-guides.md](step1-playtest-builds-and-build-guides.md) | 0 | CI artifacts with client and servers for all three OSes; build guides in `docs/build/` |
| 2 | [step2-cross-os-playtest.md](step2-cross-os-playtest.md) | 1; 2a for the Windows run | Manual test passes on all three OSes and in a cross-OS game |
| 2a | [step2a-windows-10-target.md](step2a-windows-10-target.md) | 1 | Windows 10 22H2 is the minimum Windows target (ADR 0012); all three executables carry the manifest |
| 3 | [step3-test-harness.md](step3-test-harness.md) | 1 | Packet fuzz/replay harness; master-server tests |
| 4 | [step4-packet-hygiene.md](step4-packet-hygiene.md) | 3 | Sender binding, range and size checks, `SV_CHANGE` filter |
| 5 | [step5-crash-fixes.md](step5-crash-fixes.md) | 3 | R1–R5, UBSan defects, start-up and teardown null dereferences |
| 6 | [step6-sql-prepared-statements.md](step6-sql-prepared-statements.md) | 3 | Bound parameters for every SQL statement |
| 7 | [step7-medium-low-defects.md](step7-medium-low-defects.md) | 3 | R6–R10, R12, R13, R15 and babonet R7 |
| 8 | [step8-server-validator.md](step8-server-validator.md) | 4 | `ServerValidator` with today's checks and a violation score, behaviour unchanged |
| 9 | [step9-anti-cheat-checks.md](step9-anti-cheat-checks.md) | 8 | Weapon, movement and rate checks behind `sv_antiCheat` |
| 10 | [step10-server-credentials.md](step10-server-credentials.md) | 4 | Challenge-response passwords; no MAC identity |
| 11 | [step11-server-containers.md](step11-server-containers.md) | 1 | Container images for `bv2dedicated` and `bv2master` |
| 12 | [step12-remote-admin.md](step12-remote-admin.md) | 10, 11 | Authenticated, rate-limited, audited admin channel |
| 13 | [step13-localisation.md](step13-localisation.md) | 2 | OS language default, gettext PO, checker, translator guide |

Order: 0 → 1 → 2. Step 2a runs while step 2 is IN PROGRESS; step 2's Windows run waits for it. Steps 3–7 run one at a time (they share `ServerRecv.cpp` and `Server.cpp`). Steps 8–10 follow step 4. Steps 11–13 can run alongside once their dependencies are DONE. Every step closes with [REVIEW.md](../../../REVIEW.md) section 5: `tools/review.sh --full`, a fresh-context `/thermo-nuclear-code-quality-review`, and a record in `reviews/stepN.md`.

## Phase B done when

- Every step is DONE.
- CI is green, including CodeQL, and publishes macOS, Windows and Linux artifacts.
- The [manual test](../phase-a-modern-portable-build/phase-a-manual-test.md) passes on all three OSes after step 2, and again after the last step.
- A new contributor builds each OS by following only `docs/build/<os>.md`.
- Fuzz runs (2 min per message type, ASan) find no crash.

## Risks

| Risk | Mitigation |
|---|---|
| No Windows 11 machine; only Windows 10 22H2 is tested by a person | Step 2a makes 22H2 the floor; Windows 11 shares its API level, and CI builds on Windows Server 2022 |
| The Windows client has never been run; step 2 may grow | Fix only what blocks a game; record the rest in `possible-new-scope.md` |
| Unsigned test builds are blocked by Gatekeeper and SmartScreen | The guides document the workaround; signing is PNS-16 |
| The manual test needs the owner and three machines | Between runs, rely on CI, the fuzz harness and the behaviour-pinning tests (step 8) |
| Steps 4 and 10 bump the protocol; old clients stop connecting | Allowed (Phase A README: free to break 2.11). Bump `GAME_VERSION_SV/CL` and update `tests/test_netpacket.cpp` |
| R3 (`sv_serverType = 1`) is load-bearing | Step 5 stops for the owner's ruleset decision and an ADR |
| Anti-cheat false positives kick laggy players | `sv_antiCheat` defaults to log-only; tune thresholds on play-tests |
| TLS for remote admin needs a dependency (OpenSSL was removed) | Step 12 decides in an ADR; likely TLS ends at a reverse proxy |
| The Linux bundle lacks system GL/GLU on clean installs | Step 1 documents the packages and tests in a clean `ubuntu:24.04` container |
| Fuzzing finds more defects than R1–R15 | Record them in `docs/analysis/KEY_QUESTIONS.md`; out-of-scope ones go to `possible-new-scope.md` |
| The review skill isn't installed | REVIEW.md: stop and ask the owner |
| Step 0 changes repository settings | The owner confirms at execution time |
