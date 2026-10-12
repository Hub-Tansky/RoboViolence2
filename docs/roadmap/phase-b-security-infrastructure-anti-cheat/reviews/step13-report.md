SKILL: Launching skill: thermo-nuclear-code-quality-review

# Step 13 fresh-context review report

Diff: `bbe93ad..e89b9dc` (4 commits: 9a0b0a2 step13.2, 43a1c93 step13.1, 7b7d0d3 step13.3, e89b9dc step13.4). Worktree: RoboViolence2-step13. I only read the code. I did not build it or run ctest or review.sh.

## Correctness of the moved .lang parser (task item 3)

I compared `lang::readLang` (game/src/Lang.cpp:84-130) with the removed `GameVar::loadLanguage` (`git show bbe93ad:game/src/GameVar.cpp`, around line 1223):
- **Latin-1 to font bytes.** The 32 chained `if`s became the `LATIN1[32]` loop. Every source value is 163 or higher and every target is in 128-159, so the old `if`s never chained. Same result.
- **Escapes.** `\\` and `\xN` give the same output. The old `case '\\'` was a no-op because the char was already `\\`, and dropping it changes nothing. `\x` followed by a non-digit keeps that char in both. Tabs are dropped in both. The 510-byte bound and the EOF and END handling are unchanged.
- **'%' handling.** The old code wrote `%` as `%%`, then `CString value = varValue` ran it through `CString(char* fmt, ...)` (engine/zeven/src/CString.cpp:38), which turned it back into `%`. The new code stores the raw `%` and assigns with `CString("%s", text)` (GameVar.cpp:1247). Both store the same final text, e.g. `"%s\x8 joined the game"`. Later `CString(lang_x.s, args)` formatting therefore sees the same format string as before.
  - Side effect: the old doubling could reach `varValue[511]`. The new code can't.
- **Fallback.** Keys missing from a chosen file now fall back to English instead of keeping their constructor values. This is the intended change (ADR 0011).
- **en.lang.** Pure ASCII, tab-separated, and holds no backslash escapes other than `\n` and `\xN`. So `lang-to-pot.py` and the C++ reader agree on the real file.

## Findings

1. **Medium.** `tools/lang-check.py:62-77` and `content/languages/README.md:16`. Translations are used as printf formats, and the checker never compares placeholders.
   - Many keys are format strings fed to `CString(char* fmt, ...)`: `lang_joinedTheGame` `%s`, `lang_spawnIn` `%i:%02i`, `lang_serverVersion` `%01i.%02i.%02i`, and others.
   - A `.po` whose `msgstr` drops, adds, reorders the types of, or invents a conversion is accepted by `lang-check.py` and loaded by the game. Examples: `%s` where the English has `%i`, an extra `%s`, or `%n`. The result is undefined behaviour or a crash when the string is formatted.
   - The guide only asks translators to "keep" the placeholders. Before this step the only text source was the repo's `en.lang`. PO from outside contributors is the new attack and mistake surface.
   - **Fix:** in `lang-check.py`, extract `re.findall(r'%[-+ #0]*\d*(?:\.\d+)?[a-zA-Z%]', s)` from the `en.lang` text and from the `msgstr` (unescaped) for each key. Fail when the lists differ. Add one bad-placeholder case to a test, or show it as fix proof by running the checker on a scratch `.po`.
   - Optionally, `utf8ToFont`/`parsePo` could reject an entry whose specifiers differ from the English one. That keeps a hand-set `languageFile` safe too.

2. **Low.** `game/src/Lang.cpp:390` (the `unquote` `\t` branch) with `fontByte` (Lang.cpp:346), and `tools/lang-check.py:19`. A PO `\t` escape becomes byte 9, which the font treats as colour code `\x9`.
   - `fontByte` accepts 1-9 as colour codes, so a tab in a translation silently changes the text colour.
   - The `.lang` reader drops raw tabs. The PO path keeps them as colour codes.
   - **Fix:** drop `\t` from the accepted escapes in both parsers, so the C++ and Python checks stay in agreement. Alternatively, map `\t` to a space in `unquote`.

3. **Low.** `tools/lang-check.py` (no glyph check). A committed `.po` that uses characters the font lacks passes hygiene. At runtime it is then skipped by auto-detection without any message (Lang.cpp:523).
   - A translator adding `pl.po` gets green CI and a game that still shows English.
   - **Fix:** in `lang-check.py`, report (warn, not fail) characters outside ASCII and the 32 Latin-1 letters. The guide already lists the set; reuse it as a Python constant that mirrors `LATIN1`.

4. **Low (correctness and agreement).** The C++ and Python PO parsers disagree on a repeated keyword within one entry.
   - C++ `parsePo` (Lang.cpp:493-494) appends to `*cur` for a second `msgstr "..."` (or `msgid`/`msgctxt`) in the same entry, because `unquote` appends.
   - Python `read_po` (lang-check.py:41) replaces the value.
   - Both accept the file, so the checker's view of the text differs from the game's.
   - **Fix:** in C++, clear `*cur` before `unquote` on a keyword line. Better, reject a repeated keyword in both parsers.

