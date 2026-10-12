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

## Round 2

SKILL: Launching skill: thermo-nuclear-code-quality-review

## Round 2 review: step 13 fix commits (`git diff e89b9dc..HEAD`: fa1e951, 2b70477, 8651e22, bb3d697)

Read-only review in the worktree `/Users/piotr/Dev/BaboViolent_2_relaunch/RoboViolence2-step13`. Nothing was edited.

### Verification of the round 1 fixes

- **1 (printf conversions): mostly fixed, with one gap (finding A below).**
  - C++ `formatSpecs` (`game/src/Lang.cpp:73-87`) and Python `SPECS`/`specs()` (`tools/lang-check.py:25,30-31`) agree. I compiled `formatSpecs` on its own and fuzzed it against the Python regex with 20,000 random strings over the alphabet `%`, digits, `d s . l h 0 - + # z q t a X * é`: 0 mismatches.
  - The two read different forms of the text: C++ reads unescaped bytes, Python reads the escaped PO text. This doesn't change the result, because `\n`/`\xN`/`\"`/`\\` never contain `%` or flag characters.
  - Proof by scratch run: a copy of the checker with a `pl.po` that maps `lang_joinedTheGame` to `%i` fails with "changes the conversions ['%s'] to ['%i']", rc=1.
  - C++ test: `tests/test_lang.cpp:37-44` covers a changed type, an extra conversion, `%n`, a lone `%`, and keeps `%02i` and `%%`. That is good fix proof.
- **2 (`\t` escape): fixed in both parsers** (`Lang.cpp:63-65`, `lang-check.py:21`). Test at `test_lang.cpp:35`. A literal TAB byte still gets through (finding B).
- **3 (glyph warning): fixed.**
  - `LATIN1` in Python matches the C++ `LATIN1[32]` code points in the same order (checked by `ord()`).
  - Scratch run with `ż` prints the warning.
  - It misses control characters (finding B).
- **4 (repeated keyword): fixed, and both parsers agree.** C++ `seen[k]` at `Lang.cpp:186-187`, Python `keyword in cur` at `lang-check.py:56-57`. Both start a new entry when `msgctxt`/`msgid` follows a `msgstr`. Scratch run: "msgstr twice in one entry", rc=1. C++ test at `test_lang.cpp:34`.
- **5 (parsePo restructure): fixed.**
  - The `field[3]`/`seen[3]`/`flush` lambda is clearly simpler than the old `next`/`haveStr`/pointer state machine.
  - End of input is now a plain `flush()` after the loop. The header entry (no `msgctxt`) is still skipped.
- **6 (double parse): documented** at `Lang.h:19-21`. Accepted.
- **7 / 9 / 10:** noted as intended. The guide note is `content/languages/README.md:21`.
- **8 (docs):** the ARCHITECTURE rows for `Lang.h`, `test_lang.cpp`, `lang-check.py` and `step13-report.md` are updated, and the step context is fixed. Leaving the ADR 0011 line reference as it was is consistent with ADRs recording the context at decision time.
- **Checks run:**
  - `python3 tools/lang-check.py` on the real tree: "ok: translations".
  - Sizes: `Lang.cpp` 240 lines, `lang-check.py` 105.

### Findings

1. **Medium.** `game/src/Lang.cpp:158` (the `formatSpecs(field[1]) == formatSpecs(field[2])` check in `flush`). C++ compares the translation's conversions with the PO's own `msgid`, not with the English text the game formats with.
   - The game passes the text to `CString(fmt, ...)` with the argument types of the English key from `en.lang`.
   - `msgid` comes from the same file the translator controls. A `.po` with `msgid "%i joined"` / `msgstr "%i joined"` for `lang_joinedTheGame` (English `%s`) passes the C++ check, and the game formats an `int` as `%s`.
   - `lang-check.py:88` compares with `en.lang` and catches it, but only for committed files. A hand-set `languageFile` or an auto-picked `.po` dropped into `main/languages/` gets no protection. That protection was the stated reason for the C++ half of fix 1.
   - Neither tool checks `msgid` against `en.lang`. My scratch `.po` used `msgid "x"` and nothing flagged it.
   - **Fix (also a simplification):** `load()` already reads `en.lang` into `out` first. In `flush`, compare with what is already there: `auto en = out.find(field[0]); if (en != out.end() && formatSpecs(en->second) == formatSpecs(field[2])) …`. Drop `msgid` from the decision (keep it as a parsed field only), so C++ and Python use the same reference.
   - Update the `Lang.h:14-15` comment and the test. The test would seed `f` with the English values first, which the `load` contract already guarantees.
   - Optionally, `lang-check.py` could fail when `msgid` differs from `en.lang`. That flags a stale `.po`, but it isn't needed once the comparison is against `en.lang`.

