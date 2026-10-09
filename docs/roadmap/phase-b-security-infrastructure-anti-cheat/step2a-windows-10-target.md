# Step 2a: Windows 10 22H2 as the minimum Windows target

**Status:** IN PROGRESS

**Depends on:** [Step 1](step1-playtest-builds-and-build-guides.md). **Blocks:** the Windows run of [step 2](step2-cross-os-playtest.md). **Next:** [step3-test-harness.md](step3-test-harness.md). **Index:** [README.md](README.md)

## Scope

| Field | Value |
|---|---|
| Goal | Windows 10 22H2 (build 19045) is the documented minimum Windows target; `bv2`, `bv2dedicated` and `bv2master` all carry the manifest, so every Windows executable runs with the UTF-8 code page |
| In scope | Tasks 2a.1–2a.5 below |
| Out of scope | Windows 10 builds older than 22H2, including LTSC 2019 (1809): no UTF-8 code page, would need wide-character paths; Windows arm64; code signing (PNS-16); the step 2 play-test itself |
| Allowed paths | `tools/review.sh` (task 2a.1 only; owner-approved 2026-10-10), `docs/decisions/**`, `packaging/windows/**`, `CMakeLists.txt`, `game/CMakeLists.txt`, `masterserver/CMakeLists.txt`, `tests/**`, `README.md`, `docs/build/**`, `docs/roadmap/**`, `ARCHITECTURE.md` |
| Inputs | `AGENTS.md`, `ARCHITECTURE.md`, earlier review records `reviews/step*.md`, `packaging/windows/bv2.manifest`, `game/src/Paths.cpp` |
| Deliverables | ADR 0012; manifest linked into all three executables; a Windows ctest proving it; docs naming Windows 10 22H2 as the floor |
| Definition of done | See "Acceptance checks". All must pass. `ARCHITECTURE.md` is updated |

## Context

- The owner has a Windows 10 Pro 22H2 machine and no Windows 11 machine, so the step 2 Windows run can only be done on Windows 10.
- The code already targets the Windows 10 API level: `_WIN32_WINNT=0x0A00` (`CMakeLists.txt:70`). The manifest's `supportedOS` GUID covers Windows 10 and 11 (`packaging/windows/bv2.manifest:6`). The code calls only `WSAStartup`, `GetModuleFileNameW`, `MessageBox` and `Get/SetProcessAffinityMask`. SDL3, miniaudio and sqlite3 support Windows 7 or later.
- `activeCodePage UTF-8` (`bv2.manifest:13`) needs Windows 10 1903 or later. 22H2 has it.
- Only `bv2` links the manifest (`game/CMakeLists.txt:172`). `bv2dedicated` compiles `game/src/Paths.cpp`, which turns the executable's directory into UTF-8 (`Paths.cpp:62`) and opens files through narrow `fopen`/`fstream`. Without the UTF-8 code page, a non-ASCII install path (for example `C:\Gry\Łódź\`) breaks loading `main/`, on Windows 10 and 11 alike. `bv2master` opens only relative ASCII paths (`masterserver/src/cMasterServer.cpp:59`), but gets the manifest for consistency.
- CI's `windows-latest` is Windows Server 2025 (build 26100, the Windows 11 24H2 generation; corrected in review, run 38002113508). Nothing in CI runs a Windows 10-generation kernel, so the owner's machine is the only Windows 10 runtime check.
- `tools/review.sh:52` and `:94` match `step(\d+)`. A `step2a` branch is not recognised as a step: no scope check, and no DONE gate for this file.

## Tasks

### 2a.1 Recognise lettered steps in `tools/review.sh`

- Change both step patterns (`tools/review.sh:52`, `:94`) to `step(\d+[a-z]?)`, so branch `refactor/phase-b-step2a-windows-10-target` maps to this file and its DONE gate applies. This is a guard file: the owner approved this change on 2026-10-10.
- Commit it first, as `step2a.1: …`, then run `tools/review.sh` on this branch to confirm it reports `step: docs/roadmap/…/step2a-windows-10-target.md`.

### 2a.2 ADR 0012: Windows 10 22H2 is the minimum Windows target

