# Step 1: Player data outside the package

**Status:** DONE (2026-10-14)

**Depends on:** Phase B steps. **Next:** step 2 (in-game name and credits). **Index:** [README.md](README.md)

## Scope

| Field | Value |
|---|---|
| Goal | The client finds or asks for the player's BV2 data without an env var, and launch scripts, language files and default config always load from the package |
| In scope | Tasks 1.1–1.4 below |
| Out of scope | Installers (step 3); in-game branding (step 2, PNS-53); copying the player's data (PNS-51 option F, dropped); translating the picker text (PNS-21); the dedicated server's lookup beyond `BV2_DATA_DIR` |
| Allowed paths | `game/src/Paths.{h,cpp}`, `game/src/DataDirPicker.{h,cpp}` (new), `game/src/main.cpp`, `game/src/GameVar.cpp`, `game/src/Console.cpp`, `game/src/Lang.{h,cpp}`, `game/CMakeLists.txt`, `game/README.md`, `tests/**`, `.github/workflows/build.yml` (smoke run gets `--placeholders`, owner GO 2026-10-14), `content/languages/README.md`, `config/README.md` (owner 2026-10-14: `config/local` location), `docs/build/**`, `docs/decisions/**`, `README.md`, `AGENTS.md`, `ARCHITECTURE.md`, `docs/roadmap/**` |
| Inputs | `AGENTS.md`, `ARCHITECTURE.md`, PNS-51, [ADR 0005](../../decisions/0005-runtime-main-data-root.md), [ADR 0007](../../decisions/0007-data-root-pref-dir-config-layers.md), [ADR 0010](../../decisions/0010-bv2-asset-compat-and-gui-freeze.md), [ADR 0011](../../decisions/0011-gettext-po-translations.md) |
| Deliverables | Package root and content lookup in `Paths`; saved path, auto-detect and first-run picker in the client; tests; ADR 0016; updated build guides |
| Definition of done | See "Acceptance checks". All must pass. `ARCHITECTURE.md` is updated |

## Context

- `pathsInit` picks one data root (`BV2_DATA_DIR`, else beside the executable, else the working directory) and `chdir`s into it; code opens `main/...` relative to it (`game/src/Paths.cpp:139`).
- With `BV2_DATA_DIR` set, the package's `main/` is ignored: its launch scripts (`game/src/Console.cpp:677`), language files (`game/src/GameVar.cpp:1247`) and `main/bv2.example.cfg` (`game/src/Paths.cpp:237`) come from the player's BV2 copy instead.
- The upstream repository keeps the data in `BaboViolent2/Content/main/` (with `fr`, `nl`, `hu`, `swe` `.lang` files); a downloaded zip extracts to `BaboViolent2-master/`.
- CI's smoke job runs the client without data (`.github/workflows/build.yml:106`); a modal dialog there would block it.

## Tasks

### 1.1 Package root and content lookup

- `pathsInit` also records the package root: the first folder with `main/` among the app bundle, `<exe>/../share/bv2`, the exe folder and the working directory. With no valid `BV2_DATA_DIR`, the data root is the package root (unchanged behaviour).
- `contentFile(rel)`: the package's copy of `rel` when it exists, else `rel` in the data root. Used for `main/bv2.example.cfg` and `main/LaunchScript/*.cfg`.
- Language files: `lang::pickFile` takes a list of folders, package first, so a player's original `fr.lang` still loads when the package has no French; `en.lang` (the base) comes from the package.

### 1.2 Saved path and auto-detect

- `findDataIn(dir)`: the folder holding `main/textures/` at `dir`, its parent when `dir` is `main/`, `dir/Content` or `dir/BaboViolent2/Content`.
- Saved path: one line in `<prefDir>/data-dir.txt`; ignored when it no longer holds data.
- Auto-detect: `BaboViolent2-master` and `BaboViolent2` in the home folder's `Downloads`, `Desktop` and `Documents`; on Windows also `BaboViolent 2` and `BaboViolent2` in `Program Files` and `Program Files (x86)`.

### 1.3 First-run picker [GUI] (owner-approved 2026-10-14)

- Client only, when `BV2_DATA_DIR` is unset, `--placeholders` is absent, and neither the saved path nor auto-detect finds data.
- A native message box explains that the BV2 data isn't included, with "Choose folder…" and "Use placeholders". "Choose folder…" opens the OS folder dialog (`SDL_ShowOpenFolderDialog`); a folder without data shows a message and returns to the first box. A valid choice is saved (1.2).
- `--placeholders` skips the lookup; the CI smoke run passes it.

### 1.4 Docs and ADR

- ADR 0016: lookup order and the package/data split; amends ADR 0005 and 0007.
- `docs/build/` guides, `content/languages/README.md`, `game/README.md`, `AGENTS.md`: `BV2_DATA_DIR` becomes optional for players.

## Critical files

- `game/src/Paths.{h,cpp}`, `game/src/DataDirPicker.{h,cpp}`, `game/src/main.cpp`, `game/src/Lang.cpp`, `game/src/GameVar.cpp`, `game/src/Console.cpp`

## Acceptance checks

- `ctest --test-dir build/<preset> --output-on-failure` passes, including `paths` (package/data split, `findDataIn`, saved path) and `lang` (folder order).
- With original data: the client started without `BV2_DATA_DIR` and no saved path shows the picker; choosing the BV2 folder starts the game with original assets and English text from the package; the next start doesn't ask.
- With `BV2_DATA_DIR` set to original data, `execute CTF` runs the package's launch script.
- `tools/review.sh --full <preset>` passes, and the fresh-context review record `reviews/step1.md` and the raw reviewer report `reviews/step1-report.md` (with the skill's load line) exist (`REVIEW.md` section 5).
- `ARCHITECTURE.md` lists every file added, moved or removed in this step.
- The secret scan (`gitleaks git --staged` / CI) is clean.
