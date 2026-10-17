# Step 3: Packaged client, server and master start in CI on Windows and macOS

**Status:** IN PROGRESS

**Depends on:** Phase B step 2b (release packages), Phase C step 3 (installers; artifact names). **Next:** T3 client autotest mode (candidate). **Index:** [README.md](README.md)

## Scope

| Field | Value |
|---|---|
| Goal | Every CI run starts the packaged client for 10 s and the packaged server and master on Windows and macOS, as `package-check` already does on Linux |
| In scope | Tasks 3.1 … 3.3 below; plan item T2 ([plan](../20261014_testing_environment_plan.md#t2-packaged-client-smoke-on-windows-and-macos-ci)) |
| Out of scope | Making the new job required (owner, after 10 consecutive green runs on `main`); client assertions beyond "started and alive" (T3); installer and `.dmg` tests (Phase C owns them in `build`); any game or packaging change |
| Allowed paths | `.github/workflows/build.yml`, `AGENTS.md`, `docs/roadmap/**`, `ARCHITECTURE.md` |
| Inputs | `AGENTS.md`, `ARCHITECTURE.md`, the plan, [step 1 review](reviews/step1.md), `.github/workflows/build.yml` (`package-check`, `smoke`), `packaging/make-package.py`, `packaging/scripts/` |
| Deliverables | `build.yml` job `package-smoke` (Windows, macOS), not required |
| Definition of done | See "Acceptance checks". All must pass. `ARCHITECTURE.md` is updated |

## Context

- The Windows and macOS packages are built and uploaded, but no CI job starts their client; only the Linux ASan build's client runs (`smoke`), and only the Linux package's server and master start (`package-check`).
- GitHub-hosted Windows runners have no OpenGL driver. Mesa llvmpipe from [mesa-dist-win](https://github.com/pal1000/mesa-dist-win) (pinned release and SHA-256) next to `bv2.exe` provides one; Windows loads `opengl32.dll` from the application folder first.
- An init failure shows a modal message box (`game/src/main.cpp:464`, `:489`), so a client still alive after 10 s is not proof it started. The console, which creates `console.log` in the pref dir, is built only after window, input, GL, audio and network init succeed (`game/src/main.cpp:558`). That file is the start signal.
- Release packages start fullscreen. The job passes `--config` with `r_fullScreen false`, as runners have no real display mode to switch.

## Tasks

### 3.1 Scope

- This file; row in the phase README.

### 3.2 Job

- `build.yml` job `package-smoke`, matrix Windows and macOS, `needs: build`: download the zip, extract it, start the server through its run script with `quit` on stdin and expect `Server Created on port 3333`, start the master through its run script and expect TCP 10207, start the client with `--placeholders` for 10 s and expect it alive with `console.log` written.
- Windows: Mesa llvmpipe as above, `GALLIUM_DRIVER=llvmpipe`.

### 3.3 Docs and inventory

- `AGENTS.md` CI list, `ARCHITECTURE.md` (`build.yml` row, this file).

## Critical files

- `.github/workflows/build.yml`

## Acceptance checks

- CI `package-smoke (win-x64-msvc)` and `package-smoke (macos-arm64)` pass on the PR.
- `tools/review.sh --step docs/roadmap/phase-t-testing-environment/step3-ci-package-smoke.md` passes.
- `tools/review.sh --full <preset>` passes, and the fresh-context review record `reviews/step3.md` and the raw reviewer report `reviews/step3-report.md` (with the skill's load line) exist (`REVIEW.md` section 5).
- `ARCHITECTURE.md` lists every file added in this step.
- The secret scan (`gitleaks git --staged` / CI) is clean.
