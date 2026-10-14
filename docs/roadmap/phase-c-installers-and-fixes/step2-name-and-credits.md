# Step 2: In-game name and credits

**Status:** IN PROGRESS

**Depends on:** [step 1](step1-player-data-dir.md) (language files load from the package). **Next:** step 3 (unsigned installers). **Index:** [README.md](README.md)

## Scope

| Field | Value |
|---|---|
| Goal | The game calls itself Robo Violence 2, credits the original team as an unofficial fork, shows a "Loading" splash instead of the RndLabs logo, and drops the dead survey |
| In scope | Tasks 2.1–2.4 below (PNS-53; **[GUI]**, owner-approved 2026-10-14) |
| Out of scope | Binaries, `BV2_*`, the `BaboViolent2` pref path, master game ID and the rest of the rebrand (PNS-20); in-game terms ("Babo", "Counter Baborist"); the duplicate client schema (PNS-55); translating the new strings |
| Allowed paths | `game/src/CCredit.cpp`, `game/src/IntroScreen.{h,cpp}`, `game/src/Scene.{h,cpp}`, `game/src/MenuSetup.cpp`, `game/src/CSurvey.{h,cpp}` (removed), `game/src/CMaster.{h,cpp}`, `game/src/CUserLogin.cpp`, `game/src/GameVar.{h,cpp}`, `game/src/main.cpp`, `game/src/Paths.cpp`, `game/CMakeLists.txt`, `content/languages/en.lang`, `content/languages/en.pot`, `config/bv2.example.cfg`, `content-seed/bv2.sql`, `engine/babonet/include/cMSstruct.h`, `masterserver/src/**`, `masterserver/README.md`, `tests/**`, `docs/decisions/**`, `docs/analysis/**`, `ARCHITECTURE.md`, `docs/roadmap/**` |
| Inputs | `AGENTS.md`, `ARCHITECTURE.md`, PNS-53, [ADR 0008](../../decisions/0008-display-name-robo-violence-2.md), [ADR 0010](../../decisions/0010-bv2-asset-compat-and-gui-freeze.md), [ADR 0016](../../decisions/0016-player-data-lookup-and-package-content.md) |
| Deliverables | Renamed strings; new credits header; "Loading" splash; survey removed; ADR 0017 |
| Definition of done | See "Acceptance checks". All must pass. `ARCHITECTURE.md` is updated |

## Context

PNS-53 lists every visible place: `content/languages/en.lang:1` (window title), `game/src/CCredit.cpp:38`, `game/src/GameVar.cpp:372` and `config/bv2.example.cfg:38` (`sv_gameName`), `game/src/main.cpp:300` (server banner), `game/src/CUserLogin.cpp:52` (forum sentence), `game/src/IntroScreen.cpp:31` (RndLabs splash). The survey (`game/src/CSurvey.*`) is never shown: its render call is commented out (`game/src/Scene.cpp:332`).

## Tasks

### 2.1 Name

- `lang_gameName` "Robo Violence 2"; `sv_gameName` default "Robo Violence 2 - Server" (code and example config); the dedicated-server banner. Remove the unused `lang_RndLabsTeam`. Regenerate `en.pot`.

### 2.2 Credits and login note

- Credits, in order: title "Robo Violence 2"; "Unofficial fork of BaboViolent 2 by bitHeads / RndLabs; not endorsed by them."; "Thank all of you for the game!"; header "Original credits below"; the original list unchanged.
- Login note: drop the sentence about the official forum.

### 2.3 Splash

- `IntroScreen` shows "LOADING" in the big menu font (as `Scene`'s loading frame), with the same fade, instead of `RnDLabs.tga`.

### 2.4 Survey removal

- Delete `CSurvey.*` and its hooks in `Scene`, `MenuSetup`, `CMaster`; the `DidSurvey` row in `content-seed/bv2.sql` and `openClientDb`; the master's `SURVEY_*` handling and `SSurvey` in both `cMSstruct.h`. Message IDs 1016–1018 stay unused (no renumbering).

## Critical files

- `game/src/CCredit.cpp`, `game/src/IntroScreen.cpp`, `game/src/Scene.cpp`, `masterserver/src/cNetManager.cpp`

## Acceptance checks

- With original data: window title "Robo Violence 2"; "LOADING" splash instead of the RndLabs logo; Credits show the new header above the original list.
- `grep -rni survey game/src masterserver/src engine` finds nothing.
- `ctest --test-dir build/<preset> --output-on-failure` passes, including `lang_check`.
- `tools/review.sh --full <preset>` passes, and the fresh-context review record `reviews/step2.md` and the raw reviewer report `reviews/step2-report.md` (with the skill's load line) exist (`REVIEW.md` section 5).
- `ARCHITECTURE.md` lists every file added, moved or removed in this step.
- The secret scan (`gitleaks git --staged` / CI) is clean.
