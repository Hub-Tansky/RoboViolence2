# Testing environment plan (zero-cost variant)

**Status:** PROPOSED (2026-10-14), revised after owner review. Proposal for approval: T1 is a standalone step; T2–T7 are candidate follow-up steps, scoped only after T1 is proven.

## Goal

Every successful `main` build is downloaded, installed, tested against a disposable local server and reported (pass or failure) without manual work. Tests that need a real network use the hosted staging server on demand. Mac is the hub; the Windows/Linux workstation is for real-GPU checks only.

## Constraints

- Zero recurring cost. Every tool below is free for this use.
- Never execute pull request code on the owner's machines. The repo is public: no self-hosted runner, and no agent downloads PR artifacts. PR testing, if ever needed, is a separate, explicitly approved, isolated flow (not planned).
- Secrets stay in a local env file outside the repo ([config/README.md](../../config/README.md)).
- `gh` runs as `GH_TOKEN=$(gh auth token --user Hub-Tansky) gh …` (AGENTS.md).
- Free Actions minutes depend on the repo staying public (private: 2000 min/month, macOS minutes ×10).

## Public and private material

| Goes in the repo (public) | Goes in `docs/prv_wiki/docs_restricted/testing-environment/` (git-ignored by `/docs/prv_*/`) | Goes only in env files (mode 600) or a password manager |
|---|---|---|
| this plan, ADRs, agent scripts, plist and compose templates with placeholders, generic README | Mac setup (paths, user, installed tool versions, LaunchAgent install notes, preflight output), staging host (provider, address, ports, router forward, SSH access), workstation and VM details, LAN addresses, ntfy topic name | admin passwords, tokens |

The private wiki is a plaintext Obsidian vault, so secrets stay out of it as well. Public docs refer to private facts by role ("the staging host"), never by value.

## Already in place

| What | Where | Gating |
|---|---|---|
| Debug + Release build, ctest, packages for 3 OSes | `.github/workflows/build.yml` job `build` | required |
| Packaged server and master start (Linux package only, clean `ubuntu:24.04`) | `build.yml` job `package-check` | not required |
| Client reaches the main menu for 10 s (Linux ASan build, not the package) | `build.yml` job `smoke` | required |
| Packet fuzzing | `build.yml` job `fuzz` | not required; `main` only, red until known defects are fixed |
| Server/master images, `:latest` and `:<sha>`, amd64 | `.github/workflows/containers.yml` (separate workflow) | n/a |
| Admin HTTP API: `GET /status` (map, game type, score, player list), `POST /command` | `game/src/AdminApi.cpp:110`, [ADR 0015](../decisions/0015-admin-http-api-behind-tls-proxy.md) | n/a |
| Run scripts; the client takes one console command | `packaging/scripts/` | n/a |

## Tools (all free)

| Need | Tool |
|---|---|
| CI, artifacts | GitHub Actions (free for public repos) |
| Artifact download | `gh` CLI |
| Scheduling on Mac | `launchd` LaunchAgent (user GUI domain) |
| Notifications | ntfy.sh (random topic, in the env file) + `osascript` desktop notification |
| Windows on Mac (T5) | VMware Fusion Pro (free) + Windows 11 ARM, unactivated |
| Workstation (T6) | Windows 11 + WSL2/WSLg |
| GL on Windows CI (T2) | Mesa llvmpipe from mesa-dist-win, pinned release |

## T1: Mac pull agent (proposed step)

**Definition:** the agent downloads only a successful `main` push package, verifies and installs it in an isolated version directory, and tests the packaged client against one of two targets, chosen per test: a disposable local server started from the same package, or the hosted staging server. It records a report and sends a pass or failure notification for every attempted SHA. It never downloads or executes PR artifacts.

### Targets

| Target | Server | Used by | Lifetime | Checks |
|---|---|---|---|---|
| `local` | packaged `bv2master` + `bv2dedicated` from the same artifact, on loopback | per-build automated test (unattended); manual play with `--play local` | started for one test, stopped after it | full: server log, master port, `/status` over plain HTTP on 127.0.0.1 |
| `staging` | hosted server run by the owner from the GHCR images (compose, [docs/ops/runbook.md](../ops/runbook.md)) | on-demand tests that need a real network, the public master or other players: `--test staging-connect`, `--play staging` | long-lived, shared | read-only: client joins; `GET /status` over TLS shows the agent's player |

Rules:
- Per-build automated tests run only against `local`. They never touch staging, so they stay isolated: no shared state, no other players, no dependency on staging uptime or its deployed version.
- Every `local` run gets a fresh pref dir, fresh copies of `master.db`/`web.db`, and fixed loopback ports. It is torn down after the test whether it passes or fails.
- The agent sends staging only `GET /status`, never `POST /command`.
- Staging must run a build with the same protocol version as the client. A mismatch fails `staging-connect` at `staging-join`. The agent does not deploy staging.

