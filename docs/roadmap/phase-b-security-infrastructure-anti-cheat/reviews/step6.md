# Step 6 review

Reviewer: fresh-context subagent, /thermo-nuclear-code-quality-review, 2026-10-11
Diff: origin/main...fb56aa9 (9d44d14..fb56aa9)
Skill evidence: `SKILL: Launching skill: thermo-nuclear-code-quality-review` (raw report: [step6-report.md](step6-report.md))

## review.sh

`tools/review.sh --full macos-arm64` and `--full macos-arm64-asan` at fb56aa9: all checks PASS. After the fixes, both presets build and all 11 ctests pass locally. The acceptance grep lists only `game/src/Paths.cpp:283-284`, the constant DDL, with its comment.

## Findings

| # | Finding | Outcome |
|---|---|---|
| 1 | Acceptance grep still lists a commented-out `sqlite3_exec` | fixed in d42277c (dead block removed) |
| 2 | `content-seed/master.sql` still points at the removed `BV2_BAN_*` macros | fixed in d42277c |
| 3 | `SendBanList` (LIKE filter, explicit columns) untested | fixed in 009309a (empty filter, `Nick'`, `'--;%`) |
| 4 | No injection in a ban's IP | fixed in 009309a (`1';--%`) |
| 5 | Web `Games` write copied three times | fixed in d42277c (`WriteWebGame`) |
| 6 | `AddBan` repeats `IsBanned`'s query | fixed in d42277c (`FindBan`) |
| 7 | Single-value reads repeat a scope block | fixed in 847d47a, d42277c (`SqlStatement::value`) |
| 8 | Manual `sizeof` bounds | fixed in 847d47a (array overload bounds by `N`) |
| 9 | Prepare failures silent | fixed in 847d47a (logged to stderr) |
| 10 | Empty filter skips NULL nicks | accepted, noted in the step file (only hand-inserted rows) |
| 11 | Test closes the database with live statements | fixed in 009309a |
| 12 | The two `SqlStatement.h` copies can drift | fixed in 009309a (`sql_statement_copies` ctest) |
| 13 | Inventory row out of order | fixed in this commit |
| 14 | Commit subjects without `.M` (repeat) | rejected: pushed history not rewritten |

## Fix proof

- Q-S3, the old master code (`cMasterServer.cpp`, `cNetManager.cpp` reverted): `FAIL tests/test_master.cpp:129: games.row() && games.text(0) == name && games.text(1) == map`, `FAIL ...:147: answered`, `FAIL ...:152: bans.row() && ...`.
- Ban list (#3): with the IP and MAC columns swapped in `SendBanList`, `FAIL tests/test_master.cpp:171: listed` for the filters `""` and `"Nick'"`.

## Not done / unsure

- The master's other messages still `memcpy` whole structs without size checks (master packet hygiene, not this step).
- The ban messages are authorised by a password in the source (Q-S5, step 10).
- `masterserver/README.md` still describes `sprintf` SQL → PNS-36.
