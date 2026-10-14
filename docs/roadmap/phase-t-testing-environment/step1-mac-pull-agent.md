# Step 1: Mac pull agent

**Status:** DONE (2026-10-14)

**Depends on:** Phase B step 2b (release packages), step 12 (admin API). **Next:** T8 staging (candidate). **Index:** [README.md](README.md)

## Scope

| Field | Value |
|---|---|
| Goal | Each successful `main` build is downloaded, verified, installed and tested on the owner's Mac against a disposable local server, with a pass or failure report and notification |
| In scope | Tasks 1.1 … 1.5 below; plan item T1 ([plan](../20261014_testing_environment_plan.md#t1-mac-pull-agent-proposed-step)) |
| Out of scope | Staging host and deploy agent (T8); CI client smoke on Windows/macOS (T2); client autotest mode (T3); E2E bots (T4); any game, packaging or CI change (Phase C) |
| Allowed paths | `tools/playtest/**`, `tests/test_playtest_agent.sh`, `tests/CMakeLists.txt`, `docs/roadmap/**`, `docs/decisions/**`, `ARCHITECTURE.md` |
| Inputs | `AGENTS.md`, `ARCHITECTURE.md`, the plan, `packaging/make-package.py`, `packaging/scripts/`, `game/src/AdminApi.cpp`, [ADR 0015](../../decisions/0015-admin-http-api-behind-tls-proxy.md) |
| Deliverables | `tools/playtest/agent.sh`, `tools/playtest/com.roboviolence2.playtest.plist`, `tools/playtest/README.md`, `tests/test_playtest_agent.sh`, ADR 0017 |
| Definition of done | See "Acceptance checks". All must pass. `ARCHITECTURE.md` is updated |

## Context

- CI uploads `roboviolence2-macos-arm64.zip` per build (`.github/workflows/build.yml`, `archive: false`). The Actions API lists it with a `sha256:` digest.
- Prototype on the owner's Mac (2026-10-14), build `5cfcfee`: `gh api …/artifacts/<id>/zip` returns the exact uploaded zip (digest matched); `gh run download` extracts it, so it can't be verified and isn't used. Packaged master and server started; the admin API on 127.0.0.1 answered; the client with `--config <absolute path> connect 127.0.0.1` joined (`players 1`, configured name); `quit` on the server's stdin and `kill` stopped everything.
- Config files use `name value` lines (`config/bv2.example.cfg`), not `set name value`.
- The packaged master and server listen on all interfaces, not only loopback (`lsof`: `*:10207`).

## Tasks

### 1.1 Track and scope

- `docs/roadmap/phase-t-testing-environment/` (README, this file); row in `docs/roadmap/README.md`.

### 1.2 Agent

- `tools/playtest/agent.sh` (macOS `/bin/bash` 3.2, `jq`, `curl`, `ditto`, `nc`, `gh`), per the plan's T1 with the deviations below.
- Modes: default (`local-smoke` on the newest build), `--sha <sha>`, `--test staging-connect`, `--play local|staging`, `--stop`, `--preflight`.

### 1.3 Tests

- `tests/test_playtest_agent.sh`: sources the agent's helpers and checks run selection (fork PR from `main`, other branch, failed run), the artifact digest, the attempt rule and the env-file mode check. ctest `playtest_agent` on Unix.

### 1.4 LaunchAgent and README

- `tools/playtest/com.roboviolence2.playtest.plist` (template; `__HOME__` placeholder), `tools/playtest/README.md` (install, env file, operation, uninstall).

### 1.5 ADR and inventory

- ADR 0017: pull agent, main-only, disposable local server, staging read-only, plaintext admin API on loopback.
- `ARCHITECTURE.md`, `docs/decisions/README.md`.

## Deviations from the plan

| Plan | Implemented | Why |
|---|---|---|
| `gh run download` | `gh api …/artifacts/<id>/zip` | `gh run download` extracts the zip, so the digest can't be checked |
| Env file loaded with `set -a` | Sourced without export; each secret goes only to the process that needs it | The client and `curl` never see the admin password; `curl` reads the header from stdin, not argv |
| Preflight via `launchctl kickstart` and a flag | Every run starts with the preflight checks (stage `preflight`); `--preflight` runs them alone | The first LaunchAgent run is the preflight; no extra flag. Infrastructure failures notify once per distinct stage |
| Keep 5 report sets | Keep 30 reports; 5 versions, prefs and run dirs | Reports are small and are the history |
| — | While `--play` is running, scheduled runs skip without counting an attempt | Manual play holds the fixed ports |
| Server ready when the log says `Server Created` | Server ready when its game port listens | The server's stdout is block-buffered in a file (review finding 1) |
| Every failure counts as an attempt | Only failures from `verify` on count; GitHub, download, busy port and no console session notify once | Environment problems must not use up a build's attempts (review finding 5) |
| `versions/<date>-<sha7>` | `versions/<sha7>`, reused when the same build is tested again; unzip beside, then swap | Re-testing never deletes the live package (review finding 6) |
| — | A run stopped by SIGTERM/SIGINT (logout, `bootout`) records nothing | Not a build failure (review finding 13) |
| — | Automated client runs pass `--placeholders` | Phase C step 1 (ADR 0016) added a first-run data-folder dialog that would block the unattended client; builds before `67fd59f` don't know the flag: `--sha` on them fails at the join check |

## Critical files

- `tools/playtest/agent.sh`, `packaging/scripts/run-client.sh`, `game/src/AdminApi.cpp:110`

## Acceptance checks

- `ctest --test-dir build/macos-arm64 -R playtest_agent --output-on-failure` passes.
- `tools/playtest/agent.sh --sha <latest main sha>` on the owner's Mac: report shows every stage passed, notification `READY TO TEST`, no `bv2*` process left (`pgrep -l bv2` empty).
- A digest mismatch is rejected (`playtest_agent` test); a busy master port fails at `ports-busy` and notifies with the stage.
- `--play local` starts master, server and client; a scheduled run meanwhile skips without an attempt; `--stop` leaves no `bv2*` process.
- After the owner installs the LaunchAgent: the next `main` build is reported within 20 minutes of its build ending, while the Mac is awake (owner check; `launchd` doesn't run the agent during sleep, owner decision 2026-10-14).
- `tools/review.sh --full macos-arm64` passes, and `reviews/step1.md` and `reviews/step1-report.md` exist (`REVIEW.md` section 5).
- `ARCHITECTURE.md` lists every file added in this step.
- The secret scan is clean.
