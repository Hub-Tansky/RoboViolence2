# 0016: The client finds the player's BV2 data; the package supplies its own content

- Status: Accepted; amended by [0033](0033-first-run-dialog-recommends-bv2-zip.md), [0040](0040-first-run-downloads-original-data.md), [0041](0041-game-assets-option-remembers-placeholders.md)
- Date: 2026-10-14
- Affects: [Phase C step 1](https://github.com/Hub-Tansky/RoboViolence2/blob/58c91b2a660db2b290b0b5f504381980a6aedba2/docs/roadmap/phase-c-installers-and-fixes/step1-player-data-dir.md); amends [0005](0005-runtime-main-data-root.md) and [0007](0007-data-root-pref-dir-config-layers.md)

## Context
- Releases ship without the original assets (PNS-20 owner decision, 2026-10-14). Players had to set `BV2_DATA_DIR` permanently, which is hard on macOS and Windows.
- With `BV2_DATA_DIR` set, the single data root hid the package's launch scripts, language files and `bv2.example.cfg`, so repo changes to them (translations, the step 2 rebrand) never reached players.

## Decision
- Two roots: the **data root** (the player's BV2 data; the process `chdir`s into it, assets load as `main/...`) and the **package root** (the folder with the executable's own `main/`). `bv2::contentFile` reads launch scripts, `bv2.example.cfg` and a saved `languageFile` from the package when present; languages are picked from the package first, then the data root. `config/local/*.cfg` is read beside the package only.
- One rule finds a data folder under a path, for `BV2_DATA_DIR`, the saved folder, auto-detect and the dialog: the first of the path, its parent when it is `main/`, `Content`, `BaboViolent2/Content` (the upstream repository) and, since [0041](0041-game-assets-option-remembers-placeholders.md), `<name>/BaboViolent2/Content` (Windows Extract All) that holds `main/textures/`.
- The client looks for data in this order: `BV2_DATA_DIR`, the saved folder (`<prefDir>/data-dir.txt`), the upstream repository unzipped in `Downloads`, `Desktop` or `Documents` (and `BaboViolent 2` in Program Files on Windows), then a first-run native message box and OS folder dialog (owner-approved **[GUI]**). A found or chosen folder is saved. `--placeholders` skips the lookup (CI smoke run).
- The dedicated server reads only `BV2_DATA_DIR`; a saved or detected folder must not change test or container behaviour.

## Consequences
- An original `fr.lang` in the player's data still loads; for the same file name the package wins.
- The dialog text is English only until PNS-21.
- "Use placeholders" is not remembered: a player without data is asked on every start, as approved for PNS-51.
- Rejected: copying the assets into the pref dir (PNS-51 option F), a known pref-dir folder (A), an installer page (E): see PNS-51.
