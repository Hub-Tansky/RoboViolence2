SKILL: Launching skill: thermo-nuclear-code-quality-review

Fresh-context review of Phase C step 5 (branch refactor/phase-c-step5-client-engine-fixes, diff origin/main..HEAD, 6 commits). No files modified.

Overload checks done (clang, C++17/20, scratch test programs):
- Copy-init (function argument, `=`, return) from a literal, a `char[N]` or a `const char[N]`: picks the template, copies the text verbatim. Copy-init from a `char*`: rejected. Matches the static_asserts in the test.
- Direct-init `CString("lit")` and `CString(constArray)`: template, verbatim.
- **Direct-init `CString(nonConstCharArray)`, `CString{buf}` and `CString(packet.msg)`: the FORMAT constructor.** Both candidates are Exact Match. Array-to-pointer is an lvalue transformation, so the tie goes to the non-template.
- `operator==` / `!=` with a literal still pick the `const char*` overload. `operator+` with a literal still picks `operator+(char*)`. Both unchanged.
- `strnlen` is in `<string.h>` on MSVC UCRT, and in glibc under g++'s default `_GNU_SOURCE`, so there is no portability problem.
- No implicit literal relied on `%%` escaping (grep), so changing literal conversion to verbatim regresses nothing.

## Findings

1. **High.** Direct-initialised call sites still run untrusted text as a printf format. `explicit` only makes the compiler catch implicit conversions. Any `CString(ptr)`, and any `CString(nonConstArray)` (see above), still uses the format constructor. Remaining reachable sites:
   - `engine/zeven/src/dksvar.cpp:39`: `CString strCommand(str_command)` runs on every `set` command. That includes a server's `NET_SVCL_SV_CHANGE` (`ClientRecv.cpp:569` → `Console::svChange` → `dksvarCommand(command.s)`, `Console.cpp:576`). The caller's `CString("%s", …)` wrapping is undone here, so a malicious server can crash every client that joins with `set sv_hostname %n%s`. Remote-admin and admin console commands reach the same line.
   - `game/src/GameShowStats.cpp:73-79`: `CString(text1)`, where `text1` is `showName.s` (a player name from the wire). Any player whose name contains `%s` can crash every client that opens the scoreboard. No `%` filter on names was found.
   - `game/src/CMaster.cpp:444`: `CString(command.command)`, a `char[128]` remote-admin wire array.
   - `game/src/CMaster.cpp:791`: `CString((char*)buffer + 2)`, master-server wire data.
   - `game/src/Console.cpp:379`: `CString textCpy(m_currentText->s)`, typed console text.
   - Lower risk: `Console.cpp:278/445` (`recognitionVar[]`, var names) and `dksvar.cpp:58` (`varName`).
   - Fix, preferred: make the constructor choice itself correct. Turning the format constructor into `template <class P, class = std::enable_if_t<std::is_same<P, char*>::value>> explicit CString(P fmt, ...)` makes arrays (const or not) and literals pick the verbatim constructor, while `char*` still formats. This was verified in a scratch test.
   - Fix, alternatively: change the sites above to `CString("%s", x)`. `dksvar.cpp` and the non-flagged `game/src` sites are outside the step's Allowed paths ("call sites the explicit constructor flags"), so widen the scope in the step file rather than leaving the PNS-34 goal ("can no longer … by accident") half met.
   - Either way, add a direct-init case to `tests/test_cstring.cpp`, e.g. `char wire[16]="%s%n"; CString d(wire);` must equal the text. It fails today.

2. **Medium.** The documentation states the wrong contract. `engine/zeven/include/CString.h:64-65` ("never used as a printf format"), `AGENTS.md:70` ("char arrays convert verbatim") and the `test_cstring.cpp` header all say char arrays are safe. That holds only for copy-init, so a future author writing `CString(packet.field)` will think it is safe. Fix: correct these together with finding 1, or after it.

