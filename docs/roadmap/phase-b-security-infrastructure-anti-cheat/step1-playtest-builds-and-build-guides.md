# Step 1: Play-test builds and build guides

**Status:** IN PROGRESS

**Depends on:** [Step 0](step0-supply-chain-security.md). **Next:** [step2-cross-os-playtest.md](step2-cross-os-playtest.md). **Index:** [README.md](README.md)

## Scope

| Field | Value |
|---|---|
| Goal | Every CI run publishes a play-test package per OS (client, `bv2dedicated`, `bv2master`, `main/`), and `docs/build/` explains how to build and run on macOS, Windows and Linux |
| In scope | Tasks 1.1–1.5 below |
| Out of scope | Signed installers, notarisation, AppImage/Flatpak/.deb (PNS-16); fixing gameplay bugs found while testing (step 2); renaming binaries (§H, PNS-20) |
| Allowed paths | `.github/workflows/build.yml`, `packaging/**`, `tools/setup-dev.*`, `docs/build/**`, `README.md`, `AGENTS.md`, `ARCHITECTURE.md`, `docs/roadmap/**`, `CMakeLists.txt` (install rules only), `.gitignore` (owner-approved 2026-10-07: anchor `build/` so `docs/build/` is tracked) |
| Inputs | `AGENTS.md`, `ARCHITECTURE.md`, earlier review records `reviews/step*.md`, `README.md` "Platforms and build", `tools/setup-dev.sh`, `.github/workflows/build.yml`, [ADR 0005](../../decisions/0005-runtime-main-data-root.md), [ADR 0007](../../decisions/0007-data-root-pref-dir-config-layers.md) |
| Deliverables | Artifacts `roboviolence2-<preset>` per OS; `docs/build/README.md`, `macos.md`, `linux.md`, `windows.md`; run scripts |
| Definition of done | See "Acceptance checks". All must pass. `ARCHITECTURE.md` is updated |

## Context

- `.github/workflows/build.yml:55–75` packages only `bv2dedicated.exe` on Windows; `bv2dedicated` and `bv2.app` on macOS; `bv2`, `bv2dedicated` and `main/` on Linux. `bv2master` is never shipped, and the Windows client is missing.
- `README.md` "Platforms and build" holds the only build instructions, a few lines per OS.
- The client links system GL and GLU (PNS-3), so a clean Linux install may need runtime packages.
- Downloaded unsigned builds are quarantined by macOS Gatekeeper and flagged by Windows SmartScreen.

## Tasks

### 1.1 Package all executables per OS

- Each package contains the client, `bv2dedicated`, `bv2master` and `main/`: Linux `tar.gz`, macOS zip (`bv2.app` plus servers), Windows zip (`bv2.exe` plus servers).
- Name artifacts `roboviolence2-<preset>`; the binaries keep their `bv2*` names (ADR 0009).

### 1.2 Run scripts

- `run-server` and `run-master` scripts (`.sh`, `.ps1`) start the servers from the package directory and pass `BV2_DATA_DIR` through.

### 1.3 Build guides

- `docs/build/README.md`: which guide to read, CI artifacts vs building, what `BV2_DATA_DIR` does, and that placeholders are used without it.
- `macos.md`: Homebrew packages, cloning and bootstrapping vcpkg, `VCPKG_ROOT`, `tools/setup-dev.sh`, the `macos-arm64` preset, the `runtime/` output (`bv2.app` and its `Contents/Resources/main` symlink), running the client (`open`, or the binary for console output), running the servers (`execute CTF`), `BV2_DATA_DIR` three ways (shell `export`, `launchctl setenv`, replacing the symlink), `xattr -dr com.apple.quarantine` for downloaded zips.
- `linux.md`: the `tools/setup-dev.sh --linux-packages` apt line, the `linux-x64` preset, X11 vs Wayland, runtime packages for the CI bundle (GL, GLU), `BV2_DATA_DIR` via `~/.profile`.
- `windows.md`: VS Build Tools (C++), CMake, Ninja, vcpkg, a Developer PowerShell, the `win-x64-msvc` preset, `setx BV2_DATA_DIR`, SmartScreen "Run anyway".
- Every guide: running the tests (`ctest`), connecting a client to a local server, where logs and config go (pref dir), and troubleshooting (black window, missing language file, missing vcpkg).

### 1.4 Point to the guides

- `README.md` "Platforms and build" becomes a short paragraph linking `docs/build/`. `AGENTS.md` Build links the guides.

### 1.5 Verify the Linux bundle on a clean system

- Unpack the CI Linux artifact in a clean `ubuntu:24.04` container. Install only the packages `linux.md` lists, then start `bv2dedicated` and `bv2master`.

## Critical files

- `.github/workflows/build.yml`
- `README.md`, `tools/setup-dev.sh`, `tools/setup-dev.ps1`

## Acceptance checks

- Each of the three CI artifacts contains the client, `bv2dedicated`, `bv2master` and `main/`.
- In a clean `ubuntu:24.04` container, the servers from the Linux artifact start with only the documented packages installed:

```bash
./run-server.sh   # prints the server console prompt; `quit` exits 0
```

- The owner follows `docs/build/macos.md` from scratch and gets a running client and server.
- The Markdown link check finds no broken link in `docs/build/`.
- `tools/review.sh --full <preset>` passes, and the fresh-context review record `reviews/step1.md` exists ([REVIEW.md](../../../REVIEW.md) section 5).
- `ARCHITECTURE.md` lists every file added, moved or removed in this step.
- The secret scan (`gitleaks git --staged` / CI) is clean.
