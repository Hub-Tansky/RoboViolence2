# Step 7: Windows TCP-close crash (R28)

**Status:** IN PROGRESS

**Depends on:** Phase B DONE. **Next:** Phase C close. **Index:** [README.md](README.md)

## Scope

| Field | Value |
|---|---|
| Goal | babonet no longer crashes on Windows when a TCP connection closes, and the master test runs on Windows again |
| In scope | Tasks 7.1–7.3 below (PNS-38 R28); debugging through a Windows CI job (owner, 2026-10-17) |
| Out of scope | Other babonet rewrites (PNS-17, Phase D); the master's game list cleanup (PNS-67) |
| Allowed paths | `engine/babonet/**`, `masterserver/src/**`, `tests/**`, `.github/workflows/r28-windows.yml` (temporary diagnostic, removed at the end), `docs/analysis/**`, `docs/decisions/**`, `AGENTS.md`, `ARCHITECTURE.md`, `docs/roadmap/**` |
| Inputs | `AGENTS.md`, `ARCHITECTURE.md`, PNS-38, `docs/analysis/KEY_QUESTIONS.md` R28, `engine/babonet/README.md` |
| Deliverables | Root cause and fix in babonet; `master` test enabled on Windows; KEY_QUESTIONS R28 updated |
| Definition of done | See "Acceptance checks". All must pass. `ARCHITECTURE.md` is updated |

## Context

- R28: on Windows, an access violation when a TCP connection closes (a client's `bb_clientDisconnect` right after receiving; the master dropping timed-out clients). Found by the step 3 master test (CI runs 38073594887, 38074969674); macOS and Linux ASan are clean. The test is skipped on Windows (`tests/CMakeLists.txt`, `if(NOT WIN32)`).

## Tasks

### 7.1 Reproduce and locate

- A temporary workflow runs `test_master` many times on `windows-2025`, under MSVC ASan and under `cdb`, and prints the faulting stack.

### 7.2 Fix

- Fix the cause in babonet (or the master); add a test if the cause can be exercised on every OS.

### 7.3 Enable and clean up

- Run the `master` test on Windows in the normal CI; delete the diagnostic workflow; update KEY_QUESTIONS R28.

## Critical files

- `engine/babonet/src/cConnection.cpp`, `engine/babonet/src/cClient.cpp`, `engine/babonet/src/cServer.cpp`, `tests/CMakeLists.txt`

## Acceptance checks

- The diagnostic loop passes 50 runs on Windows (ASan and Debug) after the fix, and failed before it (recorded in the review).
- CI `build (win-x64-msvc)` runs and passes the `master` test.
- `tools/review.sh --full <preset>` passes, and the fresh-context review record `reviews/step7.md` and the raw reviewer report `reviews/step7-report.md` (with the skill's load line) exist (`REVIEW.md` section 5).
- `ARCHITECTURE.md` lists every file added, moved or removed in this step.
- The secret scan (`gitleaks git --staged` / CI) is clean.
