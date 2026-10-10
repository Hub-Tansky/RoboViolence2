# Step 2a review

Reviewer: fresh-context subagent, /thermo-nuclear-code-quality-review, two rounds (2026-10-10)
Diff: round 1 5e6ec03..59bacb7; round 2 49ed746..ba9cac5 (without the merge from main)
Skill evidence: both rounds `SKILL: Launching skill: thermo-nuclear-code-quality-review` (raw reports: [step2a-report.md](step2a-report.md))

## review.sh

Before the review, `tools/review.sh --full macos-arm64` at 59bacb7: architecture, hygiene, content-case, original-assets, secrets, diff-rules, configure, build, ctest all PASS.

## Findings (round 1)

| # | Finding | Outcome |
|---|---|---|
| 1 | Docs say CI runs Windows Server 2022 (Windows 10 generation); the log shows Server 2025 (Windows 11 24H2 generation) | fixed in aa46dd0 (windows.md, Phase B README risk row, step file, ADR 0012); pinning the runner deferred → PNS-27 |
| 2 | `smoke_server_utf8_path.py` duplicates `smoke_server.py` and drops its exit-code and `bv2.cfg` checks; `smoke_server.py` checks only the banner | fixed in 9e4525d: one script with `--install-under`; both modes check exit code, "Server Created on port" and `bv2.cfg` |
| 3 | ADR 0012 cites line numbers this diff shifted | fixed in aa46dd0 (cites by name) |
| 4 | "A Windows 11-only API fails to compile" overstates what `NTDDI_VERSION` guards | fixed in aa46dd0 (CMake comment, ADR 0012) |
| 5 | Nothing asserts the cwd and `<exe>/../share/bv2` hold no `main/` | fixed in 9e4525d (dedicated `utf8-path` dir, assertion before the run) |
| 6 | The UTF-8 path test ignores the exit code | fixed in 9e4525d |
| 7 | Both server smoke tests bind the same port; flaky under `ctest -j` | fixed in 9e4525d (`RESOURCE_LOCK bv2_server_port`) |
| 8 | Only `bv2dedicated` is proven to carry the manifest; `bv2` and `bv2master` use the same mechanism untested | rejected: same `target_sources` `.manifest` mechanism as the proven `bv2dedicated`; the owner's Windows 10 run starts all three |
| 9 | `ARCHITECTURE.md` tests row omits the Windows-only smoke | fixed in 9e4525d |

## Findings (round 2)

| # | Finding | Outcome |
|---|---|---|
| 1 | Acceptance check not met: only `bv2dedicated` shown on Windows; `bv2master` not run, no game joined; round 1 finding 8's premise fails | confirmed with owner: owner runs `bv2master`; client play-test moves to step 2 on a PC with an OpenGL 2.1 driver (acceptance check amended) |
| 2 | Packages are Debug builds: windowed start (`GameVar.cpp:642–646`), debug C++ runtime, not redistributable | confirmed with owner: new step 2b (Release packages); PNS-28 corrected and accepted → step 2b |
| 3 | Static CRT has no ADR; `CMakeLists.txt` cites ADR 0012, which didn't mention it | fixed in this round's commit (ADR 0012 Consequences) |
| 4 | Fix proof lacks the failing output | fixed in this record (below) |
| 5 | Record's Diff line, round 2 table and `--full` at the final head missing | fixed in this record; `--full` below |
| 6 | `ARCHITECTURE.md` tests row omits `windows_no_crt_dlls` | fixed in this round's commit |
| 7 | `windows.md` says a person verified Windows 10 in step 2 | fixed in this round's commit (servers only, step 2a) |
| 8 | Commit subjects `step2a:` instead of `step2a.M:` | fixed going forward; pushed history not rewritten |
| 9 | Test dropped if one target is off | rejected: minimal-change rule; all three targets are built in every preset |

## Fix proof

- Static C++ runtime (11c3e62): `windows_no_crt_dlls` before the fix, run 38045503231:

  ```
  …/runtime/bv2.exe: MSVCP140D.dll
  …/runtime/bv2.exe: VCRUNTIME140_1D.dll
  …/runtime/bv2.exe: VCRUNTIME140D.dll
  …/runtime/bv2.exe: ucrtbased.dll
  …/runtime/bv2dedicated.exe: MSVCP140D.dll
  …/runtime/bv2master.exe: MSVCP140D.dll
  ```

  After the fix, run 38046255900: `6/6 Test #6: windows_no_crt_dlls … Passed`.
- The UTF-8 code page manifest: `smoke_server_utf8_path` failed on `windows-latest` without it (run 38000906829: banner, then quit, no "Server Created on port") and passed with it (run 38002113508).
- Finding 2/5/6: `tests/smoke_server.py … --install-under` passes locally on macOS; the Windows run follows in this PR's CI.

## Owner acceptance (2026-10-10, Windows 10 Pro 22H2, Intel G41 Express, CI package of 11c3e62)

- First package (49ed746): `bv2dedicated.exe` failed with "MSVCP140D.dll was not found". Fixed in 11c3e62 (static C++ runtime), with the `windows_no_crt_dlls` test failing before (run 38045503231) and passing after (run 38046255900).
- Fixed package: `bv2dedicated` runs (`Server Created on port 3333` with original data, CTF-Crazy). The client starts but renders garbled text, flickers and ignores clicks: the G41's Microsoft WDDM 1.1 driver has no OpenGL ICD, so Windows gives OpenGL 1.1. Owner decision: no code change; the Windows guide states the OpenGL 2.1 driver requirement.
- `bv2master` from the same package: "Database opened successfully", "Web Database opened successfully", "Master Server Up and Running".
- Accepted by the owner: the servers on Windows 10 22H2; the client is blocked by this PC's hardware, not by the package.

## Not done / unsure

- The client could not be play-tested on Windows: the owner's only Windows PC has an Intel G41 GPU without an OpenGL 2.1 driver (see Owner acceptance). Step 2's Windows play-test needs another Windows 10/11 PC.
- `--full` at a2ceaf6 (final code and docs): all PASS. This record's last commit only adds the master result and Status.
- `--full` after the review fixes: to rerun at the final head.
- The UTF-8 ACP and the console code page: non-ASCII server console output isn't tested.
