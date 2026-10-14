# Step 3: Unsigned installers

**Status:** IN PROGRESS

**Depends on:** [step 1](step1-player-data-dir.md), [step 2](step2-name-and-credits.md). **Next:** step 5 (after PNS-31). **Index:** [README.md](README.md)

## Scope

| Field | Value |
|---|---|
| Goal | CI builds a Windows `.exe` installer and a macOS `.dmg` on every run, and a `v*` tag publishes them with the Linux package and a source tarball as a GitHub release |
| In scope | Tasks 3.1–3.5 below (PNS-16, PNS-23, PNS-26, PNS-27) |
| Out of scope | Signing and notarisation (PNS-49); Flatpak (PNS-52); stores (PNS-50); renaming `bv2.app` or the binaries (PNS-20); creating the first release tag (owner) |
| Allowed paths | `packaging/**`, `triplets/**` (new), `CMakePresets.json`, `.github/workflows/build.yml`, `docs/build/**`, `docs/decisions/**`, `README.md`, `AGENTS.md`, `ARCHITECTURE.md`, `docs/roadmap/**` |
| Inputs | `AGENTS.md`, `ARCHITECTURE.md`, PNS-16, PNS-23, PNS-26, PNS-27, PNS-20 (engine-only release decision), [ADR 0012](../../decisions/0012-windows-10-22h2-minimum.md), [ADR 0016](../../decisions/0016-player-data-lookup-and-package-content.md) |
| Deliverables | Inno Setup script; `.dmg` creation; licence notices in every package; release job; overlay triplet; ADR 0020; install guides |
| Definition of done | See "Acceptance checks". All must pass. `ARCHITECTURE.md` is updated |

## Context

- `packaging/make-package.py` stages one folder (client, servers, `main/`, databases, run scripts) and archives it as `.zip` (Windows, macOS) or `.tar.gz` (Linux); `build.yml` uploads it per OS.
- The Windows runner has Inno Setup 6 (`ISCC.exe`); macOS has `hdiutil`.
- vcpkg installs each port's licence as `vcpkg_installed/<triplet>/share/<port>/copyright`.
- macOS ports are built for the build machine's macOS, not 12.0 (PNS-26); `windows-latest` can move (PNS-27).

## Tasks

### 3.1 Licence notices

- `make-package.py` copies `LICENSE.txt` and every port's `copyright` (as `licenses/<port>.txt`) into the package.

### 3.2 Windows installer

- `packaging/windows/roboviolence2.iss`: per-user install (no admin) of the package folder into `{autopf}\Robo Violence 2`, Start-menu and optional desktop shortcut to `bv2.exe`, uninstaller. `make-package.py` runs `ISCC` on Windows and writes `roboviolence2-<preset>-setup.exe`.

### 3.3 macOS disk image

- `make-package.py` writes `roboviolence2-<preset>.dmg` (`hdiutil`, UDZO) with the package folder and an `Applications` link. The ad-hoc signature stays.

### 3.4 CI and release

- `build.yml` uploads the installer and the `.dmg` beside the archives; pins `windows-2025` (PNS-27).
- On a `v*` tag, a `release` job creates the GitHub release with the Windows installer, the `.dmg`, the Linux `.tar.gz` and `git archive` source tarball.
- Overlay triplet `triplets/arm64-osx.cmake` builds the macOS ports for 12.0 (PNS-26).

### 3.5 Docs and ADR

- ADR 0020: package and release format (PNS-23) and engine-only releases (PNS-20 decision).
- `docs/build/`: install from a release per OS, with the SmartScreen and Gatekeeper steps.

## Critical files

- `packaging/make-package.py`, `packaging/windows/roboviolence2.iss`, `.github/workflows/build.yml`, `triplets/arm64-osx.cmake`

## Acceptance checks

- CI artifacts include `roboviolence2-win-x64-msvc-setup.exe` and `roboviolence2-macos-arm64.dmg`; each package holds `licenses/`.
- `make-package.py` on macOS writes a `.dmg` that mounts and starts the game.
- The macOS build links without "built for newer 'macOS' version" warnings.
- `tools/review.sh --full <preset>` passes, and the fresh-context review record `reviews/step3.md` and the raw reviewer report `reviews/step3-report.md` (with the skill's load line) exist (`REVIEW.md` section 5).
- `ARCHITECTURE.md` lists every file added, moved or removed in this step.
- The secret scan (`gitleaks git --staged` / CI) is clean.
