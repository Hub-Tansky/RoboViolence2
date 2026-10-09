# Step 2a review

Reviewer: fresh-context subagent, /thermo-nuclear-code-quality-review, 2026-10-10
Diff: 5e6ec03..59bacb7
Skill evidence: `SKILL: Launching skill: thermo-nuclear-code-quality-review` (raw report: [step2a-report.md](step2a-report.md))

## review.sh

Before the review, `tools/review.sh --full macos-arm64` at 59bacb7: architecture, hygiene, content-case, original-assets, secrets, diff-rules, configure, build, ctest all PASS.

## Findings

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

## Fix proof

- The UTF-8 code page manifest: `smoke_server_utf8_path` failed on `windows-latest` without it (run 38000906829: banner, then quit, no "Server Created on port") and passed with it (run 38002113508).
- Finding 2/5/6: `tests/smoke_server.py … --install-under` passes locally on macOS; the Windows run follows in this PR's CI.

## Not done / unsure

- Owner acceptance: start `bv2`, `bv2dedicated` and `bv2master` from the CI Windows package on Windows 10 Pro 22H2 and join a local game. Not done yet; the step stays IN PROGRESS.
- `--full` after the review fixes: to rerun at the final head.
- The UTF-8 ACP and the console code page: non-ASCII server console output isn't tested.
