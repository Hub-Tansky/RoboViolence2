# Step 6 raw reviewer reports

## Round 1

SKILL: Launching skill: thermo-nuclear-code-quality-review

**Scope reviewed:** `git diff origin/main...HEAD` on `refactor/phase-b-step6-sql-prepared-statements` (7 commits, 9d44d14..fb56aa9, 14 files). I checked it against the step 6 file (tasks 6.1–6.3, Notes, acceptance checks), REVIEW.md §2 and §5, and the step 1–5 records. I ran the acceptance grep, `cmp` on the two `SqlStatement.h` copies (identical), and checked the bound columns against `content-seed/master.sql`/`web.sql` and the `cMSstruct.h` structs (`stBV2row`, `stCacheBan`, `stCacheList`, `stCacheBanned`, `SSurvey`). I did not build or run ctest.

**Verified correct (no finding):**
- **Null `m_stmt` is safe.** `sqlite3_bind_*` on a NULL statement returns `SQLITE_MISUSE`, `row()`/`run()` guard on `m_stmt`, and `sqlite3_finalize(NULL)` does nothing.
- **Text binding:** `strnlen` with the `(size_t)-1` default is safe, and `SQLITE_TRANSIENT` is correct for temporaries and stack buffers.
- **The survey insert** has 20 columns, 20 placeholders and 20 binds, in order. Types match `SSurvey`, and all 7 text fields are terminated at `[79]` (`cNetManager.cpp:311-317`).
- **Explicit ban columns** match the `BanList` schema. The AddBan update and insert parameter order is right.
- **CheckBans deletes the current row while stepping the SELECT.** SQLite's isolation rules allow deleting the current row by rowid, and `ID` is `INTEGER PRIMARY KEY`.
- **NUL-termination:** the ban, list and banned messages are now terminated before binding.
- **Scoping:** the game-side read statements sit in blocks that finalize before `sqlite3_close`.
- **Formatting:** `GameVar.cpp` now uses `CString("%s", …)` for the database value.
- **Remaining SQL:** `tools/seed_db.cpp:36` runs `sqlite3_exec` only on the tracked seed files. No macros are left in `cMasterServer.h`.
- **Docs:** Q-S3 is marked fixed, and ARCHITECTURE.md lists both new files.

