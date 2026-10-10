# Step 2b: Release play-test packages

**Status:** IN PROGRESS

**Depends on:** [Step 2a](step2a-windows-10-target.md). **Next:** [step3-test-harness.md](step3-test-harness.md). **Index:** [README.md](README.md)

## Scope

| Field              | Value                                                                                                                                                                                      |
| ------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| Goal               | CI builds the play-test packages in Release, so they start fullscreen, carry no debug C++ runtime, and run at full speed; development presets stay Debug                                   |
| In scope           | Tasks 2b.1–2b.3 below                                                                                                                                                                      |
| Out of scope       | Changing the Debug development presets; code signing (PNS-16); a startup OpenGL version check (owner: no code change); Debug packages in CI (owner, 2026-10-10: Release only; shareable debug builds are PNS-29)                                                                      |
| Allowed paths      | `CMakePresets.json`, `.github/workflows/build.yml`, `packaging/**`, `tests/**`, `docs/build/**`, `docs/decisions/**`, `docs/roadmap/**`, `ARCHITECTURE.md`, `AGENTS.md`, `tools/review.sh` (owner-approved 2026-10-10: accept padded table rows)                    |
| Inputs             | `AGENTS.md`, `ARCHITECTURE.md`, earlier review records `reviews/step*.md`, `CMakePresets.json`, `.github/workflows/build.yml`, [ADR 0012](../../decisions/0012-windows-10-22h2-minimum.md) |
| Deliverables       | Release build of each package in CI; packages built from it; docs and inventory updated                                                                                                    |
| Definition of done | See "Acceptance checks". All must pass. `ARCHITECTURE.md` is updated                                                                                                                       |

## Context

- Every preset builds Debug (`CMakePresets.json:16`), and CI packages that build.
- In Debug, `r_fullScreen` defaults to false (`game/src/GameVar.cpp:642–646`), so packages start windowed; Release starts fullscreen (PNS-28).
- Debug Windows builds use the debug C++ runtime (`MSVCP140D`, `ucrtbased`), which Microsoft doesn't allow redistributing, even linked statically (step 2a round 2, finding 2).

## Tasks

### 2b.1 Release build for packaging

- Add a Release variant per OS preset (configure and build presets), or set `CMAKE_BUILD_TYPE=Release` for the CI packaging build. Keep the Debug build for tests and smoke.

### 2b.2 Package the Release build

- `packaging/make-package.py` packages the Release runtime directory. The `windows_no_crt_dlls` test also covers the Release executables.
- Done in 2b.1 without a change here: the CI build directory is now Release, so `make-package.py` and the build job's ctest (including `windows_no_crt_dlls`) see the Release executables.

### 2b.3 Docs

- `docs/build/README.md`: packages are Release builds and start fullscreen; `AGENTS.md` and `ARCHITECTURE.md` updated.

## Critical files

- `CMakePresets.json`, `.github/workflows/build.yml`, `packaging/make-package.py`

## Acceptance checks

- CI uploads Release packages for all three OSes; `package-check` passes on the Linux one.
- The owner starts the macOS package and it opens fullscreen.
- `tools/review.sh --full <preset>` passes, and the fresh-context review record `reviews/step2b.md` and the raw reviewer report `reviews/step2b-report.md` (with the skill's load line) exist ([REVIEW.md](../../../REVIEW.md) section 5).
- `ARCHITECTURE.md` lists every file added, moved or removed in this step.
- The secret scan (`gitleaks git --staged` / CI) is clean.
