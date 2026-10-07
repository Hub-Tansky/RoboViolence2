# AGENTS.md: Robo Violence 2

Robo Violence 2, an unofficial fork of BaboViolent 2 ([ADR 0008](docs/decisions/0008-display-name-robo-violence-2.md)): GPLv3 C++ top-down multiplayer shooter (bitHeads, 2012). Detailed architecture notes live in `docs/analysis/` (start with `docs/analysis/README.md`).

## Layout

- `engine/babonet/`: networking (`bb_*` API); `engine/zeven/`: engine utilities (`dkc`, `dksvar`, `CString`/`CVector`/`CMatrix`; client modules `dkw dki dkgl dkt dkf dkp dks` on SDL3, miniaudio and glad); `engine/dko/`: `.DKO` model loader. Each has `include/` (public), `src/`, `README.md`.
- `game/src/`: game: client, listen/dedicated server, map editor (one codebase; `CONSOLE` selects the headless server).
- `masterserver/src/`: standalone master server (SQLite).
- `content/`: `languages/`, `LaunchScript/` (`main/` at runtime). `config/`, `content-seed/`: example configs and SQL seeds. `tools/`: checks, generators, setup scripts.

[ARCHITECTURE.md](ARCHITECTURE.md) lists every tracked file; module READMEs describe each module.

## Build

```bash
cmake --preset <linux-x64|macos-arm64|win-x64-msvc>
cmake --build --preset <preset> --target bv2dedicated bv2master
```

- Needs `VCPKG_ROOT`, CMake 3.25+, Ninja, Python 3. `-asan` presets add ASan + UBSan.
- Output: `build/<preset>/runtime/` (executables, `main/`, `bv2.db`, `master.db`). Run the server from there ([ADR 0005](docs/decisions/0005-runtime-main-data-root.md)).
- `bv2` (client) needs the vcpkg `client` feature (SDL3, miniaudio, stb); `-DBV2_BUILD_CLIENT=OFF` builds the servers only. `-DBV2_WITH_HTTP=ON` adds libcurl (off by default, ADR 0002).
- Tests: `ctest --test-dir build/<preset> --output-on-failure` (`tests/`: netPacket layout, config layering and secret mask, dedicated-server smoke).
- CI (`.github/workflows/`), all required: `build` (Windows, macOS, Linux), `smoke` (Linux ASan: ctest and a 10 s client run under xvfb), `secret-scan`, `architecture`, `hygiene` (paths, ignored files, size, encoding, content case), `review` (`tools/review.sh` on PRs: scope, anti-gaming, DONE gate). Not required: `codeql`, `clang-tidy`; `dependency-graph` runs on `main`. vcpkg ports get no vulnerability alerts: bump the baseline monthly. Run them locally with `tools/review.sh` (`--full <preset>` adds build and ctest).
- Warnings are errors (`bv2_target_strict`) on `babonet`, `dko`, `zeven_core`; add a target once it is clean. Format only touched lines with `git clang-format`; never mass-reformat.

## Assets

- The original assets are removed. Never re-add them; `tools/check-original-assets.py` rejects them by hash.
- With your own copy of the original data, keep it outside the repo and set `BV2_DATA_DIR`. Without it, CI and contributors use generated placeholders (step 2 §2.6).

## Config and secrets

- Saved config, `bv2.db`, logs and maps go to the pref dir (`BV2_PREF_DIR` overrides; [ADR 0007](docs/decisions/0007-data-root-pref-dir-config-layers.md)). Env `BV2_MASTER_SERVERS`, `BV2_ACCOUNT_URL`, `BV2_SV_PASSWORD`, `BV2_ADMIN_PASS` and `--config <file>` override config and are never saved.
- Tracked config is `config/*.example.cfg` with empty placeholders; real values stay local. Policy: [config/README.md](config/README.md).
- Databases are generated from `content-seed/*.sql`, never committed.
- Run `tools/setup-dev.sh` (or `.ps1`) once: it activates `.githooks` (identity, gitleaks, original-asset and encoding checks).

## Agent rules

- Work one step at a time, from its scope file in `docs/roadmap/<phase>/`. Stay inside the step's **Allowed paths**; note out-of-scope work instead of doing it.
- Branch `refactor/<phase>-stepN-<slug>` (e.g. `refactor/phase-b-step1-playtest-builds`), one commit per task item (`stepN.M: <summary>`), one PR per step. Phase A step 0 was the only step committed straight to `main`.
- Never commit secrets, real hosts or IPs. Never use `--no-verify`.
- Finish every step by updating `ARCHITECTURE.md` (inventory and changed facts) and running `tools/check-architecture.sh`.
- When a step closes, set its **Status** to `DONE (YYYY-MM-DD)` and add what you learned about later phases or out-of-scope work to [docs/roadmap/possible-new-scope.md](docs/roadmap/possible-new-scope.md) ([docs/roadmap/README.md](docs/roadmap/README.md)).
- Record non-obvious decisions as ADRs in `docs/decisions/`.
- Work is done only per [REVIEW.md](REVIEW.md): `tools/review.sh` passes (CI `review` check), and a step closes with a fresh-context `/thermo-nuclear-code-quality-review` recorded in `docs/roadmap/<phase>/reviews/stepN.md`. Its stop rule and forbidden shortcuts apply to every change.
- Don't mix mechanical rewrites (encoding, renames, formatting) with functional changes in one commit.

## Compile-time variants

- `CONSOLE`: headless dedicated server (no rendering, client, editor or menus). Guard client-only code with `#ifndef CONSOLE`.
- The Direct3D path (`_DX_`), the non-Pro ruleset (`_PRO_` is now always on, version 21100) and Visual Leak Detector were removed in Step 1.

## Git identity

- All commits, merges, pushes and GitHub actions for this project run as **`Hub-Tansky`**: `user.email = 337104978+Hub-Tansky@users.noreply.github.com`. Never `patpi`, never a personal email.
- Run `gh` as `GH_TOKEN=$(gh auth token --user Hub-Tansky) gh …`. Don't run `gh auth switch`; other projects rely on `patpi` being active.
- Setup and guard hook: `docs/roadmap/phase-a-modern-portable-build/step0-fork-rename-and-asset-removal.md` §0.0.

## Conventions and gotchas

- All text files are UTF-8 (no BOM) with LF line endings. Keep them that way; the hook and CI reject other encodings (`tools/check-encoding.py`). Some comments contain U+FFFD where upstream lost accented characters; leave them.
- Fixed 30 Hz step: every `update(float delay)` gets `delay = 1/30`, and "frames" are a time unit (30 = 1 s).
- Network messages are raw structs in `game/src/netPacket.h`, `memcpy`'d on the wire. Positions are `short` ×100, velocities `char` ×10. Changing a struct layout requires bumping `GAME_VERSION_SV/CL` in `Server.h`/`Client.h`. Removed message IDs leave gaps; don't renumber.
- `playerID` (slot 0–31) is not `babonetID` (connection handle). `bb_serverSend` takes the babonetID; destination 0 means broadcast.
- The server is authoritative for hits, damage, spawns, projectiles and flags. Clients are authoritative for their own movement.
- Server variables (`sv_*`) are registered in `game/src/GameVar.cpp` via `dksvarRegister` and pushed to clients.
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
