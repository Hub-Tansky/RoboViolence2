# Step 7 review

Reviewer: fresh-context subagent, /thermo-nuclear-code-quality-review, one round (2026-10-17)
Diff: origin/main..9ae6cbb
Skill evidence: `SKILL: Launching skill: thermo-nuclear-code-quality-review` (raw report: [step7-report.md](step7-report.md))

## Evidence (Windows CI, `windows-2025`)

- Before the fix (run 39015280742): `test_master` failed 50/50 in both builds. ASan: `heap-buffer-overflow ... READ of size 8` in `cClient::Disconnect` (`cClient.cpp:547`) ← `~cClient` ← `cServer::RemoveClient` ← `cServer::ReceivePacketsFromClients`; `cdb` on Debug: access violation at the same line.
- After the fix: 0/50 failures in both builds, twice (runs 39026339957 and 39027172661). CI `build (win-x64-msvc)` runs and passes `master` in Debug and Release (run 39026339987).
- macOS: ctest 26/26.

## Findings

| # | Finding | Outcome |
|---|---|---|
| 1 | Bad partial key leaves a closed client listed; predicted to wedge the server on Windows | tested, not reproduced: a new `master` phase sends a bad partial key and checks the master still serves; it passes on all three OSes without a change (CI run 39027172674). No fix without fix proof; deferred: PNS-68 |
| 2 | Diagnostic workflow and its row still present | fixed (deleted) |
| 3 | Post-fix evidence missing | fixed (above) |
| 4 | R28 description stale | fixed |
| 5 | Client-side crash site unexplained | fixed: both reported sites were the in-process master removing a client (the test runs master and clients in one process) |
| 6 | `cClient` constructors initialise by hand | accepted (MVP): the one missing `FD_ZERO` added; the default constructor is unused |
| 7 | Comment says Windows reuses handles | fixed |
| 8 | Bookkeeping | fixed (status, PNS-38) |

## Not done / unsure

- No person ran a Windows game after the fix; the evidence is CI.
- Commit f222bd8's message says the bad-partial-key phase "fails on Windows before the fix"; it did not (finding 1).