5. **Low (legibility, skill §3/§4).** `game/src/Lang.cpp:460-500`: `parsePo` folds end-of-input, line trimming, entry flushing and keyword dispatch into one loop.
   - End-of-input is handled by a sentinel iteration: `more`, `line` forced to `""`, `next = !more || ...`.
   - The Python version (lang-check.py:24-43), with a `flush()` closure and a plain loop, is easier to read.
   - **Fix:** restructure to `while (getline) { ...; if (starts entry) flush(); ... } flush();` with a small `flush` lambda. Behaviour stays the same and the `more`/`next` flags go away.

6. **Low (simplification).** `game/src/Lang.cpp:511-539`: `pickFile` fully parses a candidate `.po` to test its glyphs and then discards the texts. `load` then parses the same file again.
   - `lang::load(dir, file, out)` also mixes a directory (for `en.lang`) with a full path (for `file`).
   - **Fix (code-judo):** have `pickFile` return the chosen path only, and let `load(dir, file)` stay as is. Alternatively, fold the two into `load(dir, file or locales)` so the parse happens once. At minimum, document why the double parse is accepted.

7. **Low (process, REVIEW.md §2).** Commits don't match their task items.
   - 9a0b0a2 ("step13.2: PO loader") already contains task 13.1's runtime logic: `osLocales()`, `SDL_GetPreferredLocales`, `pickFile` wired into `reloadLanguage`.
   - 43a1c93 ("step13.1") only flips the default to `""`.
   - 9a0b0a2 also combines moving the loader out of GameVar.cpp with functional changes (the fallback and the dropped `%` doubling). It is not a pure mechanical move, so the §2 "no mixed mechanical+functional" rule is arguably not hit. Still, the move is not reviewable as a move.
   - **Fix:** no history rewrite needed. Note it in the review record. For later steps, keep one task per commit.

8. **Low (docs).** Stale references.
   - `docs/decisions/0011-gettext-po-translations.md:8` cites `game/src/GameVar.cpp:271`, which is now `languageFile = ""` at GameVar.cpp:260, with the loader in Lang.cpp. That file is outside Allowed paths.
   - The step file's Context cites `GameVar.cpp:271` and `GameVar::loadLanguage` (`GameVar.cpp:1223`), and that function no longer exists.
   - The `ARCHITECTURE.md` row for `Lang.h` lists `readLang, parsePo, pickFile, load` but omits `readPo`.
   - **Fix:** update the ARCHITECTURE row and the step Context. Ask the owner about the one-line ADR 0011 touch, or record it in `possible-new-scope.md`.

9. **Low (behaviour, owner decision).** Existing installs keep English.
   - `dksvarSaveConfig` writes every var to the pref-dir config, so players who ran an earlier build have `languageFile "main/languages/en.lang"` saved.
   - The "a saved `languageFile` always wins" rule then means OS auto-detection never activates for them.
   - This matches the step text, but it may not be what the owner expects for the "3 friends" playtest group.
   - **Fix:** either accept and note it in the guide ("delete the `languageFile` line to follow the OS"), or ask the owner whether the old default value should count as auto. I prefer the guide note: it is a smaller change and doesn't add a special case.

10. **Info (tests).** `osLocales()` (GameVar.cpp:1226-1235) and `reloadLanguage()` have no test. These are the SDL formatting of `ll_CC` and the `CString("%s", …)` assignment.
    - The `%`-equivalence argument above is reasoning, not a test.
    - `test_lang` covers the parsers, the fallback, `pickFile` and the glyph skip, so the step's acceptance list is met.
    - It also closes the step 2b deferral "no test seam for the language loader" (`reviews/step2b.md:47`): `readLang` now has a no-END test. The record could say so.
    - **Fix:** optional. Add a `test_lang` case asserting `en["lang_b"]` keeps a single `%s`, which is already there, and state in the record that `CString("%s", x)` keeps `%` literally.

## Section 2 checklist
- **Ground rules.** BV2 `.lang` still loads through the same byte mapping. There is no GUI change. The `bv2.example.cfg` default change is owner-approved and noted in Allowed paths.
- **New behaviour has tests.** Yes, `test_lang` for Lang.cpp. Not for the locale query or `lang-check.py`; see findings 1 and 10.
- **ARCHITECTURE and READMEs.** Updated: every new file is listed and `content/languages/README.md` was added. The ADR has a stale line reference (finding 8). There is no new non-obvious decision without an ADR; the `.po`-before-`.lang` order and the glyph skip are in ADR 0011 and the step file.
- **Mechanical and functional mix.** See finding 7.
- **Allowed paths.** Every changed file fits.
- **File size.** GameVar.cpp shrank, from about 1356 to 1265 lines. Lang.cpp has 213.
- **Repeats of earlier findings.** None of these repeats a finding rejected or deferred in an earlier record. Finding 10 relates to step2b #3, which this step resolves rather than repeats.

## Verdict
The move is behaviour-preserving for `.lang` files, including the `%` and CString formatting. The PO parser, the locale picking and the glyph skip are correct and tested. The design is small and sits in the right layer. One finding should be fixed before close: finding 1, placeholder validation, because translated text now reaches printf-style formatting from outside contributors. Findings 2-9 are low. Fix the cheap ones (2, 4, 8) and record the rest as rejected or deferred with a reason.
