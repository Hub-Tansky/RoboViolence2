# 0012: Support Windows 10 22H2 and later as the minimum Windows target

- Status: Accepted
- Date: 2026-10-10
- Affects: [Phase B step 2a](../roadmap/phase-b-security-infrastructure-anti-cheat/step2a-windows-10-target.md); supersedes the "Windows 11" floor in the Phase A plan

## Context
- The owner's only Windows machine runs Windows 10 Pro 22H2; there is no Windows 11 machine to play-test on.
- The code already targets the Windows 10 API level (`_WIN32_WINNT=0x0A00`, `CMakeLists.txt:70`), and the manifest's `supportedOS` covers Windows 10 and 11 (`packaging/windows/bv2.manifest:6`). SDL3, miniaudio and sqlite3 run on Windows 7 or later.
- The manifest's `activeCodePage UTF-8` (`packaging/windows/bv2.manifest:13`) needs Windows 10 1903 or later; `game/src/Paths.cpp` relies on it for non-ASCII paths.
- 22H2 (build 19045) is the last Windows 10 release.

## Decision
Support Windows 10 22H2 and Windows 11 (x64). Older Windows 10 builds are unsupported.

## Consequences
- No code changes needed. The SDK API level is pinned to Windows 10 20H1 (`NTDDI_VERSION`), so a Windows 11-only API fails to compile instead of failing to load on Windows 10; using one later needs a runtime check.
- Windows 11 is only covered by CI (`windows-latest`) unless a tester has it; dropping Windows 10 later needs a new ADR.
- Rejected: Windows 11 only (the owner can't test it); Windows 10 1809 / LTSC 2019 (no UTF-8 code page, would need wide-character paths throughout).
