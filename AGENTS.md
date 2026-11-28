# AGENTS.md: Robo Violence 2

Robo Violence 2, an unofficial fork of BaboViolent 2 ([ADR 0008](docs/decisions/0008-display-name-robo-violence-2.md)): GPLv3 C++ top-down multiplayer shooter (bitHeads, 2012). The original code analysis (data structures, data flow, algorithms) is at [docs/analysis/ as of fc0aff7](https://github.com/Hub-Tansky/RoboViolence2/blob/fc0aff7c5aa946406c7c110e8e7c2d972a7daeb8/docs/analysis/README.md).

## Layout

- `engine/babonet/`: networking (`bb_*` API); `engine/zeven/`: engine utilities (`dkc`, `dksvar`, `CString`/`CVector`/`CMatrix`; client modules `dkw dki dkgl dkt dkf dkp dks` on SDL3, miniaudio and glad); `engine/dko/`: `.DKO` model loader; `engine/credentials/`: password HMAC proofs (libsodium). Each has `include/` (public), `src/`, `README.md`.
- `game/src/`: game: client, listen/dedicated server, map editor (one codebase; `CONSOLE` selects the headless server).
- `masterserver/src/`: standalone master server (SQLite).
- `content/`: `languages/`, `LaunchScript/` (`main/` at runtime). `config/`, `content-seed/`: example configs and SQL seeds. `tools/`: checks, generators, setup scripts.

[ARCHITECTURE.md](ARCHITECTURE.md) holds the facts and one row per module directory; module READMEs describe each module.

## Build

```bash
cmake --preset <linux-x64|macos-arm64|win-x64-msvc>
cmake --build --preset <preset> --target bv2dedicated bv2master
```

- Needs `VCPKG_ROOT`, CMake 3.25+, Ninja, Python 3. `-asan` presets add ASan + UBSan. Per-OS setup and running: [docs/build/](docs/build/README.md).
- Output: `build/<preset>/runtime/` (executables, `main/`, `master.db`, `web.db`). Run the server from there ([ADR 0005](docs/decisions/0005-runtime-main-data-root.md)). CI packages it per OS with `packaging/make-package.py`.
- `bv2` (client) needs the vcpkg `client` feature (SDL3, miniaudio, stb); `-DBV2_BUILD_CLIENT=OFF` builds the servers only. `-DBV2_WITH_HTTP=ON` adds libcurl (off by default, ADR 0002).
- Tests: `ctest --test-dir build/<preset> --output-on-failure` (`ctest -N` lists them).
- Required checks: `build` (Windows, Linux; macOS runs only on main, `release/**`, tags and nightly, [ADR ROBO-268](docs/decisions/ROBO-268-macos-builds-on-main-and-nightly.md)), `smoke`, `scan`, `architecture`, `hygiene`, `review`, `format`. Before pushing C++: `tools/check-format.sh`. All jobs and what they do: [docs/build/README.md](docs/build/README.md#ci).
- Required checks are set in the repository ruleset "main protection" (Settings > Rules > Rulesets), not in the workflows. Only the owner changes it: "Require status checks to pass" > "Add checks", pick the job name (source GitHub Actions), save. A job appears in the picker only after it has run once, so add it after its first PR run. Branches need not be up to date with `main`. Keep the required list above equal to the ruleset; check it with `gh api repos/Hub-Tansky/RoboViolence2/rulesets/24412532 --jq '.rules[] | select(.type=="required_status_checks") | .parameters.required_status_checks[].context'`.
- Warnings are errors (`bv2_target_strict`) on `babonet`, `dko`, `zeven_core`; add a target once it is clean. Format only touched lines with `git clang-format`; never mass-reformat.

## Assets

- The original assets are removed. Never re-add them; `tools/check-original-assets.py` rejects them by hash.
- Original BV2 data is unsupported ([ADR ROBO-278](docs/decisions/ROBO-278-drop-bv2-asset-compatibility.md)): add no code, test, doc or fallback for it. The code that still reads it (first-run download, folder search, `RV2_ASSETS=original`) is removed by ROBO-279; don't extend it. CI and contributors use generated placeholders (`--placeholders` skips the first-run box) and the art pack.

## Config and secrets

- Saved config, `bv2.db`, logs and maps go to the pref dir (`BV2_PREF_DIR` overrides; [ADR 0007](docs/decisions/0007-data-root-pref-dir-config-layers.md)). Env `BV2_MASTER_SERVERS`, `BV2_ACCOUNT_URL`, `BV2_SV_PASSWORD`, `BV2_SV_BOTS`, `BV2_ADMIN_USER`, `BV2_ADMIN_PASS`, `BV2_ADMIN_API_PORT`/`_BIND`/`_PROXY`, `BV2_PUBLIC_ADDRESS` and `--config <file>` override config and are never saved. The master reads `BV2_MASTER_BAN_PASS`. `cl_masterServers` defaults to the public master and is never saved; tests start servers and clients with `none` ([ADR 0028](docs/decisions/0028-built-in-public-master.md)).
- Tracked config is `config/*.example.cfg` with empty placeholders; real values stay local. Policy: [config/README.md](config/README.md).
- Databases are generated from `content-seed/*.sql`, never committed.
- Run `tools/setup-dev.sh` (or `.ps1`) once: it activates `.githooks` (identity, gitleaks, original-asset and encoding checks, then `pre-commit` for clang-format and clang-tidy on changed lines, [ADR 0045](docs/decisions/0045-cpp-lint-pre-commit-on-changed-lines.md)). clang-tidy needs `cmake --preset macos-arm64`; on other OSes commit with `SKIP=clang-tidy`.

## Work tracking

- The backlog is the private repo [Hub-Tansky/rv2-docs](https://github.com/Hub-Tansky/rv2-docs) (the owner's Obsidian vault), folder `40-backlog/`: one file per scope item, `ROBO-NNN <Title>.md` ([ADR 0044](docs/decisions/0044-backlog-in-the-rv2-docs-vault.md)). Its [AGENTS.md](https://github.com/Hub-Tansky/rv2-docs/blob/main/AGENTS.md) has the folders, fields and who moves what. Never create `step*.md`, `NEXT_STEPS.md`, roadmap, task-list or scope files in this repo. If rv2-docs can't be reached, say so and stop.
- Statuses and folders: **Proposed** `20-proposed/` → **Next** `10-next/` (the owner moves items there) → **In progress** / **Testing** `30-inprogress/` (blocked work stays there with `blocked_by`) → **Done** `90-done/`.
- Every new item: a clear title, context (where it was found, evidence as `path:line` or a link) and an **Acceptance criteria** checklist, from `40-backlog/_templates/scope.md`. Former PNS-n ids live in `aliases`.
- Work discovered while doing something else becomes a new item in `20-proposed/`, never part of the current diff.
- Moving an item: to `30-inprogress/` (status In progress) when you start, status Testing when the PR waits for tests. A move is a commit in rv2-docs. After a merge the `backlog-move` workflow moves the item named first in the PR's Item row to `90-done/`, or keeps it Testing while a `(post-deploy)` criterion is unticked; move it by hand only when that run warns, or when no run happens (a fork PR, or a PR into a branch other than `main` or `release/**`) ([ADR ROBO-249](docs/decisions/ROBO-249-backlog-item-moves-on-merge.md)).
- GitHub issues are the public intake for bugs and requests. An accepted issue gets a scope item whose `issue:` links it.
- Durable knowledge about the code stays Markdown in this repo: `ARCHITECTURE.md`, ADRs, module READMEs, build guides, `docs/ops/runbook.md`. The release procedure (`20-product/releases/release.md`) lives in rv2-docs (ADR 0044), and so do player docs (`10-public/`, published by rv2-website, its ADR 0004).
- Recipes and gotchas for agent sessions that aren't about the code (Mac and cloud session limits, testing a PR build, deploys, review gate pitfalls) are in rv2-docs `30-dev/shared.md` (on the owner's Mac `~/Dev/RoboViolence2/Robo2-docs/30-dev/shared.md`). Read it before working around a tool or access problem, and add what you learn there.

## Agent rules

- Work one scope item at a time. The PR description holds its scope from [the PR template](.github/pull_request_template.md): goal, item (`ROBO-NNN`), out of scope and **Allowed paths**. Stay inside the Allowed paths; record out-of-scope work as a new Proposed item instead of doing it.
- Propose Minimal Changes: Provide only the simplest, most direct code or solution required to make the item work or fix the bug.
- Ignore Corner Cases: Do not write defensive code for rare edge cases, scale considerations, or downstream exceptions.
- Deliverables: Your output must reflect a "Minimum Viable Product" mindset for scope of item or bug. Keep the diff or response as small and readable as possible.
- Order when rules conflict: the scope item, then the three rules above, then the ground rules. Ground rule 3 has its own condition (a big rewrite); ground rule 4 applies only in items of kind `refactor`. A reviewer does not raise a case the scope item does not name unless it is reachable from the network.
- Commit subjects are `<kind>: <summary>` with kind `feat`, `fix`, `refactor`, `ci`, `docs`, `test` or `chore`; review fixes are `<kind>: review: <summary>`. One PR per item, with `Closes #n` when the scope item's `issue:` links one. Branch names and the number of commits are not reviewed.
- Never commit secrets, real hosts or IPs. Never use `--no-verify`.
- Finish every PR by updating `ARCHITECTURE.md` (changed facts, and a row for a new module directory) and running `tools/check-architecture.sh`. Files get no rows (ADR 0063).
- Record non-obvious decisions as ADRs in `docs/decisions/`, named after the scope item: `ROBO-NNN-<slug>.md`, never a new `NNNN-` number ([ADR ROBO-248](docs/decisions/ROBO-248-adrs-named-by-scope-item.md)).
- Work is done only per [REVIEW.md](REVIEW.md): `tools/review.sh` passes (CI `review` check), and a ready PR has a review record from the `rv2-code-review` workflow (`.claude/skills/rv2-code-review/`, section 5), for its head or a head with the same `Patch-id`. Its stop rule and forbidden shortcuts apply to every change.
- Merge by tier (`tools/merge-risk.py`, printed by `tools/review.sh`; [ADR 0060](docs/decisions/0060-merge-by-risk-tier-with-owner-brief.md)). HIGH: a risk path (`netPacket.h`, `ServerRecv.cpp`, `ClientRecv.cpp`, `engine/babonet/`, `engine/credentials/`), `packaging/`, a guard file, a `GAME_VERSION_SV/CL` change, a `[GUI]` item, more than 600 added lines or more than 25 files. MEDIUM: other changes under `game/`, `engine/`, `masterserver/`, `content/`. LOW: only docs, tests and non-guard tooling, alone or mixed. Anything else: MEDIUM. LOW and MEDIUM: merge when required checks are green and post the owner brief in the thread. HIGH: also run the Risk worker, request a GitHub Copilot review on the PR, and wait for the owner's playtest on staging and "merge". Exception: Dependabot's GitHub Actions minor and patch PRs merge themselves when green ([ADR ROBO-250](docs/decisions/ROBO-250-dependabot-actions-auto-merge.md)).
- Don't mix mechanical rewrites (encoding, renames, formatting) with functional changes in one commit.
- At most two open PRs per session. Finish review and CI on one before starting a third.

## Agent skills

Configuration for the mattpocock engineering skills.

### Issue tracker

Scope items `ROBO-NNN` in rv2-docs `40-backlog/`; GitHub issues are intake only. See `docs/agents/issue-tracker.md`.

### Triage labels

Default labels on GitHub issues; accepted issues become Proposed scope items. See `docs/agents/triage-labels.md`.

### Domain docs

Multi-context: `CONTEXT-MAP.md` lists the glossaries (root `CONTEXT.md` for code review, `game/CONTEXT.md`), ADRs in `docs/decisions/`. See `docs/agents/domain.md`.

## Ground rules

1. **No BV2 compatibility** ([ADR ROBO-278](docs/decisions/ROBO-278-drop-bv2-asset-compatibility.md)). The original BV2 assets, maps and config are unsupported: no new code, test, doc or fallback considers them. The original-asset hash checks stay. A loader change is tested on the base pack and the art pack. Formats those packs or `content/maps/` use (`.DKO`, `.bvm`, TGA, WAV, the bitmap font, `.lang`) keep their loaders; a new format may replace an old one.
2. **GUI freeze** ([ADR 0010](docs/decisions/0010-bv2-asset-compat-and-gui-freeze.md), kept by ADR ROBO-278). Menus, HUD, layout, fonts and visible text stay as they are. Work marked **[GUI]** changes what the player sees and needs the owner's explicit approval, noted in its scope item (`gui: true`), before work starts. If unmarked work turns out to change the interface, stop and ask.
3. **Renaming only with big rewrites** ([ADR ROBO-278](docs/decisions/ROBO-278-drop-bv2-asset-compatibility.md), ROBO-282). Names players and admins see become `RoboViolence` / `roboviolence` (folders, files, binaries, packages); names inside become `robo` / `ROBO_` (namespace `robo::`, env vars `ROBO_*`, CMake options and targets), also the existing `RV2_*` names. Only an item that already rewrites a file or module (a big rewrite: most of its lines change, or it moves or splits, as in ROBO-012, ROBO-017, ROBO-018) renames the `bv2`/`babo`/`RV2_` names in it, in its own `git mv` or mechanical commit listed in `.git-blame-ignore-revs`; tests and goldens unchanged by that commit. A renamed env var or CMake option keeps its old name working with one warning line. Never a standalone rename PR, except the user folder move (ROBO-281); cvars, protocol version and master IDs stay until ROBO-017.
4. In items of kind `refactor` only: **Touch-and-improve** (PNS-19). In files a PR already touches, replace raw `sprintf`/`strcpy`/`strcat` with `snprintf` or `std::string`, and use `std::unique_ptr` where ownership is clear. Add a test with each fix.

Rules 5 to 9 keep new code from adding debt against the long-term direction ([ADR 0062](docs/decisions/0062-asset-packs-mode-modules-client-independent-server.md)); they apply to every PR, and moving old code is for `refactor` items.

5. **Asset ids:** no new asset path literal (`"main/…"`) in game code; a new asset gets an id in the pack manifest.
6. **No new `CONSOLE` split:** no new `#ifdef CONSOLE` or `#ifndef CONSOLE`; rules code includes no GL, audio or input header.
7. **Modes:** no new `gameType ==` branch outside mode code. No mode code exists until the mode interface (Phase 2), so until then a branch the item cannot avoid needs the owner's approval in the PR.
8. **Data, not code:** tunable numbers (gameplay values a designer would change: speed, damage, timing, range) go in data files or cvars, not in constructor calls.
9. **Protocol:** a new or changed network message gets explicit field-by-field read and write, a sample in `tests/corpus/` and a line in the protocol doc (the Network row of `ARCHITECTURE.md` until `docs/protocol.md` exists); messages carry ids, never file names or paths.

PNS-n is an old id: scope item ROBO-0nn carries it in `aliases`.

## Deprecated, to be removed

Kept only for compatibility; remove once the trigger holds, each removal with an ADR.

| Item | Kept for | Remove when |
|---|---|---|
| `.lang` language files and their loader (`game/src/GameVar.cpp` `loadLanguage`) | `content/languages/en.lang`, the source of `en.pot` (ADR 0011); BV2 data is unsupported since ADR ROBO-278 | `en.po` replaces `en.lang` as the source for `en.pot` (old-format cleanup, ROBO-285); `.po` becomes the only format |

## Compile-time variants

- `CONSOLE`: headless dedicated server (no rendering, client, editor or menus). Guard client-only code with `#ifndef CONSOLE`.
- The Direct3D path (`_DX_`), the compile-time `_PRO_` switch and Visual Leak Detector were removed in Step 1. The runtime ruleset `sv_serverType` (0 Normal, 1 Pro) stays ([ADR 0013](docs/decisions/0013-r3-restore-normal-and-pro-rulesets.md)).

## Git identity

- All commits, merges, pushes and GitHub actions for this project run as **`Hub-Tansky`**: `user.email = 337104978+Hub-Tansky@users.noreply.github.com`. Never `patpi`, never a personal email.
- Run `gh` as `GH_TOKEN=$(gh auth token --user Hub-Tansky) gh …`. Don't run `gh auth switch`; other projects rely on `patpi` being active.
- Setup and guard hook: `tools/setup-dev.sh` and `.githooks/`. Cloud sessions get the hooks from `tools/cloud-setup.sh`.
- A Claude Code hook (`.claude/hooks/guard.py`) refuses `--no-verify`, force pushes to `main`, `v*` tags and their push, `gh auth switch`, `gh secret` and ruleset writes, and asks before a guard file is edited; `.githooks/commit-msg` refuses Claude commit trailers ([ADR ROBO-256](docs/decisions/ROBO-256-claude-code-guard-hooks.md)).

## Conventions and gotchas

- All text files are UTF-8 (no BOM) with LF line endings. Keep them that way; the hook and CI reject other encodings (`tools/check-encoding.py`). Some comments contain U+FFFD where upstream lost accented characters; leave them.
- Fixed 30 Hz step: every `update(float delay)` gets `delay = 1/30`, and "frames" are a time unit (30 = 1 s).
- Network messages are raw structs in `game/src/netPacket.h`, `memcpy`'d on the wire. Positions are `short` ×100, velocities `char` ×10. Changing a struct layout requires bumping `GAME_VERSION_SV/CL` in `Server.h`/`Client.h`. Removed message IDs leave gaps; don't renumber.
- Wire data is untrusted until the pre-dispatch check passes (`Server::checkPacket` in `ServerRecv.cpp`, `checkServerPacket` in `ClientRecv.cpp`): exact size, sender slot, index ranges, string ends. A new message type needs an entry in that table. A `CString` built from one argument copies the text as is; two or more arguments are a printf format and its arguments, so never pass wire text as the format (`CString("%s", text)` is the explicit form; [ADR 0021](docs/decisions/0021-cstring-one-argument-is-text.md)).
- `playerID` (slot 0–31) is not `babonetID` (connection handle). `bb_serverSend` takes the babonetID; destination 0 means broadcast.
- The server is authoritative for hits, damage, spawns, projectiles and flags. Clients are authoritative for their own movement.
- Server variables (`sv_*`) are registered in `game/src/GameVar.cpp` via `dksvarRegister` and pushed to clients.
- Decisions are ADRs in `docs/decisions/` (format and index in its README). Read them before reversing a choice; write one for any non-obvious new choice.
- A test waits for a condition with a deadline, never for a fixed time. The nightly `flake-watch` workflow runs the `e2e_*` and `bots` tests 10 times and opens a "Flaky test: <name>" issue per failure ([ADR 0064](docs/decisions/0064-nightly-flake-watch.md)).
- Tests start the client only through `tests/run_autotest.py` or `tests/e2e/`, with a scratch `BV2_PREF_DIR`. Never start the GUI client on the owner's desktop without asking.
- Known defects are in the backlog: items in `40-backlog/` with `kind: bug` (see [Work tracking](#work-tracking)). Check them before "fixing" odd behaviour, since some of it is load-bearing.

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
