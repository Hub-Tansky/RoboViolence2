# Step 2a raw reviewer reports

## Round 1

SKILL: Launching skill: thermo-nuclear-code-quality-review

Scope reviewed: `git diff 5e6ec03..59bacb7` (8 commits, 16 files). `tools/review.sh` (fast mode) passes locally at 59bacb7. I checked the Windows job log of CI run 38002113508: `smoke_server_utf8_path` passed in 0.62 s.

| # | Severity | Finding (file:line) | Suggested change |
|---|---|---|---|
| 1 | High (wrong fact in living docs) | `docs/build/windows.md:5` says "CI builds and tests on Windows Server 2022", and `docs/roadmap/phase-b-security-infrastructure-anti-cheat/README.md:47` says "CI builds on Windows Server 2022". The CI log of run 38002113508 shows `Operating System: Microsoft Windows Server 2025`, `Image: windows-2025-vs2026`, SDK 10.0.26100. That is build 26100, the Windows 11 24H2 generation, not a Windows 10-generation kernel. The step file context makes the same wrong claim (build 20348). The risk wording "Windows 11 shares its API level, and CI builds on Server 2022" therefore gets the coverage backwards: CI exercises the Windows 11-generation kernel, and nothing in CI runs on a Windows 10-generation kernel. | Say "Windows Server 2025 (`windows-latest`, Windows 11 24H2 generation)" in both docs. In the ADR or risk row, state that only the owner's machine covers Windows 10 at runtime, and that NTDDI_VERSION is the only Windows 10 guard in CI. Optionally pin `windows-2025` instead of `windows-latest` so the fact can't drift silently. |
| 2 | Medium (code judo / duplication) | `tests/smoke_server_utf8_path.py:19-32` copies the run, timeout and decode/print block of `tests/smoke_server.py:12-21`, but leaves out its exit-code check and its `bv2.cfg`-in-pref check. Meanwhile `smoke_server.py:25` keeps the weaker banner-only check, which step 1 finding 3 already called too early for `package-check`. The step allowed extending `smoke_server.py`. | Fold the new test into `smoke_server.py` with an optional `--install-under <dir>` argument: copy the exe and `main/` into `<dir>/Łódź`, run with `cwd=<dir>`, unset `BV2_DATA_DIR`. Both ctests then share one flow with every check (exit code, "Server Created on port", `bv2.cfg`). That deletes the second script and its ARCHITECTURE row. |
| 3 | Medium (ADR accuracy) | `docs/decisions/0012-windows-10-22h2-minimum.md:9-10` cites `CMakeLists.txt:70`, `bv2.manifest:6` and `bv2.manifest:13`. This diff shifted all three: `_WIN32_WINNT` is now on `CMakeLists.txt:71` (70 is the new comment), `supportedOS` on `bv2.manifest:7`, `activeCodePage` on `bv2.manifest:14`. | Update the line numbers, or cite by name (`bv2_target_defaults`, `<activeCodePage>`) so later edits don't break them. |
| 4 | Low (overstated claim) | `CMakeLists.txt:70` and ADR 0012 line 17 say a Windows 11-only API "fails to compile". `NTDDI_VERSION` only hides declarations the SDK headers guard with `NTDDI_WIN10_CO`/`NI` etc. Unguarded declarations, WinRT and `GetProcAddress` lookups still compile. The value is correct: `0x0A000008` = `NTDDI_WIN10_VB` (2004; 20H2–22H2 are enablement packages on the same API surface). It is also consistent with `_WIN32_WINNT=0x0A00`, so the sdkddkver mismatch check passes. | Say "a Windows 11 API that the SDK guards by NTDDI_VERSION fails to compile". |
| 5 | Low (test robustness) | `tests/smoke_server_utf8_path.py:22`: nothing asserts that `<scratch>` (the cwd, which is the `fs::current_path()` candidate at `game/src/Paths.cpp:165`) and `<scratch>/share/bv2` (`Paths.cpp:162`, the exe's parent) contain no `main/`. Today they don't, so the test does isolate the exe-dir path, and the CI fail/pass pair confirms the cause. A future test writing `main/` into the shared `test-scratch` would make this test pass for the wrong reason. | Use a dedicated subdirectory (`<scratch>/utf8-path`) as cwd and parent, and assert that no `main` exists there before running. |
| 6 | Low (missing check) | `tests/smoke_server_utf8_path.py:31-34` ignores `r.returncode`. A crash during `quit` after "Server Created on port" passes. | Check the exit code, as `smoke_server.py:22` does. Finding 2 fixes this for free. |
| 7 | Low (flaky under `ctest -j`) | `tests/CMakeLists.txt:15-21`: `smoke_server` and `smoke_server_utf8_path` both bind the default `sv_port`. A parallel ctest run makes `server->host()` fail (`game/src/SceneNet.cpp:84`), and the utf8 test then fails spuriously. | `set_tests_properties(smoke_server smoke_server_utf8_path PROPERTIES RESOURCE_LOCK bv2_server_port)`. |
| 8 | Low (proof gap vs deliverable) | The step deliverable reads "manifest linked into all three executables; a Windows ctest proving it", but only `bv2dedicated` is proven. `bv2master` (`masterserver/CMakeLists.txt:15-18`) uses the same `target_sources` `.manifest` mechanism, which CMake turns into `/MANIFESTINPUT` for MSVC+Ninja, so it very likely works, but no test covers it. | Accept, and say so in the record. Or add the step's fallback check (`mt.exe -inputresource:<exe>;#1`, grep `activeCodePage`) for `bv2master` and `bv2`. |
| 9 | Low (doc staleness) | `ARCHITECTURE.md:30`, the `tests/CMakeLists.txt` row, still lists "netPacket, config, fileio, dedicated-server smoke" and omits the new Windows-only non-ASCII path smoke test. | Add "Windows: non-ASCII install path smoke". |

Checks that hold:
- The `review.sh` regex `step(\d+[a-z]?)-` resolves `step2a` correctly. The glob `step2-*` can't match `step2a-*`, and the DONE gate's record path is right.
- The commits are atomic (test-first commit, then the fix), and no mechanical rewrite is mixed with functional changes.
- The ADR exists and is indexed. `activeCodePage` needing 1903 is correct.
- With `x64-windows-static`, copying only the exe is enough; there are no DLLs to copy.
- `GetModuleFileNameW` → `generic_u8string()` → narrow `fs::path` decoded through the ACP explains the failure without the manifest. The test exercises exactly that path.
- No GUI or protocol change.

Repeats of earlier findings: finding 2 partly repeats step 1 finding 3 (the banner check is printed before init and proves too little). `package-check` was fixed then; `smoke_server.py` itself still checks only the banner.

Not verifiable from the diff:
- That `bv2` and `bv2master` binaries really embed the merged manifest, and that the CMake default manifest doesn't conflict (no `mt.exe` output in the log I checked).
- Runtime behaviour on a real Windows 10 22H2 machine (owner acceptance check).
- That the UTF-8 ACP doesn't change console or stdin byte handling for the dedicated server's non-ASCII output or input (console code page is separate from the ACP).
- The ADR claim that SDL3, miniaudio and sqlite3 support Windows 7 or later.
- The `--full macos-arm64` result: taken as given, not rerun.
