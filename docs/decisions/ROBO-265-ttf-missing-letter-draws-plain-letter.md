# ROBO-265: A TTF font without a letter draws its plain letter; Teko may make text up to 20 % wider

- Status: Accepted
- Date: 2026-11-25
- Affects: ROBO-265, `engine/zeven/src/dkf_ttf.cpp`, `tests/test_text_fit.cpp`; amends [ADR 0067](0067-font-bytes-and-ttf-baking.md)

## Context
- ADR 0067's table bakes a code point a `.ttf` lacks as a blank. Teko, the new art's font, has no `Ľ` (U+013D, Slovak), so Slovak text loses that letter, where `babo.tga` draws plain `L` (`engine/zeven/src/CFont.cpp:214-219`).
- The bake narrows a font only until a-z average `babo.tga`'s width (ADR 0067). Teko's capitals are wider, so about a third of English strings grow, worst +16 % ("ROUND DRAW", "DEATHMATCH"), found by the ROBO-264 text-fit test.
- The owner chose to accept longer text (2026-11-24) and check overflow by eye on the staging playtest.

## Decision
A `.ttf` that lacks the code point of a byte from 160 to 255 bakes that byte from the plain letter in `FONT_EXTRA`, as `babo.tga` does. The `text_fit` test holds every English string within 20 % of its `babo.tga` width with both Teko weights.

## Consequences
- Every font byte draws something with any TTF that has ASCII; the test fails if a font lacks both a letter and its plain letter.
- A new font or new English text that grows a string past 20 % fails `text_fit`; that string or the budget is then a decision for the owner.
- Rejected: narrowing capitals or the whole font to babo.tga's A-Z width (letters 13-21 % thinner, off the approved look); a blank glyph (loses letters).
