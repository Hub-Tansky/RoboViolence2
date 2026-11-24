# ROBO-264: An art pack is checked against the base pack, and CI plays a generated fixture art pack

- Status: Accepted
- Date: 2026-11-24
- Affects: `tools/check-pack.py`, `tests/art_pack_fixture.py`, `tests/screenshots.py`, `tests/e2e/e2e.py`, `tests/golden/art-pack-*`; ROBO-264; builds on [0065](0065-asset-pack-manifest.md) and [0069](0069-asset-packs-stack-over-the-base-pack.md), ahead of the art pack (ROBO-260)

## Context
- The new art ships as a separate pack (`LicenseRef-RV2-Art`, ROBO-260) that never enters this repository, so CI only ever ran on the placeholder base pack.
- The mistakes to catch are a file the engine can't use (wrong size, no alpha where BV2 had it, a model where a texture belongs) and a misnamed id, which falls through to the placeholder without an error.

## Decision
- `tools/check-pack.py --base <base manifest>` adds, per row of the checked pack: the type must equal the base row's; an id equal to a base id ignoring case only fails; an id the base lacks is a NOTE (new ids are allowed, e.g. new themes); a texture must be a PNG or TGA with power-of-two sides, and must have alpha (PNG colour type 4 or 6 or a `tRNS` chunk, 32-bit TGA) when its base `format` ends in `x32`. The `base_pack` test runs the base pack with `--base` too.
- A row under `LicenseRef-RV2-Art` needs `tool` as well as author and licence (ADR 0065 left `tool` optional; it stays optional for other licences).
- `tests/art_pack_fixture.py` generates a stand-in art pack: one flat-colour PNG per base texture id, same size, alpha where 32-bit, its own `main/pack.tsv`. Goldens named `art-pack-*` and `e2e_ctf_capture_art_pack` run the client with it as `BV2_DATA_DIR`.

## Consequences
- The real pack is checked with `tools/check-pack.py <rv2-art>/main --base content/base-pack.tsv`; it is not in CI.
- The fixture covers textures only; fonts stay with the base pack until the Teko bake fits (ROBO-265). Theme folders and full-colour skins join the fixture with ROBO-259 and ROBO-258.
- Rejected: committing sample art (licence and repository size); checking only in the art repository (a rule change here would not reach it).
