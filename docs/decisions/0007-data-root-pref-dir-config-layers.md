# 0007: Data root, per-user pref dir and layered config

- Status: Accepted
- Date: 2026-10-03
- Affects: [step 4 §4.4, §4.5](../refactoring/step4-64bit-and-cross-platform.md); refines [ADR 0005](0005-runtime-main-data-root.md)

## Context
- The game wrote `bv2.cfg`, `bv2.db`, logs, screenshots and downloaded maps into `main/` and the working directory. That fails in read-only installs and dirties the repo.
- Master server list and account URL used to come from the seeded database; real hosts must not be tracked.

## Decision
- `game/src/Paths.{h,cpp}` (`bv2` namespace) resolves two roots at startup.
- **Data root** (read-only; contains `main/`), first hit wins: `BV2_DATA_DIR`, macOS `Contents/Resources`, `<exe>/../share/bv2`, exe dir, cwd. `pathsInit` then `chdir`s there, so the `main/...` literals keep working.
- **Pref dir** (writable): `BV2_PREF_DIR`, else `%APPDATA%\BaboViolent2\bv2`, `~/Library/Application Support/BaboViolent2/bv2`, `$XDG_DATA_HOME/BaboViolent2/bv2`. Holds `bv2.cfg`, `bv2.db`, banlist, logs, screenshots, reports, downloaded and saved maps.
- **Config layers**, later overrides earlier: `main/bv2.example.cfg`; pref `bv2.cfg` (created on first run); `config/local/*.cfg` and `--config <file>`; env `BV2_MASTER_SERVERS`, `BV2_ACCOUNT_URL`, `BV2_SV_PASSWORD`, `BV2_ADMIN_PASS`.
- Values from the last two layers are transient: `saveConfig` skips them, so secrets never reach `bv2.cfg`. `dksvar` masks variables whose name contains `pass`.
- Unconfigured master list or account URL disables that feature.

## Consequences
- A read-only install dir works; `git status` stays clean after play.
- Existing saved configs in `main/` are not migrated.
- `set <name> <value>` typed in the console is still echoed.
- Rejected: `SDL_GetPrefPath` (needs SDL in the headless server); rewriting the `main/` prefix (see ADR 0005).