### Runtime path (local)

Packaged binaries only, from the one downloaded artifact. No compose, no images, no source checkout: the server, master and client all come from the same build. Compose and images stay covered by `containers.yml` and by staging.

### Files

- `tools/playtest/agent.sh`: the agent.
- `tools/playtest/com.roboviolence2.playtest.plist`: LaunchAgent, `StartInterval` 600 s.
- `tools/playtest/README.md`: install, env file, preflight, troubleshooting.
- `docs/decisions/00NN-playtest-pull-agent.md`: pull agent instead of a runner; main-only; per-build tests on a disposable local server; staging read-only; admin API in plaintext on loopback.

The owner installs the agent by copying it to `~/RV2-playtest/bin/`, so the agent runs no repo checkout.

### Environment

- `~/.config/rv2-playtest.env`, mode 600 (the agent refuses other modes). Loaded with `set -a; . file; set +a`. Never written to reports. Keys:
  - `BV2_ADMIN_PASS`: random, used only by the disposable local server;
  - `NTFY_TOPIC`;
  - `RV2_STAGING_HOST`, `RV2_STAGING_ADMIN_PASS`: optional; without them the staging tests report `not-configured`. The host never goes into the repo.
- Plist `EnvironmentVariables`: `PATH=/opt/homebrew/bin:/usr/local/bin:/usr/bin:/bin`, `HOME`.
- `gh` reads its token from the login keychain. Access from a LaunchAgent depends on the keychain being unlocked and its access controls, so it is not assumed (see Preflight).
- No join password on either target in T1: `connect` takes the password as an argument, which would be visible in `ps`. Join-password checks move to the local T4 scenarios.
- Client config: `$HOME/RV2-playtest/pref/<sha7>/playtest.cfg`, always passed as an **absolute** path (`run-client.sh` changes to the package dir first). It sets `r_fullScreen false` and `cl_playerName rv2-agent-<sha7>`; the name identifies the agent's player in `/status`, also on staging.
- Plist `StandardOutPath`/`StandardErrorPath`: `~/RV2-playtest/agent.log`.

### Preflight

`agent.sh --preflight`, run once through the LaunchAgent itself (`launchctl kickstart gui/$UID/com.roboviolence2.playtest` with a preflight flag in the env file) in the owner's logged-in session:
- `gh auth status` for Hub-Tansky;
- a read-only `gh api repos/Hub-Tansky/RoboViolence2/actions/workflows/build.yml/runs?per_page=1`;
- env file mode 600 and required keys;
- `curl`, `ditto`, `shasum` on PATH.

The result is reported through the normal notification. The agent is enabled for unattended runs only after preflight passes. Every run also starts with `gh auth status`; if it fails, the run fails as `gh-auth`.

### Layout

```
~/RV2-playtest/
  bin/        agent copy
  versions/<yyyymmdd>-<sha7>/   extracted package (replaced only by a new download)
  pref/<sha7>/                  BV2_PREF_DIR and playtest.cfg (outside the package)
  run/<sha7>-<target>-<time>/   disposable local server: master.db/web.db copies, server pref, logs, FIFO
  reports/<sha7>.txt            one report per attempt, appended
  state.tsv                     sha, run id, test, target, result, failed stage, attempts, time
  current -> versions/…         last version that passed `local-smoke`
  lock/                         mkdir lock + pid
```

### Tests

| Test | Target | Trigger |
|---|---|---|
| `local-smoke` | `local` | every new `main` build (unattended) |
| `staging-connect` | `staging` | `agent.sh --test staging-connect [--sha <sha>]` (on demand) |

`--play local|staging` starts the same setup for manual testing and leaves it running until `agent.sh --stop`. It is not an automated test and writes no pass/fail.

### Flow (`local-smoke`)