| # | Severity | Finding (file:line) | Suggested change |
|---|---|---|---|
| 1 | Medium (acceptance) | The acceptance grep still lists `masterserver/src/cMasterServer.cpp:96` `//sqlite3_exec(MasterDB,"Delete from Servers;",0,0,0);`. That line is commented-out, non-DDL SQL with no comment saying why it is allowed, so the check "Lists only constant DDL, each with a comment saying so" fails. It also contradicts the Note that commented-out SQL was deleted. | Delete the line (and the commented-out date block at `:88-93` with it). |
| 2 | Medium | `content-seed/master.sql:3` still says "Column order of BanList follows BV2_BAN_* in cMasterServer.h". 5be7edb deleted those macros, and the step Notes say the order never matched them. | Replace it with "Queries name their columns; order is free" or delete the line. |
| 3 | Medium (test strength, 6.3) | **`SendBanList` is untested.** It is the most rewritten query (`cMasterServer.cpp:105-119`): LIKE `'%' \|\| ? \|\| '%'`, 6 explicit columns and a new row-to-struct mapping. The test never sends `CACHE_BAN_LIST`. The `'%'` in the test data proves nothing on its own: the old `sprintf("%s")` never interpreted `%` in data, so `%` matters only in the LIKE path, which is not exercised. | In `tests/test_master.cpp`, send `stCacheList` with filters `""` and `"'--;%"` (or `"Nick'"`) and CHECK that a `CACHE_BAN` arrives with the stored Nick, MAC and IP. |
| 4 | Low (test strength, 6.3) | Task 6.3 says "names, **IPs** and MAC fields" carry `'`, `--`, `;`, `%`. The ban IP is the plain `"203.0.113.7"` (`tests/test_master.cpp:134`). Games.IP comes from the connection, so it can't be injected, but `stCacheBan.IP` can. | Use an IP like `"1';--%"` (it fits `IP[16]`) and select the row by Nick or MAC. |
| 5 | Medium (code judo / duplication) | **The web Games write is copied 3 times.** The identical `Update Games …` statement plus 7 binds sits at `cMasterServer.cpp:413-416` and `:438-441`, and the insert at `:476-479`. The step deleted ~60 lines of escaping around each copy, so this duplication is now the only bulk left. | Add `void cMasterServer::WriteWebGame(const stBV2row & g, bool insert)`, or two small helpers, and call them from the 3 sites. |
| 6 | Low (code judo) | **AddBan repeats IsBanned's query.** `Select ID From BanList where IP = ? or MAC = ?` is prepared in both `IsBanned` (`:126`) and `AddBan` (`:152`). | Add `long long FindBan(IP, MAC)` (returns -1 if none). `IsBanned` becomes `FindBan(...) >= 0`, and `AddBan` uses the returned ID. |
| 7 | Low (code judo) | **Six single-value reads repeat the same 4-line scope block** (`Select Value … Where Name = '…'`): `GameVar.cpp:39-48`, `Scene.cpp:126-130`, `CMaster.cpp:974-978`, `cMasterServer.cpp:362-366` and `:370-371`. | Add `static std::string SqlStatement::value(sqlite3*, const char* sql)` (an empty string when there is no row) to both copies. Each site becomes one line and needs no brace scope. |
| 8 | Low (type boundary) | **The `sizeof` bound is manual and inconsistent.** `bindText(i, text, max)` relies on each caller passing `sizeof(field)` for wire arrays. `:265` binds `G->GameInfos.ip` without it, while `:416/441/477` pass it for the same field. That is safe today because `cBV2game.cpp:28` fills `ip` with `sprintf`, but the next caller can forget the bound. | Add a `template<size_t N> bindText(int i, const char (&text)[N])` overload that bounds by `N` automatically. All the `sizeof(...)` arguments then disappear. |
| 9 | Low | **Prepare and step failures are silent.** The constructor ignores `sqlite3_prepare_v2`'s result (`SqlStatement.h:35`), and every `run()` result is discarded. A schema drift (for example a `survey.db` without `SurveyEntries`, or a renamed `BanList` column) silently drops writes or turns `IsBanned` into "not banned". This behaves as before, but the class is now the single place where one line could fix it. | On prepare failure, `fprintf(stderr, "SQL: %s\n", sqlite3_errmsg(db))` (both copies). |
| 10 | Low | **Empty-filter behaviour changed slightly.** `Nick LIKE '%' \|\| '' \|\| '%'` excludes rows whose Nick is NULL, which the old `Select *` returned (`cMasterServer.cpp:105`). AddBan always binds a Nick, so only hand-inserted rows are affected. | Accept and note it, or use `Where ? = '' or Nick LIKE …`. |
| 11 | Low | **The test closes connections with live statements.** `tests/test_master.cpp:128-130` and `:151-153` call `sqlite3_close` while `games`/`bans` are still alive. The close returns `SQLITE_BUSY` and the connection leaks. This is the same pattern the step fixes in the game with scope blocks. | Close after the statement's scope ends: an inner block, or `sqlite3_close` after `}`. |
| 12 | Low | **Two identical copies of `SqlStatement.h` can drift.** The step sanctions the copies, but nothing checks that they stay identical. | One line in an existing test or `tools/review.sh` comparing them, or a PNS entry for the shared library. |
| 13 | Low (doc) | **ARCHITECTURE.md misplaces the new row.** `game/src/SqlStatement.h` sits between `Scene.h` and `SceneNet.cpp` (`ARCHITECTURE.md:625`); alphabetical order puts it after the `Server*` rows. | Move the row. |
| 14 | Low (repeat) | **Commit subjects without `.M`:** `step6:` in 9d44d14 and fb56aa9. This repeats step2a #8, step2 #10, step3 #11 and step4 #14, all rejected (pushed history not rewritten). | None. Flagged as a repeat only. |

**REVIEW.md §2:**
- No GUI or asset change, and no mechanical rewrite is mixed with functional changes. Removing the macros is its own commit.
- New behaviour has a test (the injection test), and the author reports fix proof on the old code. Its coverage gaps are #3 and #4.
- The `masterserver/README.md` drift is deferred to PNS-36 correctly.
- No ADR is needed: switching to binding has no rejected alternative.
- Blocking for DONE: #1 (the acceptance grep) and #2 (a stale fact in an allowed path). #3 should be fixed because it covers the most changed function.
