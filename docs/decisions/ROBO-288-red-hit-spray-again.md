# ROBO-288: Hit spray and floor splats are red again; the art pack's splats are smaller

- Status: Accepted
- Date: 2026-11-28
- Affects: ROBO-288, `game/src/GameSpawn.cpp` (`spawnBlood`, `spawnBloodMinibot`), `game/src/GameVar.cpp`, `game/src/GameVar.h`; supersedes [ROBO-274](ROBO-274-art-pack-hits-oil-and-coolant.md)

## Context
- ADR ROBO-274 tinted the art pack's white blood shapes black oil with coolant flecks, and kept red only for BV2 and TGA blood (ground rule 1).
- In round fixes 1 (2026-11-28) the owner approved red blood, with all ten splats redrawn at 70% of their visible radius and a pooled `blood05` (art pack 9).
- ADR ROBO-278 dropped BV2 data compatibility, so the TGA/PNG split has no reason left.

## Decision
- Player hits tint red as BV2 did: spray red 0.3 to 1, floor splats red 0.25 to 0.5; the minibot's additive spray red 0.3 to 1, its splats 0.5 to 1. Alpha, blending, counts, placement and sizes are unchanged.
- The smaller look comes from the textures only; the code adds no size multiplier.
- `HitSpray.h`, `GameVar::tex_bloodOil` and `tests/test_hit_spray.cpp` are removed.

## Consequences
- Blood looks the same with any blood textures; the tint no longer depends on the file format.
- Rejected: a floor-splat size multiplier in code (the art pack 9 textures already carry the 30% reduction; both would stack).