2. **Low.** `game/src/Lang.cpp:55-67` (`unquote`), `Lang.cpp:20` (`fontByte`), and `tools/lang-check.py:24,92`. A literal TAB byte inside a quoted PO string still becomes byte 9, the colour code `\x9`.
   - Round 1 finding 2 was about this effect. The fix closed only the `\t` escape.
   - Python `QUOTED` accepts a raw tab. The glyph warning only looks at `ord(c) > 127`, so it doesn't flag it.
   - Scratch proof: `msgstr "%i joined<TAB>ż"` produced no complaint about the tab.
   - The same gap applies to raw bytes 1-8 and other control characters. `fontByte` accepts 1-9, and Python never flags characters below 32.
   - The new test message "\\t rejected (byte 9 is a colour code)" (`tests/test_lang.cpp:35`) suggests this is fully covered, which it isn't.
   - **Fix:**
     - In `unquote`, return false for any `c < 32` before the escape handling (`if ((unsigned char)c < 32) return false;`).
     - In Python, add `or re.search(r'[\x00-\x1f]', m.group(1))` to the "bad string" check.
     - Add one raw-tab case next to `test_lang.cpp:35`.

3. **Low (tests, REVIEW.md §2: "New behaviour has a test").** `tools/lang-check.py:56-57,87-93`: the checker's new behaviour has no automated test.
   - The new behaviour: failing on changed conversions, a repeated keyword and `\t`, and the glyph warning.
   - Nothing in `tests/` or CI runs `lang-check.py` on a bad `.po`. It runs only on the real tree, which has no `.po`, so every new branch is dead code in CI.
   - The C++ side has tests. The Python fix proof exists only if the record shows a manual scratch run; I reproduced one above.
   - **Fix:** either
     - add a small ctest that runs `lang-check.py` against a scratch `content/languages` directory with three bad `.po` files (a `LANG_DIR` override via env or argv, a few lines), or
     - record the scratch-run output as fix proof in `reviews/step13.md`.

4. **Low (docs).** `tools/check-hygiene.py:8`: the docstring still says "Translations: tools/lang-check.py (en.pot in sync, valid PO, no unknown keys)". It now also fails on changed conversions and warns about glyphs.
   - **Fix:** shorten it to "Translations: tools/lang-check.py", since the details already live in that file's docstring.

5. **Info (structure).** `tools/lang-check.py:85-93`: `main()` now holds the unknown-key, conversion, glyph and untranslated checks inline in one loop. At 105 lines this is still fine, so no action is needed now. If finding 2 or another check is added, a `check_po(name, entries, english) -> bad` function would keep `main` as orchestration only.

6. **Info (repeat check).** None of the findings above repeats a finding that an earlier phase B record rejected or deferred. Findings 1 and 2 are incomplete fixes of round 1 findings 1 and 2, not new issues.

Commit hygiene (REVIEW §2):
- The fix commits keep the C++ changes (fa1e951) and Python changes (2b70477) separate, with no mechanical rewrites mixed in.
- ARCHITECTURE and the README are updated.
- No ADR is needed: rejecting a translation whose conversions differ from the English is covered by ADR 0011's per-key fallback.

### Verdict

**Not yet closable:**
- Fixes 2-8 are correct, and the C++ and Python conversion extraction agree (fuzz-verified).
- Fix 1's runtime guard compares with the wrong reference (`msgid` instead of `en.lang`), so a hand-supplied `.po` can still crash the game.
- Fix 2 leaves raw control bytes open.

Fixing finding 1 (comparing with `out`) makes the code smaller, not larger. Findings 2 and 3 are small. Findings 4-5 are optional.

## Round 3

SKILL: Launching skill: thermo-nuclear-code-quality-review

## Round 3 review: step 13 round 2 fixes (`git diff bb3d697..HEAD`: f5c5c0a, 185c23e, 4126f19)