- Context: the owner's only Windows machine; Windows 10 is the same API level as Windows 11; UTF-8 code page needs 1903 or later; 22H2 is the last Windows 10 release.
- Decision: support Windows 10 22H2 and Windows 11 (x64). Older Windows 10 builds are unsupported.
- Consequences: no code changes needed; Windows 11-only APIs need a runtime check before use; dropping Windows 10 later needs a new ADR. Rejected: Windows 11 only (the owner can't test it); 1809/LTSC 2019 (needs wide-character paths throughout).
- Add it to `docs/decisions/README.md` and the `ARCHITECTURE.md` inventory.

### 2a.3 Pin the SDK API level to Windows 10 20H1

- Add `NTDDI_VERSION=0x0A000008` (`NTDDI_WIN10_VB`, the API surface of 2004–22H2) next to `_WIN32_WINNT`/`WINVER` in `bv2_target_defaults` (`CMakeLists.txt:70`), and update the comment at `CMakeLists.txt:44`. Calling a Windows 11-only API in our code then fails to compile instead of failing at load time on Windows 10.
- Proof: CI `build` passes on `win-x64-msvc`.

### 2a.4 Manifest for `bv2dedicated` and `bv2master`

- Link `packaging/windows/bv2.manifest` into `bv2dedicated` (`game/CMakeLists.txt`, `if(WIN32)` block after line 148) and `bv2master` (`masterserver/CMakeLists.txt`) with `target_sources`, as `bv2` does. The DPI entries have no effect in console programs; one manifest for all three keeps them in sync. Update the comment in `bv2.manifest` to say which executables use it.
- Test (Windows only, in `tests/CMakeLists.txt`): extend `tests/smoke_server.py`, or add a ctest beside it, that copies `bv2dedicated` and `main/` into `<scratch>/Łódź/` and runs the smoke check from there. It fails without the manifest and passes with it; confirm both once locally or in CI and note the result in the commit body.
- If the copy-and-run test proves too slow or flaky, fall back to extracting each executable's manifest (`mt.exe -inputresource:<exe>;#1 -out:<file>`) and checking for `activeCodePage` and the `supportedOS` GUID. Record the choice in the commit body.

### 2a.5 Documentation

Replace "Windows 11" as the floor with "Windows 10 22H2 or later (x64)" in the living docs:

| File | Change |
|---|---|
| `README.md:36` | Targets line |
| `ARCHITECTURE.md:11` | Platforms row; also the `packaging/windows/bv2.manifest` row (line 72): used by all three executables |
| `docs/build/windows.md:1`, `:5` | Title "Windows 10 22H2+ / 11 (x64)"; the verification note |
| `docs/build/README.md` | Only if it names a Windows version |
| `docs/roadmap/phase-a-modern-portable-build/phase-a-manual-test.md:5`, `:105` | "Windows 10 22H2 / 11"; the Results row label records the tested version |
| `docs/roadmap/phase-b-security-infrastructure-anti-cheat/README.md` | Risks: Windows 11 is not tested by a person, only Windows 10 22H2 |
| `docs/roadmap/phase-b-security-infrastructure-anti-cheat/step2-cross-os-playtest.md` | Windows run on Windows 10 22H2 |

- Leave the other Phase A scope files unchanged: they record a closed phase. ADR 0012 supersedes their "Windows 11" floor.
- `docs/roadmap/possible-new-scope.md` line 136 (PNS packaging extract) quotes a source; leave the quote.

## Critical files

- `CMakeLists.txt`, `game/CMakeLists.txt`, `masterserver/CMakeLists.txt`
- `packaging/windows/bv2.manifest`
- `tests/CMakeLists.txt`, `tests/smoke_server.py`
- `tools/review.sh`

## Acceptance checks

- `tools/review.sh` on the step branch names this step file and reports no scope problems.
- CI `build` and `smoke` pass on all three OSes; the new Windows ctest passes on `windows-latest`:

```bash
ctest --test-dir build/win-x64-msvc --output-on-failure
```

- The owner starts `bv2`, `bv2dedicated` and `bv2master` from the CI Windows package on Windows 10 Pro 22H2 and joins a local game (step 2 then runs the full manual test).
- `grep -rn "Windows 11" README.md ARCHITECTURE.md docs/build docs/roadmap/phase-b-security-infrastructure-anti-cheat` shows no remaining "Windows 11 only" floor.
- `tools/review.sh --full <preset>` passes, and the fresh-context review record `reviews/step2a.md` and the raw reviewer report `reviews/step2a-report.md` (with the skill's load line) exist ([REVIEW.md](../../../REVIEW.md) section 5).
- `ARCHITECTURE.md` lists every file added, moved or removed in this step.
- The secret scan (`gitleaks git --staged` / CI) is clean.
