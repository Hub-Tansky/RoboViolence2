# Step 4 review

Reviewer: fresh-context subagent, /thermo-nuclear-code-quality-review, two rounds (2026-10-14)
Diff: round 1 db8f31a..44829d0; round 2 44829d0..7fae6bf
Skill evidence: both rounds `SKILL: Launching skill: thermo-nuclear-code-quality-review` (raw reports: [step4-report.md](step4-report.md))

## review.sh

`tools/review.sh --full macos-arm64` on the final head: all checks PASS (owner's uncommitted `SECURITY.md` and step 9 edits set aside for the run). ctest 20/20, including `smoke_server`, `server_harness` and `osv_vcpkg`. `tools/osv-vcpkg.py build/macos-arm64/vcpkg_installed` checks all five ports; miniaudio reports CVE-2026-32837 (PNS-54).

## Findings (round 1)

| # | Finding | Outcome |
|---|---|---|
| 1 | `content-seed/bv2.sql` duplicates `openClientDb`'s schema | deferred: PNS-55 |
| 2 | `config/README.md` still says to seed `bv2.db` by hand | fixed in 7fae6bf |
| 3 | `osv-vcpkg.py` has no test | fixed in 7fae6bf (`upstream()`), completed in round 2 (`main()` exit code) |
| 4 | Network or git failures exit 1 ("vulnerable") | fixed in 7fae6bf; test in round 2. Fix proof: with the e3c7448 script the test ends in `urllib.error.URLError: <urlopen error offline>` |
| 5 | Hard-coded helper-port list | fixed in 7fae6bf (`vcpkg-` prefix); covered by the round-2 test |
| 6 | Threads linked twice | fixed in 7fae6bf |
| 7 | `bv2server_lib` usage requirements listed twice | fixed in 7fae6bf (`bv2_game_target(<t> <visibility>)`) |
| 8 | A clean build likely never produced `runtime/bv2.db`: `bv2_seed_db` had no `ALL` and no dependents | accepted: PNS-24 removed a dead rule, not a generated file; step context and acceptance check reworded |
| 9 | ADRs 0013–0015 missing from the ARCHITECTURE Decisions row | fixed in 7fae6bf |
| 10 | W1 paragraph tense | fixed in 7fae6bf |
| 11 | Unclosed file handle | fixed in 7fae6bf |
| 12 | Long CI sentence in AGENTS.md | fixed in 7fae6bf |

## Findings (round 2)

| # | Finding | Outcome |
|---|---|---|
| 1 | Round-1 fixes in one commit | rejected: no mechanical change mixed in; noted here (same as step 1), no history rewrite |
| 2 | Exit-code fix and prefix skip untested | fixed: `test_osv_vcpkg.py` runs `main()` with OSV unreachable and a `vcpkg-cmake` port, expects exit 2. The annotated-tag pick needs git and the network: waived (exercised by every `osv-vcpkg` run) |
| 3 | `dependency-graph` listed as scheduled | fixed (AGENTS.md: "On `main`") |
| 4 | Unclear CMake comment | fixed |
| 5 | Missing `git` exits 1 | rejected: CI always has git (AGENTS corner-case rule), as the reviewer says |
| 6 | Python tests not named in AGENTS.md | rejected: the list is illustrative |

## Not done / unsure

- The new `osv-vcpkg` workflow has not run on GitHub yet (it runs on `main`, weekly or by hand); it will fail until PNS-54 is fixed.
