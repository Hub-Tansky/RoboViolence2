# 0020: Package and release format; engine-only public releases

- Status: Accepted
- Date: 2026-10-14
- Affects: [Phase C step 3](https://github.com/Hub-Tansky/RoboViolence2/blob/58c91b2a660db2b290b0b5f504381980a6aedba2/docs/roadmap/phase-c-installers-and-fixes/step3-unsigned-installers.md); PNS-16, PNS-20, PNS-23

## Context
- Phase B step 1 introduced per-OS play-test packages (`packaging/make-package.py`) without an ADR (PNS-23).
- Friends need installers; signing costs money and accounts (PNS-49). Original assets can't be shipped; the client asks for the player's copy (ADR 0016).

## Decision
- **Package** (every CI run, per OS): one folder with the client, `bv2dedicated`, `bv2master`, `main/` (placeholders, launch scripts, languages), `master.db`, `web.db`, run scripts (`.cmd` on Windows: they run without PowerShell's execution policy; `.sh` elsewhere), `LICENSE.txt` and `licenses/` (each vcpkg port's licence and `glad.txt`). Archived as `.zip` (Windows, macOS; `ditto` keeps the bundle signature) or `.tar.gz` (Linux, keeps file modes), uploaded unarchived.
- **macOS**: `bv2.app/Contents/Resources/main` is a real copy, so the bundle works when dragged alone; the folder keeps its own `main/` for the servers. Ad-hoc signed, because arm64 binaries must carry a signature. Also a `.dmg` (UDZO) with an `Applications` link; since ADR 0030 it holds only the app, and the release also publishes the macOS `.zip` for the servers. Ports build for macOS 12 (`triplets/arm64-osx.cmake`).
- **Windows**: also an Inno Setup installer (`packaging/windows/roboviolence2.iss`): per-user install without admin (the folder stays writable for the servers), Start-menu shortcut, uninstaller, fixed `AppId`. The GPL is shown, not accepted (GPLv3 §9). The master's `master.db` and `web.db` are installed only if missing and never uninstalled, so bans and accounts survive upgrades. CI installs, upgrades and uninstalls it silently. CI runs on `windows-2025` (pinned, ADR 0012's image).
- **Release**: a `v*` tag runs the same build and a `release` job publishes (as a pre-release for `v0.*` and `v*-*` tags) the Windows installer, the `.dmg`, the Linux `.tar.gz` and a `git archive` source tarball (GPLv3).
- **Engine-only releases are public** (owner, 2026-10-14): code and generated placeholders only; no original asset is shipped. Flathub and stores wait for PNS-20.

## Consequences
- Installers are unsigned: SmartScreen and Gatekeeper warn; the guides explain both (PNS-49 removes the warnings).
- Changing the Inno `AppId` breaks upgrades and uninstall of existing installs.
- A schema change in `content-seed/master.sql` or `web.sql` never reaches an existing Windows install: it needs a migration in `bv2master`.
- Uninstalling leaves `master.db` and `web.db` in the install folder.
- Rejected: AppImage and `.deb` (Flatpak covers Linux later, PNS-52); WiX/MSI (more setup for the same result); a zipped artifact around the zip.
