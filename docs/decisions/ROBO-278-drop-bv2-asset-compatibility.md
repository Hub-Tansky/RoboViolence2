# ROBO-278: Drop BV2 data compatibility; name things RoboViolence outside and robo inside; rename only with big rewrites

- Status: Accepted
- Date: 2026-11-27
- Affects: [AGENTS.md](../../AGENTS.md) ground rules 1 to 3, "Assets" and "Deprecated"; [REVIEW.md](../../REVIEW.md) section 2; [game/CONTEXT.md](../../game/CONTEXT.md); supersedes [0010](0010-bv2-asset-compat-and-gui-freeze.md) for assets and [0062](0062-asset-packs-mode-modules-client-independent-server.md) R6; amends [0009](0009-rename-internal-identifiers-continuously.md), [0040](0040-first-run-downloads-original-data.md), [0042](0042-game-assets-option-remembers-placeholders.md), [0058](0058-agent-rule-order-ground-rules-3-4-refactor-only.md), [0069](0069-asset-packs-stack-over-the-base-pack.md), [ROBO-261](ROBO-261-build-option-fetches-game-assets.md); ROBO-278 to ROBO-286 (owner request 2026-11-27)

## Context
- ADR 0010 kept the game running on the original BaboViolent 2 data, and said dropping that needs a new ADR. Ground rule 1 made every loader PR test with original data and add new formats beside old ones (ADR 0062 R6).
- Owner, 2026-11-27: keeping BV2 data working slows development and gives players nothing; the base pack and the art pack (ADRs 0069, ROBO-260) now carry the game. Stability comes first, so renames must not become one big-bang change.
- Code reading the BV2 data still exists: the first-run download (ADR 0040), the Options folder (ADR 0042), the "pack without `main/pack.tsv`" stack rule (ADR 0069), `RV2_ASSETS=original` (ADR ROBO-261). ROBO-279 removes it.
- Direction and full plan: rv2-docs `30-dev/refactor-direction-handoff.md`, section "Owner decision 2026-11-27".

## Decision
1. **BV2 data is unsupported.** The original BV2 assets, maps and config get no new code, test, doc or fallback, and no feature considers them. The original-asset hash blocklist (`tools/original-assets.sha256`, `tools/check-original-assets.py`, install and package checks) stays as the licence safeguard. Formats the base pack, the art pack or `content/maps/` use (`.DKO`, `.bvm`, TGA, WAV, the bitmap font, `.lang`) keep their loaders; a new format may replace an old one.
2. **The GUI freeze stays** as its own rule (owner decision D1, ADR 0010): menus, HUD, layout, fonts and visible text change only in owner-approved `gui: true` work. A swappable default art set (theme, event, test packs over the default art pack, ROBO-280) is in scope.
3. **Naming:** `RoboViolence` / `roboviolence` for what players and admins see (folders, files, binaries, packages); `robo` / `ROBO_` inside (namespace `robo::`, env vars `ROBO_*`, CMake options and targets). This replaces ADR 0009's `roboviolence2` for code symbols. The existing `RV2_*` names and the `RoboViolence2` cache folder move to this scheme along the way.
4. **Rename rule:** only the user folder moves now (ROBO-281: `RoboViolence/`, `roboviolence.cfg`). Every other rename rides on an item that already rewrites that file or module (ROBO-282 code and env vars, ROBO-283 binaries and deploy), in its own `git mv` or mechanical commit listed in `.git-blame-ignore-revs`. Cvar names, protocol version 22100 and master IDs stay until ROBO-017.

## Consequences
- Loader and asset PRs are tested on the base pack and the art pack only; CI already does that. Reviewers flag code, tests or docs that add support for BV2 data (REVIEW.md section 2).
- Without anyone testing on BV2 data, an accidental reuse of a BV2 file is more likely, which is why the hash checks stay.
- `bv2`/`babo` names stay longer in files no big rewrite touches; grep both prefixes.
- Rejected: one rename PR for everything (a large diff that breaks scripts, configs and deploys at once, against stability first); keeping BV2 data as an optional pack (its only cost is ours, its only users are testers comparing with BV2); `rv2` as the internal prefix (owner chose `robo`).