1. **Lock.** `mkdir ~/RV2-playtest/lock`. If it exists and its pid is alive, exit silently; if the pid is dead, remove it and continue.
2. **Auth.** `gh auth status`; failure → stage `gh-auth`.
3. **Select.** `gh api repos/Hub-Tansky/RoboViolence2/actions/workflows/build.yml/runs?branch=main&event=push&status=success&per_page=1`. Accept only `event == push`, `head_branch == main`, `head_repository.full_name == Hub-Tansky/RoboViolence2`; this excludes fork PRs from a branch named `main`. Skip if the SHA is `pass` in `state.tsv`, or `fail` with 3 attempts.
4. **Download and verify.** `gh run download <id> -n roboviolence2-macos-arm64.zip` into a new empty dir. Compare its `shasum -a 256` with the artifact's `digest` from `GET …/runs/<id>/artifacts`, after stripping the `sha256:` prefix.
5. **Install.** `ditto -x -k` into `versions/<…>/` (fresh dir), `xattr -dr com.apple.quarantine`. Keep the last 5 version, pref, run and report sets.
6. **Start the disposable server.** Create `run/<…>/`; copy `master.db` and `web.db` there. Then:
   - master from the copies (TCP 10207);
   - `run-server.sh` with `BV2_PREF_DIR=run/<…>/pref`, `BV2_MASTER_SERVERS=127.0.0.1:10207`, `BV2_ADMIN_API_PORT=18080` (default bind 127.0.0.1), stdin from a FIFO the agent holds open (`std::cin` loop, `game/src/main.cpp:385`).
   - `run-client.sh --config "$HOME/RV2-playtest/pref/<sha7>/playtest.cfg" connect 127.0.0.1` with `BV2_PREF_DIR=$HOME/RV2-playtest/pref/<sha7>`.
7. **Check** (each with a timeout; the first failure names the stage):
   - server log contains `Server Created on port 3333` (30 s);
   - master accepts TCP on 10207 (10 s);
   - client process alive after 10 s;
   - `curl -H "Authorization: Bearer $BV2_ADMIN_PASS" http://127.0.0.1:18080/status` shows `players 1` and `rv2-agent-<sha7>` (30 s).
8. **Tear down.** Stop the client, write `quit` to the server FIFO, stop the master; kill whatever remains after 10 s. On pass, point `current` at the version.
9. **Report and notify** (always, also from a `trap` on any agent error):
   - append to `reports/<sha7>.txt`: run URL, SHA, test, target, each stage with result and time, the last 50 lines of server, master and client logs;
   - update `state.tsv` (attempts +1);
   - ntfy + desktop notification: `RV2 <sha7> READY TO TEST` or `RV2 <sha7> FAILED at <stage>`, with the report path.
10. **Unlock.**

A failed SHA is retried on the next tick, up to 3 attempts. `agent.sh --retry <sha>` resets its attempts; the SHA must still pass step 3's checks.

If no GUI session is available (user logged out), the client stage fails as `no-gui-session` and is reported, not skipped silently.

### Flow (`staging-connect`)

Steps 1–5 as above for the requested SHA (default: `current`). Then:
1. Client with `connect $RV2_STAGING_HOST` (default port 3333).
2. `curl -H "Authorization: Bearer $RV2_STAGING_ADMIN_PASS" https://$RV2_STAGING_HOST:8443/status` lists `rv2-agent-<sha7>` within 30 s; failure → stage `staging-join`.
3. Stop the client, which leaves the staging server. Report and notify as above, with target `staging`.

### Done when

- For each successful `main` build, the agent records the SHA and sends a `local-smoke` pass or failure report within 20 minutes of the build ending. It reports "ready to test" only when the client is connected to the disposable local server and `/status` confirms its player.
- After every `local-smoke`, no agent-started process is left running.
- A deliberately broken install (corrupted zip) and a stopped master both produce a failure notification naming the stage.
- Preflight has passed in the LaunchAgent. `staging-connect` passes against a staging server on the same build, or reports `not-configured` without staging keys.

## Candidate follow-up steps (not yet approved)

### T2: Packaged client smoke on Windows and macOS CI

- First a prototype PR: can a GitHub-hosted macOS runner and a Windows runner (Mesa llvmpipe `opengl32.dll`, pinned mesa-dist-win release, documented source and hash) run the packaged client for 10 s? Gate on it only after 10 consecutive green runs.
- Also start the packaged server and master on macOS and Windows (today only Linux, in `package-check`).

### T3: Client autotest mode (game code)

- `--autotest <script>`: console commands with waits and assertions (`expect-console <text>`, `expect-scene <name>`).
- Pass: the script reaches its end and every assertion holds, within `--timeout <s>` (default 60). Exit 0 on pass, 1 on a failed assertion, 2 on timeout. Log lines alone never fail a run.
- In autotest mode the client console goes to stdout, so assertions and CI can read it.
- Runs under Xvfb on Linux CI, natively on Mac.

### T4: E2E scenarios with bots

Runner in `tests/e2e/`: the packaged server + N autotest clients (Xvfb on CI). Each scenario has one observable:

| Scenario | Observable |
|---|---|
| Join / leave | `/status` `players N` |
| Join password (right / wrong) | `/status` players 1 / 0; client `expect-console` on the refusal |
| Chat | server stdout has the message (`game/src/ServerRecv.cpp:540` logs it) |
| CTF capture | `/status` `score blue … red …` increments |
| Kick / ban | `POST /command kick <id>` → players 0; banned client's rejoin keeps players 0 |
| Master list | client autotest refreshes the browser and expects `Games found : 1` (`game/src/CMaster.cpp:720`) |

