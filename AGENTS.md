# AGENTS.md: Robo Violence 2

Robo Violence 2, an unofficial fork of BaboViolent 2 ([ADR 0008](docs/decisions/0008-display-name-robo-violence-2.md)): GPLv3 C++ top-down multiplayer shooter (bitHeads, 2012). The original code analysis (data structures, data flow, algorithms) is at [docs/analysis/ as of fc0aff7](https://github.com/Hub-Tansky/RoboViolence2/blob/fc0aff7c5aa946406c7c110e8e7c2d972a7daeb8/docs/analysis/README.md).

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

- Needs `VCPKG_ROOT`, CMake 3.25+, Ninja, Python 3. `-asan` presets add ASan + UBSan. Per-OS setup and running: [docs/build/](docs/build/README.md).
- Output: `build/<preset>/runtime/` (executables, `main/`, `master.db`, `web.db`). Run the server from there ([ADR 0005](docs/decisions/0005-runtime-main-data-root.md)). CI packages it per OS with `packaging/make-package.py`.
- `bv2` (client) needs the vcpkg `client` feature (SDL3, miniaudio, stb); `-DBV2_BUILD_CLIENT=OFF` builds the servers only. `-DBV2_WITH_HTTP=ON` adds libcurl (off by default, ADR 0002).
- Tests: `ctest --test-dir build/<preset> --output-on-failure` (`tests/`: netPacket layout, config layering and secret mask, dedicated-server smoke).
- CI (`.github/workflows/`), all required: `build` (Windows, macOS, Linux; Debug and `-release` presets; uploads the Release `roboviolence2-<preset>` packages, the Windows installer and the macOS `.dmg`; a `v*` tag also runs `release`, ADR 0020), `smoke` (Linux ASan: ctest and a 10 s client run under xvfb), `secret-scan`, `architecture`, `hygiene` (paths, ignored files, size, encoding, content case), `review` (`tools/review.sh` on PRs: scope, anti-gaming, DONE gate). Not required: `codeql`, `clang-tidy`, `package-check` (Linux package in a clean `ubuntu:24.04`), `package-smoke` (Windows and macOS packages: server, master, and client for 10 s; Mesa llvmpipe on Windows), `containers` (server images and the compose example; pushes to GHCR from `main`). On `main`: `dependency-graph`. Weekly: `osv-vcpkg` (OSV check of the vcpkg ports, `tools/osv-vcpkg.py`). Bump the vcpkg baseline monthly, or when `osv-vcpkg` reports a port. Run them locally with `tools/review.sh` (`--full <preset>` adds build and ctest).
- Warnings are errors (`bv2_target_strict`) on `babonet`, `dko`, `zeven_core`; add a target once it is clean. Format only touched lines with `git clang-format`; never mass-reformat.

## Assets

- The original assets are removed. Never re-add them; `tools/check-original-assets.py` rejects them by hash.
- With your own copy of the original data, keep it outside the repo; the client asks for it on first start, or set `BV2_DATA_DIR` ([ADR 0016](docs/decisions/0016-player-data-lookup-and-package-content.md)). Without it, CI and contributors use generated placeholders (`--placeholders` skips the dialog).

## Config and secrets

- Saved config, `bv2.db`, logs and maps go to the pref dir (`BV2_PREF_DIR` overrides; [ADR 0007](docs/decisions/0007-data-root-pref-dir-config-layers.md)). Env `BV2_MASTER_SERVERS`, `BV2_ACCOUNT_URL`, `BV2_SV_PASSWORD`, `BV2_ADMIN_PASS`, `BV2_ADMIN_API_PORT`/`_BIND`/`_PROXY`, `BV2_PUBLIC_ADDRESS` and `--config <file>` override config and are never saved. The master reads `BV2_MASTER_BAN_PASS`. `cl_masterServers` defaults to the public master and is never saved; tests start servers and clients with `none` ([ADR 0027](docs/decisions/0027-built-in-public-master.md)).
- Tracked config is `config/*.example.cfg` with empty placeholders; real values stay local. Policy: [config/README.md](config/README.md).
- Databases are generated from `content-seed/*.sql`, never committed.
- Run `tools/setup-dev.sh` (or `.ps1`) once: it activates `.githooks` (identity, gitleaks, original-asset and encoding checks).

## Work tracking

- The private GitHub Project [Robo Violence 2](https://github.com/users/Hub-Tansky/projects/1) is the only source of truth for backlog, next steps, bugs, features and proposed scope ([ADR 0024](docs/decisions/0024-github-project-is-the-backlog.md)). Never create `step*.md`, `NEXT_STEPS.md`, roadmap, task-list or scope files. If the Project can't be reached, say so and stop; no Markdown fallback.
- Statuses: **Proposed** (ideas, discovered or accepted-but-unscheduled work) → **Next** → **In progress** (also when blocked: the item body then starts with `Blocked by: <what>`) → **Testing** (implemented, awaiting tests before review or merge) → **Done**.
- New ideas and private plans are **draft items**. Convert a draft to a public issue only when the work is ready to expose or collaborate on. Issues are public.
- Every new item: a clear title, context (where it was found, evidence as `path:line` or a link), and an **Acceptance criteria** checklist. Set Phase, Category and Priority when known. Former PNS IDs live in the Source field.
- Work discovered while doing something else goes to the Project as a new Proposed draft item, never into the current diff.
- Moving an item: In progress when you start; Testing when the PR is up and waits for tests; Done comes from the PR merge or issue close. Run `gh` for the Project as below; it needs the `project` scope.
- Durable knowledge stays Markdown in the repo: `ARCHITECTURE.md`, ADRs, module READMEs, build guides.

## Agent rules

- Work one Project item at a time. The PR description holds its scope from [the PR template](.github/pull_request_template.md): goal, item link, out of scope and **Allowed paths**. Stay inside the Allowed paths; record out-of-scope work as a new draft item instead of doing it.
- Propose Minimal Changes: Provide only the simplest, most direct code or solution required to make the item work or fix the bug.
- Ignore Corner Cases: Do not write defensive code for rare edge cases, scale considerations, or downstream exceptions.
- Deliverables: Your output must reflect a "Minimum Viable Product" mindset for scope of item or bug. Keep the diff or response as small and readable as possible.
- Branch `<kind>/<slug>` with kind `feat`, `fix`, `refactor`, `ci`, `docs` or `chore` (e.g. `fix/master-ban-wire-format`); one commit per task (`<kind>: <summary>`); one PR per item, with `Closes #n` when the item is an issue.
- Never commit secrets, real hosts or IPs. Never use `--no-verify`.
- Finish every PR by updating `ARCHITECTURE.md` (inventory and changed facts) and running `tools/check-architecture.sh`.
- Record non-obvious decisions as ADRs in `docs/decisions/`.
- Work is done only per [REVIEW.md](REVIEW.md): `tools/review.sh` passes (CI `review` check), and a scoped PR is ready only with a review by the `thermo-nuclear-code-quality-review` subagent (`.claude/agents/`, rubric in `.claude/skills/thermo-nuclear-code-quality-review/`), which any session can spawn, recorded in the PR description. Its stop rule and forbidden shortcuts apply to every change.
- Don't mix mechanical rewrites (encoding, renames, formatting) with functional changes in one commit.

## Ground rules

1. **BV2 asset compatibility** ([ADR 0010](docs/decisions/0010-bv2-asset-compat-and-gui-freeze.md)). Until replacement assets ship (PNS-20, H.1), the game runs on original BV2 data (`BV2_DATA_DIR`) unchanged: same file formats (`.DKO`, `.bvm`, textures, sounds, fonts, `.lang`), names and paths under `main/`, and map download sends `.bvm` unchanged. New formats are added beside old ones, never instead. A PR touching a loader is tested locally with original data; CI keeps using placeholders.
2. **GUI freeze** (same ADR). Menus, HUD, layout, fonts and visible text stay as in BV2. Work marked **[GUI]** changes what the player sees and needs the owner's explicit approval, noted on its Project item, before work starts. If unmarked work turns out to change the interface, stop and ask.
3. **Internal renaming** ([ADR 0009](docs/decisions/0009-rename-internal-identifiers-continuously.md)). Replace `bv2`/`babo` with `roboviolence2` in code symbols, namespaces, source file and class names, internal CMake targets and variables, and comments, in files a PR already touches. Binaries (`bv2`, `bv2dedicated`, `bv2master`), `BV2_*` env vars and CMake options, cvars, config and pref paths, `bv2.db`, file formats, protocol and master-server IDs, and visible strings wait for the rebrand (PNS-20, H.7). Renames are separate mechanical commits; tests stay green.
4. **Touch-and-improve** (PNS-19). In files a PR already touches, replace raw `sprintf`/`strcpy`/`strcat` with `snprintf` or `std::string`, and use `std::unique_ptr` where ownership is clear. Add a test with each fix.

PNS-n names a Project item by its Source field.

## Deprecated, to be removed

Kept only for compatibility; remove once the trigger holds, each removal with an ADR.

| Item | Kept for | Remove when |
|---|---|---|
| `.lang` language files and their loader (`game/src/GameVar.cpp` `loadLanguage`) | Original BV2 data (ADR 0010, ADR 0011) | BV2 assets are no longer supported (after PNS-20 H.1); `.po` becomes the only format and `en.po` replaces `en.lang` as the source for `en.pot` |

## Compile-time variants

- `CONSOLE`: headless dedicated server (no rendering, client, editor or menus). Guard client-only code with `#ifndef CONSOLE`.
- The Direct3D path (`_DX_`), the compile-time `_PRO_` switch and Visual Leak Detector were removed in Step 1. The runtime ruleset `sv_serverType` (0 Normal, 1 Pro) stays ([ADR 0013](docs/decisions/0013-r3-restore-normal-and-pro-rulesets.md)).

## Git identity

- All commits, merges, pushes and GitHub actions for this project run as **`Hub-Tansky`**: `user.email = 337104978+Hub-Tansky@users.noreply.github.com`. Never `patpi`, never a personal email.
- Run `gh` as `GH_TOKEN=$(gh auth token --user Hub-Tansky) gh …`. Don't run `gh auth switch`; other projects rely on `patpi` being active.
- Setup and guard hook: `tools/setup-dev.sh` and `.githooks/` (history: [Phase A step 0 §0.0](https://github.com/Hub-Tansky/RoboViolence2/blob/58c91b2a660db2b290b0b5f504381980a6aedba2/docs/roadmap/phase-a-modern-portable-build/step0-fork-rename-and-asset-removal.md)).

## Conventions and gotchas

- All text files are UTF-8 (no BOM) with LF line endings. Keep them that way; the hook and CI reject other encodings (`tools/check-encoding.py`). Some comments contain U+FFFD where upstream lost accented characters; leave them.
- Fixed 30 Hz step: every `update(float delay)` gets `delay = 1/30`, and "frames" are a time unit (30 = 1 s).
- Network messages are raw structs in `game/src/netPacket.h`, `memcpy`'d on the wire. Positions are `short` ×100, velocities `char` ×10. Changing a struct layout requires bumping `GAME_VERSION_SV/CL` in `Server.h`/`Client.h`. Removed message IDs leave gaps; don't renumber.
- Wire data is untrusted until the pre-dispatch check passes (`Server::checkPacket` in `ServerRecv.cpp`, `checkServerPacket` in `ClientRecv.cpp`): exact size, sender slot, index ranges, string ends. A new message type needs an entry in that table. A `CString` built from one argument copies the text as is; two or more arguments are a printf format and its arguments, so never pass wire text as the format (`CString("%s", text)` is the explicit form; [ADR 0021](docs/decisions/0021-cstring-one-argument-is-text.md)).
- `playerID` (slot 0–31) is not `babonetID` (connection handle). `bb_serverSend` takes the babonetID; destination 0 means broadcast.
- The server is authoritative for hits, damage, spawns, projectiles and flags. Clients are authoritative for their own movement.
- Server variables (`sv_*`) are registered in `game/src/GameVar.cpp` via `dksvarRegister` and pushed to clients.
- Decisions are ADRs in `docs/decisions/` (format and index in its README). Read them before reversing a choice; write one for any non-obvious new choice.
- Known defects up to Phase C are in [KEY_QUESTIONS.md](https://github.com/Hub-Tansky/RoboViolence2/blob/fc0aff7c5aa946406c7c110e8e7c2d972a7daeb8/docs/analysis/KEY_QUESTIONS.md) (R/Q ids); new ones are Project items with Category Bug. Check both before "fixing" odd behaviour, since some of it is load-bearing.

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
