# AGENTS.md: RoboViolence 2

RoboViolence 2, an unofficial fork of BaboViolent 2 ([ADR 0004](docs/decisions/0004-project-name-roboviolence2.md)): GPLv3 C++ top-down multiplayer shooter (bitHeads, 2012). Detailed architecture notes live in `docs/analysis/` (start with `docs/analysis/README.md`).

## Layout

- `BaboViolent2/Code/` — game: client, listen/dedicated server, map editor (one codebase, variants by macro)
- `Engine/babonet/` — networking library (`bb_*` API)
- `Engine/DukZeven/` — engine utilities: `dkc` timer, `dksvar` config vars, `dkw` window, `dkgl`, `dki`, `dkt`, `dkf`, `dkp`, `dks` (FMOD)
- `Engine/dko/` — `.DKO` model loader + ray/sphere intersection
- `MasterServer/Source/` — standalone master server (autotools, SQLite)

## Build

- Linux dedicated server: `make` in the repo root → `BaboViolent2/Content/bv2dedicated` (needs sqlite3, libcurl, OpenSSL, GLU dev packages). See `Engine/DukZeven/Code/readme.md`.
- Windows client: `BaboViolent2/Code/BaboViolent2.sln`; only the `ProDebug` configuration is known to work (`README.txt`).
- There are no automated tests.

## Compile-time variants

- `CONSOLE`: headless dedicated server (no rendering, client, editor or menus). Guard client-only code with `#ifndef CONSOLE`.
- `_PRO_`: "Pro" protocol/ruleset, version 21100 vs 21000. The root Makefile defines it, so Linux servers only accept Pro clients.
- `_DX_`: unfinished Direct3D path; ignore it.

## Git identity

- All commits, merges, pushes and GitHub actions for this project run as **`Hub-Tansky`**: `user.email = 337104978+Hub-Tansky@users.noreply.github.com`. Never `patpi`, never a personal email.
- Run `gh` as `GH_TOKEN=$(gh auth token --user Hub-Tansky) gh …`. Don't run `gh auth switch`; other projects rely on `patpi` being active.
- Setup and guard hook: `docs/refactoring/step0-fork-rename-and-asset-removal.md` §0.0.

## Conventions and gotchas

- Source files use LF line endings. Their encoding is mixed: ASCII, Latin-1/CP1252 (French comments) and UTF-8. Until the scripted UTF-8 conversion in `docs/refactoring/step1-baseline-and-legacy-removal.md` §1.4 lands, don't re-encode files by hand, and use `grep -a` on Latin-1 files.
- Fixed 30 Hz step: every `update(float delay)` gets `delay = 1/30`, and "frames" are a time unit (30 = 1 s).
- Network messages are raw structs in `BaboViolent2/Code/netPacket.h`, `memcpy`'d on the wire. Positions are `short` ×100, velocities `char` ×10. Changing a struct layout requires bumping `GAME_VERSION_SV/CL` in `Server.h`/`Client.h`.
- `playerID` (slot 0–31) is not `babonetID` (connection handle). `bb_serverSend` takes the babonetID; destination 0 means broadcast.
- The server is authoritative for hits, damage, spawns, projectiles and flags. Clients are authoritative for their own movement.
- Server variables (`sv_*`) are registered in `GameVar.cpp` via `dksvarRegister` and pushed to clients.
- Decisions are ADRs in `docs/decisions/` (format and index in its README). Read them before reversing a choice; write one for any non-obvious new choice.
- Known defects (unchecked `playerID` from packets, `sv_serverType = 1` assignment, etc.) are listed in `docs/analysis/KEY_QUESTIONS.md`. Check there before "fixing" odd behaviour, since some of it is load-bearing.

## Writing documentation

Applies to everything under `docs/`, READMEs, this file, and comments.

- Be concise. Say it once, in the fewest words that stay accurate.
- Lead with the fact or decision. No preamble ("This document describes…"), no closing summary that repeats the body.
- Prefer short sentences, bullet lists and tables over paragraphs.
- Cite code as `path:line` instead of pasting large snippets.
- State facts plainly. No hedging, filler ("basically", "it is worth noting"), marketing tone or emoji.
- Don't restate what the code or another doc already says; link to it.
- Cut any sentence that doesn't change what the reader knows or does.
- When editing a doc, tighten nearby wordy text rather than adding to it.
