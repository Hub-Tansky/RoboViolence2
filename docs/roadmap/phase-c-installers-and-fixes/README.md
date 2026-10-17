# Phase C: installers and fixes

**Status:** IN PROGRESS (steps 1–7 DONE 2026-10-17; release and manual test open)

**Depends on:** Phase B ([README](../phase-b-security-infrastructure-anti-cheat/README.md)). Steps 1–4 can start before Phase B's manual test (PNS-31); steps 5–7 change game, server or engine behaviour and wait for it as the regression baseline. **Rules:** [roadmap README](../README.md) (ground rules, step closing).

## Goal

Friends install the game from an unsigned Windows 10/11 installer or macOS `.dmg`, or build it from a source release, and point it at their own BaboViolent 2 data without setting an environment variable. Phase B leftovers are closed. The game shows its own name and an accurate fork credit. Gameplay and the BV2 interface are otherwise unchanged, except the approved PNS-30 default ([ADR 0010](../../decisions/0010-bv2-asset-compat-and-gui-freeze.md)).

Releases are engine-only: code and generated placeholders; the player supplies the BV2 data. The owner allows them before replacement assets ship (PNS-20, 2026-10-14).

## Steps and dependencies

Entries are in [possible-new-scope.md](../possible-new-scope.md). Scope files are written from [_template-step.md](../_template-step.md) as each step starts.

| # | Step                            | PNS                                                  | Depends on    | Outcome                                                                                                                                                                                                                                                                                                                                                          |
| - | ------------------------------- | ---------------------------------------------------- | ------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 1 | Player data outside the package | PNS-51                                               | Phase B steps | Assets load from the player's BV2 folder; launch scripts and language files from the package. Lookup: `BV2_DATA_DIR`, saved path, auto-detect, then a first-run OS folder picker (**[GUI]**, owner-approved 2026-10-14) that saves the path. ADR                                                                                                                 |
| 2 | In-game name and credits        | PNS-53                                               | 1             | **[GUI]**, owner-approved 2026-10-14: window title and server name say Robo Violence 2; Credits states the unofficial fork of BaboViolent 2 by bitHeads / RndLabs, thanks them, then the original credits; "Loading" splash replaces the RndLabs logo; forum text, survey and `lang_RndLabsTeam` removed. ADR amending 0008 and 0010; source notices unchanged   |
| 3 | Unsigned installers             | PNS-16, PNS-23, PNS-26, PNS-27                       | 1, 2          | CI builds a Windows `.exe` installer, a macOS `.dmg` and a source tarball and attaches them, with the Linux `.tar.gz` package, to GitHub releases. ADR for the package format (PNS-23) and the engine-only release (PNS-20 decision). macOS minimum version fixed or documented (PNS-26); Windows runner pinned (PNS-27); licence notices for bundled libraries. |
| 4 | Build, CI and docs housekeeping | PNS-22, PNS-24, PNS-33, PNS-36, PNS-45               | Phase B steps | SBOM with upstream purls; runtime `bv2.db` removed; `bv2dedicated` built from `bv2server_lib`; docs no longer describe R3 as current; `containers` in the CI list                                                                                                                                                                                                |
| 5 | Client and engine fixes         | PNS-30, PNS-34, PNS-38 (R29), PNS-47                 | PNS-31        | **[GUI]** `r_highDetailMenu` defaults to `false` (owner-approved 2026-10-14); `CString` format constructor `explicit`; R29 fixed; old saved `languageFile` default no longer pins English                                                                                                                                                                        |
| 6 | Server and master               | PNS-42, PNS-43 (master ban password), PNS-44, PNS-46 | PNS-31        | Admin attempt budget and minimum `zsv_adminPass` length; master bans authorised without the hard-coded password; advertised server address; legacy UDP remote admin removed                                                                                                                                                                                      |
| 7 | Windows TCP-close crash         | PNS-38 (R28)                                         | PNS-31        | R28 fixed; the master test runs on Windows again                                                                                                                                                                                                                                                                                                                 |

Order: 1 → 2 → 3; 4 alongside. Steps 5–7 one at a time after PNS-31. Every step closes per [REVIEW.md](../../../REVIEW.md) section 5.

## Out of Phase C

| Item | Where |
|---|---|
| Sim core (PNS-12), networking rework (PNS-17), join order and relayed IPs (PNS-43), lag-stall tuning (PNS-41) | Phase D |
| Flatpak on Flathub (PNS-52) | Phase D, after PNS-20 |
| Replacement assets and the rest of the rebrand (PNS-20: binaries, `BV2_*`, pref paths, master game ID) | Deferred |
| Signing and notarisation (PNS-49); Steam and itch.io (PNS-50) | Deferred |
| AppImage, `.deb` | Dropped (PNS-16) |
| Other **[GUI]** items (PNS-25, PNS-32, PNS-48), renderer (PNS-18), accounts (PNS-14, PNS-15), advanced anti-cheat (PNS-13) | Later phases |

## Phase C done when

- Every step is DONE; CI is green.
- A GitHub release holds the Windows installer, the macOS `.dmg`, the Linux package and a source tarball. A new player installs each, points it at their BV2 data and joins a game without setting an environment variable.
- The [manual test](../phase-a-modern-portable-build/phase-a-manual-test.md) passes on all three OSes after the last step.

## Risks

| Risk                                                                                                                   | Mitigation                                                                                       |
| ---------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------ |
| Unsigned installers trigger SmartScreen and Gatekeeper (macOS 15+: System Settings → Privacy & Security → Open Anyway) | Install guide per OS; signing is PNS-49                                                          |
| The player's BV2 data differs between copies, so maps and textures mismatch                                            | Map download already sends missing `.bvm`; document which BV2 release to use                     |
| Layering two data roots changes how every file is opened                                                               | Step 1 keeps one lookup function; test with original data and with placeholders                  |
| The player's `en.lang` would override the rebranded name                                                               | Step 2 depends on step 1: language files load from the package                                   |
| macOS packages may need macOS 14+ (PNS-26)                                                                             | Step 3 sets the vcpkg deployment target or documents the minimum                                 |
| GPLv3 and bundled library licences (SDL3, miniaudio, libsodium, sqlite, stb)                                           | Source tarball per release; notices in every installer                                           |
| `explicit` `CString` touches many call sites                                                                           | Step 5 compiles every target on all three OSes; mechanical call-site changes in their own commit |
| R28 needs Windows debugging                                                                                            | Crash dump from the owner's PC, or a Windows CI job with a debugger                              |
