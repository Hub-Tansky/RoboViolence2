# ROBO-274: Art pack hits spray oil and coolant; BV2 blood stays red

- Status: Superseded by [ROBO-288](ROBO-288-red-hit-spray-again.md)
- Date: 2026-11-26
- Affects: ROBO-274, `game/src/HitSpray.h`, `game/src/GameSpawn.cpp` (`spawnBlood`, `spawnBloodMinibot`), `game/src/GameVar.cpp`, `tests/test_hit_spray.cpp`

## Context
- Robots don't bleed. The owner decided (O1, 2026-11-26) that hits are black oil with a few coolant flecks, never red (style guide 01).
- Art pack 7's `textures/blood01` to `blood10` are white shapes with alpha. The code tints hit particles and floor splats; BV2 tinted them red.
- Ground rule 1 (ADR 0010): with BV2 data the game looks as before.

## Decision
- When `textures/blood01` resolves to a PNG (the art pack's format), player hits tint dark grey oil (0.02 to 0.1, around token `oil` #0B0B0C) with one particle in six coolant (#39FF8E); the minibot's additive spray, where oil would not show, tints coolant only. The rules live in `HitSpray.h`.
- Otherwise (BV2 data, base pack placeholders, all TGA) the old red tint stays, with the old ranges.

## Consequences
- The tint is decided once, when the textures load; a pack that later delivers PNG blood takes effect on the next start, as its textures do.
- A future art pack that ships blood as TGA would get red; ship PNG.
- Rejected: coloured textures in the pack (the code's tint would still multiply them, and one texture set could not be both oil and coolant); red (O1); tinting BV2 blood too (breaks ground rule 1).