3. **Medium (REVIEW §2, fix proof).** The destructor half of R29 has no test that runs it. `test_crash_fixes` links `bv2_server_harness`, which is built with `CONSOLE`, and `~_typLayer`'s body is `#ifndef CONSOLE`. So the comment at `tests/test_crash_fixes.cpp:119` ("layers without a texture make no GL call") claims something the test cannot observe. The fix proof covers only the null-layer half. Fix: reword the comment, and record in `reviews/step5.md` that the GL half is verified by inspection or a manual client run (it can't be tested headless).

4. **Low.** `game/src/GameVar.cpp:1250-1251` has two consecutive `if (file.empty())` checks. Merge them into `if (file.empty()) { languageFile = ""; file = lang::pickFile(...); } else file = bv2::contentFile(file);`.

5. **Low.** "Saved back empty" (PNS-47) is tested only through the pure `lang::savedFile`. The `languageFile = ""` write in `reloadLanguage` has no test. This is acceptable under the MVP rule. Note it in the step record.

6. **Low (REVIEW §2, ADR).** Implicit conversion from a literal or array changed from format to verbatim, and `char*` must now be wrapped. This is a project-wide non-obvious rule with no ADR; only the step file and `AGENTS.md` record it. Add a short ADR, or justify in the record why the step file is enough.

7. **Info, out of scope.** In `ePTexture::loadMap`, an unknown `CHUNK_MAP_NAME` leaves `ptrLayer` on the previous layer. A second texture chunk for the same layer overwrites `textureID` without freeing it, which is a leak and not a crash. Record it as a PNS.

8. **Info.** The `BV2_WITH_HTTP=ON` build (`game/src/CCurl.cpp`) is not compiled in CI, so the compiler never checked it for flagged conversions. A grep finds no implicit `char*`→`CString` there. The changed `Server.cpp:901` CCurl site also fixes URL `%20` mangling.

9. **Info.** `Lang`, the `r_highDetailMenu` default and config, the `GameVar` weapon-name `.s` removals, and the `dkf`/`CBrowser`/`CFriends`/`Client`/`Server` wraps are correct and minimal. The scratch layer for R29 is a clean, minimal fix. `ARCHITECTURE.md` lists the new test and step file. Commits are one per task, with nothing mechanical mixed in.

## Verdict

**Not approved.** Tasks 5.2–5.4 are fine apart from findings 3–6. Task 5.1 leaves the PNS-34 bug class open: a non-const char array passed by direct-init still formats, and a malicious server (`dksvar.cpp:39`) or any player name (`GameShowStats.cpp:73`) can still crash clients. The new docs also overstate the guarantee. Fix finding 1 (preferably with the `enable_if` constructor plus a direct-init test) and finding 2, then re-review.

## Round 2

SKILL: Launching skill: thermo-nuclear-code-quality-review

Round 2 review, Phase C step 5 (`git diff 1b4282d..HEAD` and `origin/main...HEAD`). I modified no files. I left out the files that came in from main. On macos-arm64, ctest `cstring`, `crash_fixes` and `lang` pass.

### C++ check of the new constructors
- **One argument.** `CString(x)` and implicit conversion both go to `CString(const char*)`, because the template needs at least two arguments. Literals, `char[N]` (direct or implicit), `char*` and `const char*` are all copied as is.
- **`0`, `NULL` and `nullptr`** become a null `const char*` (a standard conversion), which beats the copy constructor and gives "".
- **Other single values.** `CString x = 5` and `CString('a')` do not compile, the same as before. `x = 0` still picks `operator=(int)`.
- **Two or more arguments** go to the template. Arguments are taken by value, so arrays decay to pointers and float, char and bool get the normal vararg promotions.
- **A `CString` or other non-trivial type as a format argument.** I checked this with Apple Clang: `CString("%s", a)` gives "error: cannot pass object of non-trivial type 'CString' through variadic method" (`-Wnon-pod-varargs`, an error by default). GCC also rejects it. MSVC accepts it, with at most C4840 at /W4.
- **Formatting call sites.** Every format call inside `CString.cpp` (lines 94, 221, 243, 285, 351, 596, 625) and `Player.cpp:645` passes two or more arguments, so it still formats.
- **One-argument literals with `%`.** `grep 'CString("…%…")'` finds none. No `%%` in `content/languages`.
- **Round-1 sites.** `dksvar.cpp:39`, `GameShowStats.cpp:73-79`, `CMaster.cpp:444/791` and `Console.cpp:379` now copy verbatim, and none of them needed formatting.
- **`set()`.** No `CString::set` call passes a single non-literal argument.

