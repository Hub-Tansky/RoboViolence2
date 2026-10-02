# AGENTS.md: RoboViolence 2

RoboViolence 2, an unofficial fork of BaboViolent 2 ([ADR 0004](docs/decisions/0004-project-name-roboviolence2.md)): GPLv3 C++ top-down multiplayer shooter (bitHeads, 2012). Detailed architecture notes live in `docs/analysis/` (start with `docs/analysis/README.md`).

## Layout

Current (target layout: [docs/refactoring/README.md](docs/refactoring/README.md#target-repository-layout-ai-ready), reached in Step 2):

- `BaboViolent2/Code/`: game: client, listen/dedicated server, map editor (one codebase; `CONSOLE` selects the headless server)
- `BaboViolent2/inc/`: project headers shared with the engine (`baboNet.h`, `dk*.h`, `cMSstruct.h`, ...)
- `Engine/babonet/`: networking library (`bb_*` API)
- `Engine/DukZeven/`: engine utilities: `dkc` timer, `dksvar` config vars, `dkw` window, `dkgl`, `dki`, `dkt`, `dkf`, `dkp`, `dks` (FMOD)
- `Engine/dko/`: `.DKO` model loader and ray/sphere intersection
- `MasterServer/Source/`: standalone master server (SQLite; no build system yet)
- `config/`, `content-seed/`: example configs and SQL seeds; `tools/`: checks and setup scripts

[ARCHITECTURE.md](ARCHITECTURE.md) lists every tracked file.

## Build

- Linux dedicated server: `make` in the repo root builds `BaboViolent2/Content/bv2dedicated` (needs sqlite3, libcurl and GLU dev packages). Step 1 removed the vendored SDKs, so it builds only after Steps 2 and 3.
- Client and master: no build until Step 2 (CMake + vcpkg). The Visual Studio project is kept only as a source-list reference.
- There are no automated tests.

## Assets

- The original assets are removed ([docs/ASSETS-LICENSE.md](docs/ASSETS-LICENSE.md), [docs/assets/ASSET-INVENTORY.md](docs/assets/ASSET-INVENTORY.md)). Never re-add them; `tools/check-original-assets.py` rejects them by hash.
- With your own copy of the original data, keep it outside the repo and set `BV2_DATA_DIR` (step 4 §4.4; until then symlink `main/` into the run dir). Without it, CI and contributors use generated placeholders (step 2 §2.6).

## Config and secrets

- Tracked config is `config/*.example.cfg` with empty placeholders; real values stay local. Policy: [config/README.md](config/README.md).
- Databases are generated from `content-seed/*.sql`, never committed.
- Run `tools/setup-dev.sh` (or `.ps1`) once: it activates `.githooks` (identity, gitleaks, original-asset and encoding checks).

## Agent rules

- Work one step at a time, from its scope file in `docs/refactoring/`. Stay inside the step's **Allowed paths**; note out-of-scope work instead of doing it.
- Branch `refactor/stepN-<slug>`, one commit per task item (`stepN.M: <summary>`), one PR per step. Step 0 was the only step committed straight to `main`.
- Never commit secrets, real hosts or IPs. Never use `--no-verify`.
- Finish every step by updating `ARCHITECTURE.md` (inventory and changed facts) and running `tools/check-architecture.sh`.
- Record non-obvious decisions as ADRs in `docs/decisions/`.
- Don't mix mechanical rewrites (encoding, renames, formatting) with functional changes in one commit.

## Compile-time variants

- `CONSOLE`: headless dedicated server (no rendering, client, editor or menus). Guard client-only code with `#ifndef CONSOLE`.
- The Direct3D path (`_DX_`), the non-Pro ruleset (`_PRO_` is now always on, version 21100) and Visual Leak Detector were removed in Step 1.

## Git identity

- All commits, merges, pushes and GitHub actions for this project run as **`Hub-Tansky`**: `user.email = 337104978+Hub-Tansky@users.noreply.github.com`. Never `patpi`, never a personal email.
- Run `gh` as `GH_TOKEN=$(gh auth token --user Hub-Tansky) gh …`. Don't run `gh auth switch`; other projects rely on `patpi` being active.
- Setup and guard hook: `docs/refactoring/step0-fork-rename-and-asset-removal.md` §0.0.

## Conventions and gotchas

- All text files are UTF-8 (no BOM) with LF line endings. Keep them that way; the hook and CI reject other encodings (`tools/check-encoding.py`). Some comments contain U+FFFD where upstream lost accented characters; leave them.
- Fixed 30 Hz step: every `update(float delay)` gets `delay = 1/30`, and "frames" are a time unit (30 = 1 s).
- Network messages are raw structs in `BaboViolent2/Code/netPacket.h`, `memcpy`'d on the wire. Positions are `short` ×100, velocities `char` ×10. Changing a struct layout requires bumping `GAME_VERSION_SV/CL` in `Server.h`/`Client.h`. Removed message IDs leave gaps; don't renumber.
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