This was a read-only review in `/Users/piotr/Dev/BaboViolent_2_relaunch/RoboViolence2-step13`. I edited nothing; scratch files are under `/private/tmp/claude-501/r3`.

### Verification of the round 2 fixes

- **1. Conversions now checked against the English text: fixed.**
  - `game/src/Lang.cpp:162-164`: `flush` looks up `out.find(field[0])` and compares `formatSpecs(en->second)` with the msgstr. `msgid` no longer decides anything.
  - Keys that English lacks are skipped.
  - `pickFile` seeds English before its glyph check (`Lang.cpp:227`).
  - Fix proof: I put back the old `formatSpecs(field[1])` comparison, rebuilt `test_lang`, and it fails with "FAIL a msgid that agrees with a bad msgstr doesn't count".
  - The `Lang.h:14-16` comment and `tests/test_lang.cpp` (seeded `Texts`, untranslated entry keeps English, unknown key skipped) are updated.
  - The change is smaller than the old logic, as round 2 predicted.
  - Side effect, an improvement: the `pickFile` glyph test now counts only entries that are actually applied.
- **2. Raw control bytes: fixed in both parsers.**
  - C++: `Lang.cpp:56-57`.
  - Python: `tools/lang-check.py:60`, `[\x00-\x1f]`.
  - Fix proof (C++): with the C++ check removed, the tests "raw tab rejected" and "raw colour byte rejected" fail.
  - Fix proof (Python): with the Python check removed, `test_lang_check.py` fails with "FAIL raw tab: exit 0".
  - A manual run with a raw `\x02` in a `.po` gives "bad string", rc=1.
- **3. Checker test: fixed.**
  - New `tests/test_lang_check.py` (54 lines), registered as ctest `lang_check` (`tests/CMakeLists.txt:146-147`).
  - It covers a good file, a changed conversion, an unknown key, a repeated keyword, a `\t` escape, a raw tab, the glyph warning (rc 0 plus the warning text) and a stale `en.pot`.
  - `lang-check.py` and `lang-to-pot.pot_text` take a languages dir, a default-argument change of a few lines.
  - It passes locally: "ok: lang_check".
- **4. check-hygiene docstring: fixed** (`tools/check-hygiene.py:8`).
- **5. `check_po` extracted: done.** `main()` (`tools/lang-check.py:88-104`) now only orchestrates. File is 108 lines.
- **Checks run:**
  - Locally built `test_lang` (`c++ -std=c++17 -Wall -Wextra`): "ok: lang", no warnings.
  - `tools/lang-check.py`: "ok: translations".
  - `tools/check-hygiene.py`: "ok: hygiene".
  - `tools/check-architecture.sh`: "ok".
  - Sizes: `Lang.cpp` 245 lines, `lang-check.py` 108, both far from 1k.

### Findings

1. **Low (structure).** `game/src/Lang.cpp:225-227`: `pickFile` re-reads and re-parses `en.lang` once per candidate name inside the nested loop: up to two reads per locale.
   - **Fix:** read it once before the loops (`Texts english; readLang(dir + "en.lang", english);`), then use `Texts texts = english;` per candidate. That is one line moved and one line changed.
2. **Low (legibility).** `game/src/Lang.h:14-16`: the rewrapped comment has a 151-column line 15 followed by a short line 16. The wrap is clearly broken; `.clang-format` has `ColumnLimit: 0`, so no tool will fix it.
   - **Fix:** rewrap to about 120 columns like the surrounding comments.
3. **Low (test coverage).** `tests/test_lang_check.py:17-25`: the Python test covers a raw tab but not a raw colour byte 1-8.
   - The C++ test covers both. I checked `\x02` against the Python checker by hand, and it is rejected.
   - **Fix (optional):** add `("raw colour byte", ENTRY.format(key="lang_quit", text="a\x02b"), True, "bad string")`.
4. **Info (build).** `tests/CMakeLists.txt:146`: the new unconditional `find_package(Python3 REQUIRED COMPONENTS Interpreter)` makes the later conditional `find_package(Python3 …)` calls in the same file redundant. This is harmless.
   - **Fix (optional):** move the one `find_package` to the top of `tests/CMakeLists.txt` and drop the repeats.
5. **Info (docs).** `ARCHITECTURE.md:113`: the `tests/test_lang.cpp` row still lists only "an entry that changes the printf conversions is skipped". It now also covers keys English lacks, conversions compared with English rather than msgid, and raw control bytes.
   - **Fix (optional):** append "keys English lacks and raw control bytes rejected/skipped".
