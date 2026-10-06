# Roadmap

One folder per phase; inside it, the phase README (index and "done when") and one scope file per step, copied from [_template-step.md](_template-step.md). Work not yet planned into a phase lives in [possible-new-scope.md](possible-new-scope.md).

| Phase | Folder | Status |
|---|---|---|
| A: modern portable build | [phase-a-modern-portable-build/](phase-a-modern-portable-build/README.md) | DONE (2026-10-06) |
| B: security, infrastructure, anti-cheat | [phase-b-security-infrastructure-anti-cheat/](phase-b-security-infrastructure-anti-cheat/README.md) | TODO |

## Rules

- Every step file has a **Status** line under its title: `TODO`, `IN PROGRESS` or `DONE (YYYY-MM-DD)`. Set `DONE` when the step's PR merges; a phase is `DONE` when all its steps are.
- When closing a step, add anything learned about later phases or out-of-scope work to [possible-new-scope.md](possible-new-scope.md): where it was found, an extract of the source, why it matters and a suggested home. Don't change later phase plans directly; the owner accepts or rejects each entry.

## Ground rules (every phase)

1. **BV2 asset compatibility** ([ADR 0010](../decisions/0010-bv2-asset-compat-and-gui-freeze.md)). Until replacement assets ship (PNS-20, H.1), the game runs on original BV2 data (`BV2_DATA_DIR`) unchanged: same file formats (`.DKO`, `.bvm`, textures, sounds, fonts, `.lang`), names and paths under `main/`, and map download sends `.bvm` unchanged. New formats are added beside old ones, never instead. A PR touching a loader is tested locally with original data; CI keeps using placeholders.
2. **GUI freeze** (same ADR). Menus, HUD, layout, fonts and visible text stay as in BV2. Work marked **[GUI]** changes what the player sees and needs the owner's explicit approval before its scope file is written. If unmarked work turns out to change the interface, stop and ask.
3. **Internal renaming** ([ADR 0009](../decisions/0009-rename-internal-identifiers-continuously.md)). Replace `bv2`/`babo` with `roboviolence2` in code symbols, namespaces, source file and class names, internal CMake targets and variables, and comments, in files a step already touches. Binaries (`bv2`, `bv2dedicated`, `bv2master`), `BV2_*` env vars and CMake options, cvars, config and pref paths, `bv2.db`, file formats, protocol and master-server IDs, and visible strings wait for the rebrand (PNS-20, H.7). Renames are separate mechanical commits; tests stay green.
4. **Touch-and-improve** (PNS-19). In files a step already touches, replace raw `sprintf`/`strcpy`/`strcat` with `snprintf` or `std::string`, and use `std::unique_ptr` where ownership is clear. Add a test with each fix.

## Deprecated, to be removed

Kept only for compatibility; remove once the trigger holds, each removal with an ADR.

| Item | Kept for | Remove when |
|---|---|---|
| `.lang` language files and their loader (`game/src/GameVar.cpp` `loadLanguage`) | Original BV2 data (ADR 0010, ADR 0011) | BV2 assets are no longer supported (after PNS-20 H.1); `.po` becomes the only format and `en.po` replaces `en.lang` as the source for `en.pot` |
