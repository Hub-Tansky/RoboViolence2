# Step 5 review

Reviewer: fresh-context subagent, /thermo-nuclear-code-quality-review, two rounds (2026-10-17)
Diff: round 1 origin/main..1b4282d; round 2 1b4282d..64d1a65
Skill evidence: both rounds `SKILL: Launching skill: thermo-nuclear-code-quality-review` (raw reports: [step5-report.md](step5-report.md))

## review.sh and tests

- macOS Debug: build clean; ctest 25/25 (`master` excluded locally: another process held port 10207; CI runs it).
- Fix proofs: `test_cstring` aborts (exit 134, the `%n` in the text) with round 1's `CString`; `crash_fixes` segfaults in `r29` with the old `ePTexture.cpp`.

## Findings (round 1)

| # | Finding | Outcome |
|---|---|---|
| 1 | Direct-init `CString(array)` / `CString(ptr)` still formats (server `set`, player names, master data) | fixed: one argument is text, two or more format (ADR 0021); `test_cstring` covers direct and implicit forms |
| 2 | Docs overstate the contract | fixed (`CString.h`, `AGENTS.md`, test header, ADR 0021) |
| 3 | R29 destructor half untested in the CONSOLE harness | accepted: comment reworded; the condition is checked by reading the code (no GL context headless) |
| 4 | Two `if (file.empty())` | fixed |
| 5 | `languageFile = ""` write untested | accepted: `lang::savedFile` is tested; the write is one line in `reloadLanguage` |
| 6 | No ADR | fixed (ADR 0021) |
| 7 | DKT texture after an unknown map name leaks | deferred: PNS-64 |
| 8 | HTTP build not compiled in CI | accepted: no affected call site (reviewer's grep) |

## Findings (round 2)

| # | Finding | Outcome |
|---|---|---|
| 1 | `CString("%s", x)` wrappers now redundant | fixed: reverted to the original calls |
| 2 | Step file and PNS-34 title stale | fixed |
| 3 | ADR: MSVC accepts a `CString` vararg | fixed (ADR 0021 consequences) |
| 4 | One-argument text no longer capped at 511 | fixed (ADR 0021 consequences); reviewer found no fixed-buffer overflow |
| 5 | `operator=(const char*)` with null | deferred: PNS-65 |
| 6 | Vararg compile error not testable | accepted (ADR) |

## Not done / unsure

- The client was not run on screen for the menu default; it is a default value only.
