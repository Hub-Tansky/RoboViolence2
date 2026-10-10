# Step 3 review

Reviewer: fresh-context subagent, /thermo-nuclear-code-quality-review, 2026-10-10
Diff: origin/main...77da862 (14 commits, 5f8b4e3..77da862)
Skill evidence: `SKILL: Launching skill: thermo-nuclear-code-quality-review` (raw report: [step3-report.md](step3-report.md))

## review.sh

`tools/review.sh --full macos-arm64` and `--full macos-arm64-asan` at 77da862: architecture, hygiene, content-case, original-assets, secrets, diff-rules, configure, build, ctest all PASS. After the fixes (64b4c04) both pass again, 7 and 12 tests.

## Findings

| # | Finding | Outcome |
|---|---|---|
| 1 | The fake network never reports a disconnect; fuzzer rejoins fill the server with ghost players | fixed in 64b4c04 (`bb_serverDisconnectClient` queues the disconnect; `join` disconnects the old client) |
| 2 | Fuzz masks inconsistent: R23, R24 and R25 crash at once | fixed in 64b4c04 (R23/R24 and Q-S2 `weaponID` masked; `join` spawns). Shoot and projectile still reach R25 through a just-killed player; recorded in the step file and R25 |
| 3 | `last.bin` can't reproduce state-dependent crashes | fixed in 64b4c04 (`last.txt` with seed and input count; optional max-inputs argument) |
| 4 | Type table and `valid()` switch are parallel lists; an empty sample replays fine | fixed in 64b4c04 for the defect (`gen_corpus` fails on an empty sample); rejected for the table merge: minimal-change rule |
| 5 | Crash tests pass on any ASan report, not this input's | fixed in 64b4c04 (marker line before the crash input; regex requires the report after it) |
| 6 | Link-order comments give the wrong reason; fake lacks six `bb_*` functions | fixed in 64b4c04 (object-file wording; all of `baboNet.h` covered) |
| 7 | `bv2server_lib` doesn't export sqlite | fixed in 64b4c04 |
| 8 | Server sources compiled twice | deferred → PNS-33 |
| 9 | Leftover R28 workaround leaves list connections open | fixed in 64b4c04 (closes again; Windows runs as expected-fail) |
| 10 | `start` once per process undocumented; internal queue functions in the public header | fixed in 64b4c04 (documented; marked internal) |
| 11 | Commit subjects `step3:` (repeat of step2a #8, step2 #10) | rejected: pushed history not rewritten |

## Fix proof

- R27 (`cPacket` `delete []`): CI run 38070646732, Linux ASan, before the fix: `ERROR: AddressSanitizer: alloc-dealloc-mismatch (operator new [] vs operator delete)` in `cPacket::~cPacket() cPacket.cpp:213`. Passes since 7c6fd0b.
- `CMaster.cpp` `memcpy`: build error only; CI run 38070646732 failed with `-Werror=stringop-truncation` at `CMaster.cpp:1210`, later runs build.
- Crash tests: the Q-S2 test fails on a benign input (`Required regular expression not found`), and passes on its crash input.
- Master test: with the simulated 61 s removed, `FAIL tests/test_master.cpp:98: list().empty()`.
- Fake disconnects: a 3 s fuzz of every type reports no "server is full".

## Not done / unsure

- R25 and R26 have no corpus input (state-dependent).
- R28 (Windows babonet crash on disconnect): cause not found; Windows runs the master test as `master_expected_fail_r28`.
- The `fuzz` job stays red on player shoot and projectile until R25 is fixed.
