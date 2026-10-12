# Step 13 review

Reviewer: fresh-context subagent, /thermo-nuclear-code-quality-review, four rounds (2026-10-12)
Diff: round 1 bbe93ad..e89b9dc; round 2 e89b9dc..bb3d697; round 3 bb3d697..4126f19; round 4 4126f19..85ff776
Skill evidence: every round `SKILL: Launching skill: thermo-nuclear-code-quality-review` (raw reports: [step13-report.md](step13-report.md))

## review.sh

`tools/review.sh --full macos-arm64` on the final head: architecture, hygiene, content-case, original-assets, secrets, diff-rules, configure, build, ctest all PASS (ctest includes `lang` and `lang_check`).

## Findings (round 1)

| # | Finding | Outcome |
|---|---|---|
| 1 | Translations reach printf formatting; placeholders unchecked | fixed in fa1e951, 2b70477 (completed in round 2) |
| 2 | PO `\t` becomes colour code `\x9` | fixed in fa1e951, 2b70477 (raw bytes in round 2) |
| 3 | No glyph warning in the checker | fixed in 2b70477 |
| 4 | C++ and Python disagree on a repeated keyword | fixed in fa1e951, 2b70477 (rejected in both) |
| 5 | `parsePo` sentinel-iteration loop | fixed in fa1e951 (flush lambda) |
| 6 | `pickFile` parses a PO that `load` parses again | rejected: one small file, once per start; documented in `Lang.h` |
| 7 | Commits don't match task items (13.1 logic in the 13.2 commit) | rejected: noted here; no history rewrite |
| 8 | Stale references (ADR 0011 line, step context, ARCHITECTURE row) | fixed in 8651e22 (step context, ARCHITECTURE); rejected for ADR 0011: an ADR records the context at decision time |
| 9 | Players with a saved `languageFile "main/languages/en.lang"` never get the OS language | fixed in 8651e22 (guide says to set it to `""`) |
| 10 | No test for `osLocales()` / `reloadLanguage()` | rejected: thin SDL and CString glue; `test_lang` covers the logic; closes step 2b round 2 #3 (language loader now has a test seam) |

## Findings (round 2)

| # | Finding | Outcome |
|---|---|---|
| 1 | Runtime conversion check compares with the PO's msgid, not the English text | fixed in f5c5c0a (compared with English in `out`; `pickFile` seeds English) |
| 2 | Raw control bytes in PO strings become colour codes | fixed in f5c5c0a, 185c23e |
| 3 | `lang-check.py` behaviour untested | fixed in 185c23e (`tests/test_lang_check.py`, ctest `lang_check`) |
| 4 | `check-hygiene.py` docstring stale | fixed in 185c23e |
| 5 | `lang-check.py` `main()` holds every check | fixed in 185c23e (`check_po`) |

## Findings (round 3)

| # | Finding | Outcome |
|---|---|---|
| 1 | `pickFile` reads `en.lang` per candidate | fixed in 38dad0d |
| 2 | Broken comment wrap in `Lang.h` | fixed in 38dad0d |
| 3 | Python test lacks a raw colour byte case | fixed in 85ff776 |
| 4 | Repeated `find_package(Python3)` in `tests/CMakeLists.txt` | rejected: harmless; touches other tests' setup |
| 5 | ARCHITECTURE `test_lang` row incomplete | fixed in 85ff776 |
| 6 | Record and status missing | fixed: this record |

## Findings (round 4)

| # | Finding | Outcome |
|---|---|---|
| 1 | ARCHITECTURE `test_lang_check.py` row omits raw colour bytes | rejected: wording only; the row names the test's purpose |
| 2 | Record and status missing | fixed: this record; status waits for the owner checks below |

## Fix proof

- Conversions against English (round 2 #1): with `formatSpecs(field[1])` restored in `flush`, `test_lang`: `FAIL a msgid that agrees with a bad msgstr doesn't count`.
- Conversions at all (round 1 #1): with the conversion comparison removed, `test_lang`: `FAIL only the entry with the same conversions is kept`.
- Raw control bytes, C++ (round 2 #2): with the `< 32` check removed, `test_lang`: `FAIL raw tab rejected`, `FAIL raw colour byte rejected`.
- Raw control bytes, Python (round 2 #2): with the `[\x00-\x1f]` check removed, `test_lang_check.py`: `FAIL raw tab: exit 0`.

## Not done / unsure

- Owner checks from the step's acceptance list: with `LANG=pl_PL.UTF-8` and an ASCII test `pl.po`, the Linux client shows Polish (without the PO, English); with original BV2 data, `en.lang` loads and the menus look unchanged. On macOS (2026-10-12) the client with `-AppleLanguages '(pl-PL)'` showed the test `pl.po` title "Robo Przemoc PL" with the untranslated menu in English, and "Babo Violent 2" without the PO; the Linux run is still open.
- Windows and Linux builds of `Lang.cpp` are checked only by CI.
