# Step 6 review

Reviewer: fresh-context subagent, /thermo-nuclear-code-quality-review, two rounds (2026-10-17)
Diff: round 1 origin/main..15759ab; round 2 15759ab..63e71db
Skill evidence: both rounds `SKILL: Launching skill: thermo-nuclear-code-quality-review` (raw reports: [step6-report.md](step6-report.md))

## review.sh and tests

- `tools/review.sh --full macos-arm64` passes; ctest 25/25 including `admin_login` and `master`.
- Fix proofs: `admin_login` fails 3 checks with the length and budget rules off; `master` accepts `roxbabo` with the old `cNetManager.cpp`, lists the source address with the old address rule, and can't remove a game after a reconnect without the `BaboID` update (test_master.cpp:157).

## Findings (round 1)

| # | Finding | Outcome |
|---|---|---|
| 1 | `KILL_SERV` misses a game after its server reconnects | fixed (`BaboID` follows the heartbeat); test with fix proof |
| 2 | Client `Priority` bypasses ADR 0022 | fixed (`UpdateGame` zeroes it; the constructor always uses the chosen address) |
| 3 | Runbook "master address limitation" stale | fixed |
| 4 | Runbook remote-admin line stale; ADR 0015 cites removed code | fixed (runbook); ADR 0015 left as history |
| 5 | Config loader list stale | fixed |
| 6 | Dead remote-admin comments, blank lines | fixed |
| 7 | Too-long ban secret fails silently | fixed (start-up warning) |
| 8 | Short-password notice can flood the log | fixed (once) |
| 9 | Address choice split; `IsPrivateAddress` public | fixed by #2; placement accepted |
| 10 | No end-to-end test of a public-source heartbeat | accepted: the in-process test runs from loopback; `IsPrivateAddress` is unit-checked |

## Findings (round 2)

| # | Finding | Outcome |
|---|---|---|
| 1 | Priority logic now always takes one path | deferred: PNS-67 |
| 2 | Two identical update blocks in `CreateGame` | deferred: PNS-67 |
| 3 | One mis-indented line | fixed (whitespace commit) |
| 4 | Double blank lines | fixed (whitespace commit) |
| 5 | Ban-secret rule written twice | accepted: two lines |
| 6 | Once-notice never resets | accepted (corner case) |
| 7 | Old connection's kill not tested | accepted |

## Not done / unsure

- The compose example's new variables were not run in a container here (the `containers` CI job runs the compose example).