### Findings
1. **Medium (simplification).** The `CString("%s", x)` wrappers added in round 1 are now redundant leftovers of the `explicit` design, because a `char*` passed to a `CString` parameter is copied as is under ADR 0021.
   - Sites: `engine/zeven/src/dkf.cpp:62`, `game/src/CBrowser.cpp:108`, `game/src/CFriends.cpp:97`, `game/src/Client.cpp:410,412`, `game/src/Server.cpp:901`.
   - Fix: revert those lines to the originals. In `Server.cpp`, pass `reportUploadURLs[i].c_str()` and drop the `(char*)` cast. The diff shrinks and the code keeps a single idiom.
   - Optional: the same applies to the `.s` removals in `game/src/GameVar.cpp:264-276`; they are harmless and slightly cleaner, so keep or revert.
2. **Low (stale docs, same cause).**
   - In the step file `step5-client-engine-fixes.md`, task 5.1 says "The char* sites found on the way use `CString("%s", text)`", and Allowed paths says "call sites the explicit constructor flags". Neither matches the design any more. Update both after #1.
   - The PNS-34 title in `docs/roadmap/possible-new-scope.md:265` ("Make … explicit") could point to the outcome in ADR 0021.
3. **Low (ADR accuracy).** `docs/decisions/0021-cstring-one-argument-is-text.md` says passing a `CString` as a format argument "is a compile error". That holds on GCC and Clang only; MSVC compiles it and passes the object bit by bit. CI catches it only because the Linux and macOS builds are required. Fix: say "a compile error on GCC/Clang (MSVC accepts it)".
4. **Low (undocumented behaviour change).** The one-argument constructor no longer caps text at `MAX_CARAC - 1` (511 characters); only `format()` truncates now. I checked the places that copy a `CString` into fixed buffers and found no overflow:
   - `Console.cpp:1522` is limited by `resize(79)`.
   - `dksvar.cpp:97` takes its matches from registered names.
   - Fix: add one Consequences line to ADR 0021.
5. **Info (no action, out of scope).** `CString::operator=(const char*)` (`CString.h:160`) still runs `strlen` on a null pointer, while the constructor turns null into "". The step's scope excludes `operator=`, so a PNS entry is enough.
6. **Info.** The "a `CString` argument fails to compile" property cannot be tested at runtime. Finding 3 covers it in the ADR, which is enough.

### REVIEW.md section 2
- **Tests with fix proof:**
  - `test_cstring` covers direct and implicit one-argument forms (array, `char*`, literal, null) and formatting with several arguments. Round 1 claimed it aborts (exit 134) on that round's CString; I did not rerun this.
  - In `crash_fixes`, `r29` covers the null layer. The destructor half is checked by reading the code, which round 1 accepted.
  - In `lang`, `savedFile` is tested. The write-back of the empty `languageFile` is not, which round 1 accepted.
- **Docs and ADR:** `ARCHITECTURE.md` lists the test and ADR 0021. ADR 0021 exists, with the index, AGENTS.md and KEY_QUESTIONS R29 updated.
- **GUI change:** `r_highDetailMenu` is owner-approved (PNS-30).
- **Commits:** no mechanical rewrite is mixed with functional changes.

### Verdict
The constructor design is sound and closes round-1 finding #1 for every way of passing the text. I found no correctness problems. Approve after #1 (drop the redundant wrappers) and the small doc fixes in #2–#4.
