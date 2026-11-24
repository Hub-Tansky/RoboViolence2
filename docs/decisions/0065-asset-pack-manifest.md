# 0065: An asset pack is a folder with a pack.tsv manifest; the base pack's is content/base-pack.tsv

- Status: Accepted; amended by [ROBO-264](ROBO-264-art-pack-checks-and-fixture.md)
- Date: 2026-11-17
- Affects: `content/base-pack.tsv` (replaces `tools/placeholder-manifest.tsv`), `tools/gen-placeholder-content.py`, `tools/check-pack.py`, `tools/check-content-case.py`, root `CMakeLists.txt`; ROBO-231 (refactor Phase 1 A1, [ADR 0062](0062-asset-packs-mode-modules-client-independent-server.md) asset seam)

## Context
- The 207 files the game loads at startup were listed in `tools/placeholder-manifest.tsv` (folder, name, format), read only by the placeholder generator and the case check. It had no stable name for an asset and no author or licence, which the replacement art needs (issue #97 H.2).
- The owner makes the replacement assets with several tools (D4); each needs author, tool and licence on record.

## Decision
An asset pack is a folder whose root holds `pack.tsv`, a UTF-8 TAB-separated manifest with a header row and the columns `id`, `file`, `type`, `format`, `author`, `tool`, `licence`, `source`, one row per asset; the base pack's manifest is `content/base-pack.tsv`, the build copies it to `main/pack.tsv` beside the generated placeholders, and `tools/check-pack.py` checks a pack against its manifest.

| Column | Holds |
|---|---|
| `id` | Stable name code asks for: the BV2 path below `main/` without the extension (`textures/BlueFlag`), unique ignoring case. It survives a format change: `textures/BlueFlag.tga` can become `.png` with the same id |
| `file` | Path below the pack root |
| `type` | `texture`, `sound`, `model` or `font` |
| `format` | What the loader needs (TGA size and depth, WAV channels and rate, DKO meshes); placeholders copy it |
| `author`, `tool`, `licence` | Who made it, with what, and the SPDX licence. Required except `tool` |
| `source` | Where it came from; `generated placeholder`, or `missing` when the pack has no file yet (the check allows it and prints a NOTE) |

## Consequences
- Placeholder rows name the project's contributors, the generator and GPL-3.0-or-later. Real art replaces a row's file, format, author, tool, licence and source; the id stays.
- The generator's output is byte-identical to before, plus `main/pack.tsv`. CI test `base_pack` runs `tools/check-pack.py` on the generated `main/`; `check_pack` tests the checker.
- Original BV2 data has no `pack.tsv`. Because ids are the BV2 paths, ROBO-232 (registry) can read it with the base manifest's ids; stacking packs is ROBO-234.
- `Menu.ogg` and `Music.ogg` are `missing`: the placeholders have no music.
- Rejected: JSON or YAML (harder to diff and edit by hand than one line per asset, and the generator stays stdlib-only); ids unrelated to BV2 paths (would need a second table to read BV2 data).
