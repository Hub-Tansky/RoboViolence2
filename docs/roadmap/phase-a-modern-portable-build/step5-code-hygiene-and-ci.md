# Step 5: Code hygiene and CI

**Status:** DONE (2026-10-06)

**Depends on:** [Step 2](step2-cmake-build.md) for CI and [Step 4](step4-64bit-and-cross-platform.md) for raising the warning level. **Index:** [README.md](README.md)

## Scope

| Field | Value |
|---|---|
| Goal | Required CI on Windows, macOS and Linux guards the build, the secret policy and `ARCHITECTURE.md`; the encoding (converted in step 1), formatting and warnings stay enforced for agent-driven work |
| In scope | Tasks 5.1–5.6 below |
| Out of scope | Mass reformatting; fixing the warnings exposed by `-Wextra` beyond the engine libraries; behaviour changes |
| Allowed paths | `.github/**`, `.clang-format`, `.clang-tidy`, `.editorconfig`, `.gitattributes`, `.git-blame-ignore-revs`, `tools/**`, `tests/**`, `CMakeLists.txt` files, `AGENTS.md`, `ARCHITECTURE.md`, `README.md` |
| Inputs | `AGENTS.md`, `ARCHITECTURE.md`, the warning list left by Step 4 |
| Deliverables | CI workflows (including the encoding guard), smoke test, lint config |
| Definition of done | "Acceptance checks" below pass |

## Context

After Steps 1–4 the project builds on modern toolchains. This step adds the enablers that make later agent-driven work safe. Each item is mechanical and gets its **own commit**.

## Tasks

### 5.1 Encoding guard (the conversion itself is done in step 1 §1.4)

- The sources were converted to UTF-8/LF in one scripted commit in step 1 §1.4. This step only makes sure that stays true.
- Wire `tools/check-encoding.py` into the CI `hygiene` job (§5.4). It fails on invalid UTF-8, a BOM or `\r` in any tracked text file.
- Check `.git-blame-ignore-revs` contains the `step1.4` commit, and document `git config blame.ignoreRevsFile .git-blame-ignore-revs` in `tools/setup-dev.*`.
- Optional follow-up (record it as an ADR if done): make `dkf`/`CFont` decode UTF-8, so chat and translated text can use non-Latin characters.

### 5.2 Formatting

- Add a `.clang-format` close to the existing style (sample a few files first) and a `.editorconfig`.
- **Don't** mass-reformat. Format only the lines and files that are touched (`git clang-format`).

### 5.3 Warnings as a ratchet and static analysis

- Raise the level to `-Wall -Wextra` / `/W4`: engine targets first, then game targets.
- Add `-Werror` / `/WX` **per target** once that target is clean. Start with `babonet`, `zeven_core` and `dko`.
- Add a `.clang-tidy` with `bugprone-*`, `clang-analyzer-*` and `cert-err34-c`. It's non-blocking in CI at first.

### 5.4 CI (GitHub Actions); all jobs required

- **`build`** matrix, all three required:
  - `windows-latest` (Windows Server 2022+/MSVC; stands in for the Windows 11 target), preset `win-x64-msvc` (set up the MSVC environment with `ilammy/msvc-dev-cmd`, [ADR 0001](../../decisions/0001-ninja-generator-on-all-presets.md));
  - `macos-14` (arm64), preset `macos-arm64`;
  - `ubuntu-24.04`, preset `linux-x64`; install the apt packages from step 2 §2.5.
  - vcpkg binary cache.
  - Artifacts: `bv2dedicated` for each OS, the macOS `.app` (zipped), the Linux client tarball.
- **`smoke`**: on the placeholder content from step 2 §2.6, the Linux `-asan` preset starts `bv2dedicated` headless for about 10 s with a test config (load a map, then quit). The client runs under `xvfb-run` with `SDL_AUDIODRIVER=dummy`: reach the main menu, then quit.
- **`secret-scan`** (added in step 1): gitleaks over commits after `tools/FORK_BASE`, plus `tools/check-original-assets.py`. Make it a required check.
- **`architecture`**: `tools/check-architecture.sh` fails if `git ls-files` and the `ARCHITECTURE.md` inventory differ.
- **`hygiene`**:
  - no files whose path contains spaces;
  - no tracked files matching the `.gitignore` secret, binary and OS-junk categories (`git ls-files -ci --exclude-standard` is empty);
  - no files over 5 MB outside `content/` and `assets-src/`.
  - `tools/check-encoding.py`: every tracked text file is UTF-8 without a BOM, with LF only.
- The user makes all five **required status checks** in branch protection (a manual GitHub setting).

### 5.5 First tests

- `tests/` with a CTest target.
- Candidates, which also serve [Phase B](../phase-b-security-infrastructure-anti-cheat/README.md):
  - `netPacket.h` round-trips and size asserts;
  - loading every placeholder map (and, locally, every original map via `BV2_DATA_DIR`);
  - `dksvar` parsing and the config layering order (step 4 §4.5);
  - a secret mask test: `*pass*` variables are never echoed.

### 5.6 Docs

- `AGENTS.md`: build and test commands, CI checks, encoding rule, "never `--no-verify`".
- `README.md`: build instructions for Windows 11, macOS and Linux.
- `ARCHITECTURE.md`: CI and test inventory.

## Critical files

- `.github/workflows/{build,secret-scan}.yml`, `tools/check-architecture.*`, `.clang-format`, `.clang-tidy`, `.editorconfig`, `.git-blame-ignore-revs`, `tests/CMakeLists.txt`
- Updated: `.gitattributes`, `AGENTS.md`, `README.md`, `ARCHITECTURE.md`

## Acceptance checks

- All five CI jobs are green on `main`, and a PR that breaks any of them can't merge.
- A PR that adds `cl_accountPassword "x"`, a public IP, `.obsidian/`, `.DS_Store` or a `*.local.cfg` is blocked (by `.gitignore` for untracked noise, or by the secret-scan / hygiene checks).
- `babonet`, `zeven_core` and `dko` build with `-Werror` / `/WX` on all three OSes.
- A PR that adds a Latin-1 or CRLF source file is blocked by the `hygiene` job.
