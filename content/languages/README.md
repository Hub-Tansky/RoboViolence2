# Languages

The game's strings. Format and fallback: [ADR 0011](../../docs/decisions/0011-gettext-po-translations.md).

| File | Role |
|---|---|
| `en.lang` | English, the source and per-key fallback (`key<TAB>text`, ends with `END`) |
| `en.pot` | Template for translators, generated: `python3 tools/lang-to-pot.py` after editing `en.lang` |
| `<ll>.po`, `<ll>_<CC>.po` | Translations (gettext PO, UTF-8) |

## Add a language

1. Copy `en.pot` to `<ll>.po` (ISO 639-1, e.g. `pl.po`), or `<ll>_<CC>.po` for a regional variant (`pt_BR.po`).
2. Translate in [Poedit](https://poedit.net/) or Weblate: fill each `msgstr`. Leave `msgctxt` (the key) and `msgid` (the English) unchanged. An empty `msgstr` shows the English text.
3. Keep the placeholders (`%s`, `%i`, `%02i`, …) in the same order: the game skips a translation that changes them, and the check fails. Keep `\n` line breaks and the colour codes `\x1`–`\x9` (one digit; `\x8` resets the colour).
4. Run `python3 tools/lang-check.py`: it rejects invalid PO, unknown keys and changed placeholders, warns about characters the font lacks, and lists untranslated keys.

## How the game picks a language

- `languageFile ""` (the default): the OS languages in preference order; for each, `<ll>_<CC>` then `<ll>`, `.po` before `.lang`; English if none matches.
- `languageFile "main/languages/<file>"` in the config always wins (`.po` or an original BV2 `.lang`). Builds before step 13 saved `languageFile "main/languages/en.lang"`; the game now treats that value as `""` and saves it back empty (PNS-47).
- Every key missing from the chosen file shows in English.
- The files are read from the package's `main/languages/`, then from the player's BV2 data (its original `.lang` files); for the same name the package wins ([ADR 0016](../../docs/decisions/0016-player-data-lookup-and-package-content.md)).

## Glyph limit

The font has ASCII and these Latin-1 letters only: `Ç ü é â ä à å ç ê ë è ï î ì Ä Å É æ Æ ô ö ò û ù ÿ Ö Ü ø £ Ø × ª` (`game/src/Lang.cpp`). Other characters (for example Polish `ą ę ł ż`, `ó`, Cyrillic) show as `?`. The OS-language default skips a PO that uses them; setting `languageFile` by hand still loads it. New fonts: PNS-20, PNS-21.
