# Refactoring handoff: Phase A (build, dependencies, legacy cleanup)

Project: **Robo Violence 2**, an unofficial fork of [Daivuk/BaboViolent2](https://github.com/Daivuk/BaboViolent2), without the original assets (the asset licence notes (kept outside this repository)).

This repository is developed **only by AI agents**. Every step of the refactor has one scope file here. An agent picks up exactly one step, stays inside that step's scope, and finishes by passing its acceptance checks and updating `ARCHITECTURE.md`.

Phase A turns the 2012 source release into a project with **one CMake + vcpkg build**. That build produces:

- the client for **Linux, macOS and Windows 11+** (equivalent targets),
- the dedicated server `bv2dedicated`,
- the master server `bv2master`.

Gameplay, security and anti-cheat are out of scope here; see [future-phases.md](../future-phases.md) (Phase B).

## Decisions already made

| Topic | Decision |
|---|---|
| Fork and name | GitHub fork renamed `RoboViolence2`, local dir `RoboViolence2-main`. Internal code identifiers move from `bv2` to `roboviolence2` as files are touched; binaries, env vars, paths and visible names stay until [future-phases.md](../future-phases.md) §H ([ADR 0009](../../decisions/0009-rename-internal-identifiers-continuously.md)) |
| Original assets | Not committed or redistributed. Removed from `main` in [step0](step0-fork-rename-and-asset-removal.md) §0.4; every removed file is catalogued in the asset inventory (kept outside this repository) for recreation; a hash blocklist stops them coming back. Local runs use your own copy via `BV2_DATA_DIR`; CI uses generated placeholders |
| Platforms | **Linux, macOS, Windows**. All three are equivalent first-class targets: none is secondary, and each one blocks a release. Linux x64 (Ubuntu 24.04 LTS as the reference distro; X11 and Wayland); macOS 12+ (arm64); Windows 11 and later (x64). arm64 Linux/Windows and x86_64 macOS are optional extras |
| Protocol | Free to break compatibility with 2.11 clients. Bump `GAME_VERSION_SV/CL` once |
| Build system | CMake + `CMakePresets.json`, Ninja generator on every preset ([ADR 0001](../../decisions/0001-ninja-generator-on-all-presets.md)) |
| Dependencies | vcpkg manifest (`vcpkg.json`, pinned baseline); no vendored headers or binaries |
| Audio | miniaudio replaces FMOD |
| Window/input | SDL3 replaces Win32/WGL, DirectInput 8 and SDL 1.2 |
| Rendering | OpenGL 2.1 + GLU stays for Phase A; isolate, then SDL_GPU ([ADR 0003](../../decisions/0003-keep-opengl-2.1-then-sdl-gpu.md)) |
| HTTP | libcurl compiled out (`BV2_WITH_HTTP` OFF); its backend is gone ([ADR 0002](../../decisions/0002-compile-out-libcurl.md)) |
| Source encoding | All text files are UTF-8 (no BOM) with LF, converted in one scripted commit in [step1](step1-baseline-and-legacy-removal.md) §1.4 and enforced by the hook and CI |
| Legacy | Drop `_DX_`, the non-Pro ruleset, VLD, VS200x projects, Launcher, UpdateServer, RemoteAdmin, and committed binaries |
| Secrets | No credentials, server IPs/hosts or private server config in the repo. Blocked by a pre-commit hook, CI and GitHub push protection ([step1](step1-baseline-and-legacy-removal.md) §1.2) |
| Repo layout | AI-ready: `AGENTS.md` + `ARCHITECTURE.md` at the root, kebab-case paths with no spaces, one README per module ([Target layout](#target-repository-layout-ai-ready)) |

## Steps and dependencies

| # | Scope file | Depends on | Outcome |
|---|---|---|---|
| 0 | [step0-fork-rename-and-asset-removal.md](step0-fork-rename-and-asset-removal.md) | nothing | Fork renamed `RoboViolence2`; local commits ported; asset inventory written; original assets removed from `main`; new `README.txt`. **No PRs before this is done** |
| 1 | [step1-baseline-and-legacy-removal.md](step1-baseline-and-legacy-removal.md) | 0 | Secrets removed and blocked (history scans start at the fork point); sources converted to UTF-8/LF; secret blocking, `.gitignore`, `AGENTS.md`, `ARCHITECTURE.md`; legacy code removed |
| 2 | [step2-cmake-build.md](step2-cmake-build.md) | 1 | Repo moved to the target layout; CMake builds `bv2dedicated` and `bv2master` on all three OSes |
| 3 | [step3-dependency-upgrades.md](step3-dependency-upgrades.md) | 2 | vcpkg deps; SDL3, miniaudio and glad platform layer; client runs on Windows, macOS and Linux |
| 4 | [step4-64bit-and-cross-platform.md](step4-64bit-and-cross-platform.md) | 2 (can overlap 3) | Warning-clean x64/arm64 builds; fixed-width packets; portable paths; config and secrets outside the install directory |
| 5 | [step5-code-hygiene-and-ci.md](step5-code-hygiene-and-ci.md) | 2 (CI), 4 (warnings) | Encoding guard, clang-format, required CI on 3 OSes including the secret scan |
| — | [future-phases.md](../future-phases.md) | Phase A complete | Phase B: supply-chain security, security fixes, anti-cheat foundations, infrastructure, networking, renderer, localisation, assets |

Steps 3 and 4 touch the same engine files, so run them one after the other or coordinate them. Nothing else should run in parallel on the engine.

## Scope file format (every step file follows it)

Each step file begins with a **Scope** block with these fields:

| Field | Meaning |
|---|---|
| Goal | One sentence: what is true after this step |
| In scope | The task list, numbered `N.M` so commits and PRs can reference it |
| Out of scope | Tempting work that belongs to another step. Don't do it; note it for later |
| Allowed paths | Paths the agent may create or modify. Anything else needs a new decision |
| Inputs | Docs and files to read before starting |
| Deliverables | Files and commits that must exist at the end |
| Definition of done | The acceptance checks. All must pass |

A new step starts as a copy of [_template-step.md](../_template-step.md).

## Working rules for agents

0. **Step 0 first.** It commits directly to `main`, in order, with no merge/pull requests. PRs start with Step 1.
1. **One step per branch** (`refactor/stepN-<slug>`), one commit per task item (`stepN.M: <summary>`), one PR per step (or per large item).
2. Read `AGENTS.md`, `ARCHITECTURE.md` and the step's **Inputs** first. Don't edit outside **Allowed paths**.
3. **Never commit secrets**: passwords, hashes, tokens, keys, real server IPs or hostnames, private server config. Use the `*.example` / `*.local.*` pattern from step 1 §1.2. Never bypass hooks (`--no-verify`).
4. Every step ends by updating `ARCHITECTURE.md`: its file inventory and any changed summary facts. CI checks the inventory from step 5 on.
5. Record non-obvious decisions as short ADRs in `docs/decisions/NNNN-title.md`; format and index in [../../decisions/README.md](../../decisions/README.md).
6. If a step can't meet its Definition of done, stop and write down what is blocking it in the PR description. Don't widen the scope.

## Target repository layout (AI-ready)

Created in Step 1 (root files) and Step 2 (moving code with `git mv`, which keeps history):

```
AGENTS.md              # canonical agent instructions (rules, commands, gotchas)
CLAUDE.md              # one line: @AGENTS.md
ARCHITECTURE.md        # summary of key facts + inventory of every file
README.md              # human quick start
LICENSE.txt
CMakeLists.txt  CMakePresets.json  vcpkg.json
.gitignore  .gitattributes  .gitleaks.toml  .editorconfig  .clang-format
.githooks/             # pre-commit (secret scan); enabled by tools/setup-dev.*
.github/workflows/     # CI: build matrix, secret scan, architecture inventory check
docs/
  analysis/            # code analysis (was "docs/code analysis")
  refactoring/         # this plan: one scope file per step
  decisions/           # ADRs
engine/
  babonet/  zeven/  dko/   # each: include/, src/, README.md, CMakeLists.txt
game/                  # was BaboViolent2/Code: src/, README.md, CMakeLists.txt
masterserver/          # was MasterServer/Source/src
content/               # was BaboViolent2/Content/main: languages, scripts; replacement assets later (§H)
content-seed/          # SQL seeds for generated DBs (bv2.db) with no endpoints or secrets
config/                # *.example.cfg templates only; real configs are gitignored *.local.cfg
assets-src/            # source art for replacement assets (§H); the originals are removed
tools/                 # setup-dev.sh / setup-dev.ps1, check-architecture, generators
tests/                 # smoke and unit tests (starting in step 5)
```

Rules that make it AI-friendly:

- no spaces or non-ASCII characters in paths;
- one purpose per directory, each with a short `README.md` (purpose, public API, dependencies, gotchas);
- the root `ARCHITECTURE.md` indexes everything;
- generated and local files are never committed.

## Background reading

- [../../analysis/README.md](../../analysis/README.md): architecture analysis index
- [../../analysis/01_SYSTEM_OVERVIEW.md](../../analysis/01_SYSTEM_OVERVIEW.md): deliverables, macros, module map
- [../../analysis/KEY_QUESTIONS.md](../../analysis/KEY_QUESTIONS.md): defect register (R1–R15) and security findings (Q-S1…Q-S8)
- [../../../AGENTS.md](../../../AGENTS.md): conventions (30 Hz step, packet rules, encoding)

## Phase A done when

- `cmake --preset <preset> && cmake --build --preset <preset>` builds `bv2`, `bv2dedicated` and `bv2master` on `linux-x64`, `macos-arm64` and `win-x64-msvc`.
- No vendored SDKs or committed binaries remain.
- On each of Linux, macOS and Windows 11, using local original data (`BV2_DATA_DIR`), the client connects to a `bv2dedicated` and plays DM, TDM and CTF rounds with sound, input and map download working.
- The master server lists that game server.

The two items above are checked by hand: [phase-a-manual-test.md](phase-a-manual-test.md).
- CI is **required and green** on all three OSes, including the secret scan and the `ARCHITECTURE.md` inventory check.
- `gitleaks git` over all commits after the fork point (`tools/FORK_BASE`) finds nothing, and `tools/check-original-assets.py` passes.
