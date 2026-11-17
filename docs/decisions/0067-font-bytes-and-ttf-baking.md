# 0067: Text is one byte per glyph; bytes 160-255 hold 96 extra letters; a TTF is baked to babo.tga widths

- Status: Accepted
- Date: 2026-11-17
- Affects: `engine/zeven/include/FontCodes.h`, `engine/zeven/src/dkf_ttf.cpp`, `engine/zeven/src/CFont.cpp`, `engine/zeven/src/dkw.cpp`, `game/src/Lang.cpp`, `game/src/Writting.cpp`, `tools/lang-check.py`, `content/languages/README.md`; ROBO-238 (refactor Phase 1 A3, [ADR 0062](0062-asset-packs-mode-modules-client-independent-server.md))

## Context
- All text (chat, names, HUD, translations) is a `char` string; `CFont` draws one display list per byte. Bytes 1-9 are colour codes, 32-126 ASCII, and BV2's `babo.tga` has 32 Latin-1 letters at bytes 128-159 (`GameVar::loadLanguage` remapped them). Bytes 160-255 were unused.
- Owner brief D4 asks for TTF fonts with Latin Extended (Polish ą ł ś, Czech ž, Hungarian ő). Several Latin-1 letters Polish, Czech and Spanish need (ó, á, ñ) are also missing from BV2's 32.
- Changing text to UTF-8 or wide strings would touch every string, the network protocol and saved names.

## Decision
Text stays one byte per glyph, and the owner chose (decision card, 2026-11-17) to put 96 more letters in bytes 160-255: `FontCodes.h` holds the byte to code point table (`FONT_LATIN1` for 128-159, `FONT_EXTRA` for 160-255, each extra letter with its plain ASCII letter). A `.ttf` font is baked at load with stb_truetype into a 64 px per line atlas holding all 224 glyphs (bytes 32-255), narrowed horizontally so the a-z average advance is `BV2_LOWERCASE_ADVANCE` (0.273 line heights, measured on BV2's `babo.tga`), so menus and the HUD keep their layout.

| Font | Bytes 128-159 | Bytes 160-255 |
|---|---|---|
| `babo.tga` (BV2 or placeholder) | its own glyphs | the plain letter's glyph (ł draws as l) |
| `.ttf` | baked from the font | baked from the font; a code point the font lacks draws a blank |

- Translations (`Lang.cpp`) map UTF-8 to these bytes through `bv2::fontByteFor`; a letter outside the table becomes `?` and a `.po` with such letters is not picked from the OS locale. `tools/lang-check.py` reads the same table.
- Typed text (`dkw`) maps each code point through `bv2::typedFontByte` and drops letters without a byte; `Writting` no longer remaps Latin-1.
- Romanian `Ș ș Ț ț` (comma below) map to the cedilla bytes `Ş ş Ţ ţ`; Romanian `Â` and `Î` have no byte.

## Consequences
- No string, protocol or save format changes. Before this, a typed letter outside BV2's 32 (ñ) was stored as its Latin-1 byte and drew nothing; now it is its table byte; a name typed with ł on a TTF client shows as l on a `babo.tga` client.
- 96 slots are full: another script (Cyrillic, Greek) needs a wider text encoding, a new decision.
- The table order is a format: changing it changes what saved names and translations show. Add letters only by replacing, never by reordering.

## Alternatives considered
- UTF-8 everywhere: right long term, but it touches every string, width calculation and the protocol; out of scope for A3.
- Code page per language (ISO 8859-2 for Polish): two players with different languages would see different letters for the same byte.