Each later feature step adds a scenario to its DONE gate ([REVIEW.md](../../REVIEW.md)).

### T5: Windows VM on Mac

VMware Fusion + Windows 11 ARM. First check that OpenGL renders the game; if not, drop T5 and use T6. Then `agent.ps1` with the same rules as T1 (main only, verify, report every SHA), Task Scheduler instead of `launchd`.

### T6: Workstation

Windows 11 + WSL2. Weekly `agent.ps1` run on the real GPU; the Linux package under WSLg; a 2-machine LAN game with the Mac. Native Linux only if WSLg results differ from CI.

### T7: Screenshot regression

Golden images for static screens (main menu, options, scoreboard) with a pixel tolerance, taken by T3. Gameplay comparison waits for the seeded RNG (PNS-12).

### T8: Staging host and pull deploy agent

Prerequisite for T1's `staging-connect` and `--play staging`. Independent of T2–T7.

**Definition:** a long-lived staging server that runs the newest `main` server and master images, updated by a pull agent on the staging host itself. It never restarts during a game and always reports which SHA it runs.

**Host** (choice and details private):
- the workstation or another home machine (port forward needed); or
- a free cloud VM (e.g. Oracle Always Free, ARM). An ARM host needs arm64 images pushed: today `containers.yml` builds arm64 only as a check.

**Files:**
- `tools/staging/deploy.sh`: the deploy agent (Linux, `systemd` timer every 10 min, or cron).
- `tools/staging/README.md`: host setup, env file, rollback.
- `packaging/docker/compose.example.yml`: image tag from `${BV2_IMAGE_TAG:-latest}` instead of a fixed `:latest`.
- `packaging/docker/Caddyfile`: `GET /deployed` serves the deployed-SHA file (no auth; it holds only the SHA and time).
- `.github/workflows/containers.yml`: push arm64 images (only for an ARM host).
- ADR: staging deploys by pull, pinned SHA, only when empty.

**Environment on the host:**
- `/etc/rv2-staging.env`, mode 600: `BV2_ADMIN_PASS`, `NTFY_TOPIC`, `BV2_ADMIN_HOST`.
- No GitHub token: the repo, its Actions API and the GHCR images are public. Unauthenticated API use (6 calls/h) is far below the 60/h limit.
- No join password (see T1).

**Flow:**
1. **Lock** (as T1).
2. **Select.** Newest `containers.yml` run with `event == push`, `head_branch == main`, `head_repository.full_name == Hub-Tansky/RoboViolence2`, `conclusion == success`. Skip if its SHA is the deployed one, or failed 3 times.
3. **Fetch.** `compose.example.yml` and `Caddyfile` from `raw.githubusercontent.com/Hub-Tansky/RoboViolence2/<sha>/packaging/docker/`, so the files match the images. `docker pull` both images at `:<sha>`; record their digests.
4. **Wait for an empty server.** `GET /status` through Caddy shows `players 0`; otherwise retry on the next tick. After 6 h of waiting, notify once (`staging deploy waiting: N players`). `deploy.sh --force` skips the wait.
5. **Deploy.** `BV2_IMAGE_TAG=<sha> docker compose up -d`.
6. **Verify** (2 min): both containers `healthy`; `GET /status` answers 200.
7. **Record or roll back.** Pass: write `deployed` (SHA, image digests, time), served at `/deployed`. Fail: redeploy the previous SHA, verify it, mark the new SHA failed.
8. **Notify** (always, also from a `trap`): ntfy `staging now on <sha7>`, `staging deploy <sha7> FAILED at <stage>, rolled back to <sha7>`, or the waiting message.

**T1 link:** `staging-connect` first reads `https://<staging>:8443/deployed`. If the staging SHA differs from the client's, the report says so; a join failure then names `staging-version-mismatch` instead of `staging-join`.

**Done when:**
- A new `main` build runs on staging within 30 minutes of its `containers` run when no players are connected.
- With a player connected, no deploy happens until the server is empty or `--force` is used.
- A deliberately broken deploy (bad tag) rolls back to the previous SHA and sends a failure notification naming the stage.
- `/deployed` shows the running SHA, and T1's `staging-connect` passes against it.

## Open decisions for the owner

- Approve T1 as a step and choose its phase.
- ADR content: pull agent, main-only, per-build tests on a disposable local server, staging read-only, plaintext admin API on loopback (ADR 0015 covers network exposure; loopback on the owner's Mac adds none).
- Staging host for T8: home machine or free ARM VM (the second needs arm64 images). Until T8 is done, `staging-connect` reports `not-configured`.
- Whether to publish arm64 server images (only matters if staging runs on an ARM host).