6. **Process (REVIEW.md §5, not a code defect).**
   - `docs/roadmap/phase-b-security-infrastructure-anti-cheat/reviews/step13.md` doesn't exist yet.
   - The step's **Status** is still `IN PROGRESS` (`step13-localisation.md:3`).
   - Both are needed to close the step, along with the `possible-new-scope.md` entry if there is anything to add. The fix proofs above (mutation runs) can go in its "Fix proof" section.
7. **Info (repeat check).** None of the findings above repeats a finding that an earlier phase B record rejected or deferred. Rounds 1 and 2 findings 1-5 are all resolved; none reopened.

REVIEW.md §2:
- New behaviour has tests, with fix proof shown by mutation.
- Each commit keeps C++ and Python fixes separate (f5c5c0a is C++ plus its test; 185c23e is Python plus its test and harness), and none mixes in mechanical rewrites.
- ARCHITECTURE lists the new file.
- No new non-obvious decision: ADR 0011's per-key English fallback covers rejecting translations that don't match English.
- There is no asset or **[GUI]** impact.

### Verdict

**Closable.** Every round 2 finding is fixed and has fix proof. No structural regression, spaghetti growth or file-size problem remains. Findings 1-2 are small cleanups worth doing before the PR; 3-5 are optional. Closing still needs the `reviews/step13.md` record and the DONE status (finding 6).

## Round 4

SKILL: Launching skill: thermo-nuclear-code-quality-review

## Round 4 review: step 13 round 3 cleanups (`git diff 4126f19..HEAD`: 38dad0d, 85ff776)

This was a read-only review in the step13 worktree. Scratch files are under `/private/tmp/claude-501/r4`.

### Verification
- **R3-1 (pickFile re-reads en.lang): fixed.**
  - `game/src/Lang.cpp:217-218`: `en.lang` is now read once before the loops. Each candidate gets `Texts texts = english;` (`Lang.cpp:227`).
  - Behaviour is unchanged. Each candidate still starts from a fresh English copy, so one failed `.po` can't leak entries into the next. A missing `en.lang` still gives an empty base, as before.
- **R3-2 (Lang.h comment wrap): fixed.** `game/src/Lang.h:14-16` is rewrapped to about 118 columns, matching the surrounding comments.
- **R3-3 (raw colour byte in the Python test): fixed.** `tests/test_lang_check.py:23` adds a `\x02` case that expects "bad string".
- **R3-5 (ARCHITECTURE test_lang row): fixed.** `ARCHITECTURE.md:113` now lists keys English lacks, conversions compared with English, and raw control bytes.
- **R3-4 (repeated `find_package` calls in `tests/CMakeLists.txt`): left undone on purpose.** It was only informational, so I don't raise it again.
- **Checks run:**
  - `test_lang` built with `c++ -std=c++17 -Wall -Wextra`: no warnings, "ok: lang".
  - `test_lang_check.py`: "ok: lang_check".
  - `check-architecture.sh`: ok.
  - `check-hygiene.py`: ok.
  - `Lang.cpp` is 246 lines.

### Findings
1. **Info (docs).** `ARCHITECTURE.md:114`: the `tests/test_lang_check.py` row says "tabs". It now also covers a raw colour byte.
   - **Fix (optional):** change "tabs" to "tabs and raw control bytes".
2. **Process (REVIEW.md §5, carried from R3-6).** The step can't close until these are done:
   - `docs/roadmap/phase-b-security-infrastructure-anti-cheat/reviews/step13.md` doesn't exist yet.
   - `step13-localisation.md:3` still says `IN PROGRESS`.
   - Add a `possible-new-scope.md` entry if there is anything to note.
3. **None of the above repeats a finding that an earlier record rejected or deferred.**
4. **Structural review of the diff:** nothing to flag. The diff is net smaller code (one read moved out of the loop) with no new branches. No file grows meaningfully.

REVIEW.md §2:
- The new test case follows the existing table pattern.
- Commits are separated as before: C++ and header in 38dad0d; test, docs and report in 85ff776.
- Neither commit mixes in mechanical rewrites.
- No new decision needs an ADR.
- There is no asset or [GUI] impact.

### Verdict
**Closable** once the `reviews/step13.md` record and the DONE status are in place (finding 2). Finding 1 is optional.
