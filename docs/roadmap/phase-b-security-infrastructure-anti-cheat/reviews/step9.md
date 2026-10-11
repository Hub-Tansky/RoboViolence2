# Step 9 review

Reviewer: fresh-context subagent, /thermo-nuclear-code-quality-review, 2026-10-11
Diff: 5532f85...3f23b5d
Skill evidence: `SKILL: Launching skill: thermo-nuclear-code-quality-review` (raw report: [step9-report.md](step9-report.md))

## review.sh

`tools/review.sh --full macos-arm64` and `--full macos-arm64-asan` at 70fb450: all checks PASS, 13/13 ctests.

## Findings

| # | Finding | Outcome |
|---|---|---|
| 1 | A movement kick at `sv_antiCheat 2` crashed the server (null Player) | fixed in f9a20a2: the validator does every kick, the handler returns; test sends teleports until kicked and ticks on |
| 2 | `KICK` meant "kick" and "kicked" | fixed in f9a20a2 (`KICKED` only; the handler's copy of `Scene::kick` removed) |
| 3 | One lag spike scored once per packet | fixed in f9a20a2: once per rule and window; fire rate is a budget at the weapon's rate (a stall's burst is free) |
| 4 | Teleport and wall misfire for high ping | fixed in f9a20a2: allowance for the send interval; wall only inside a cell's inner half or on chords up to 1 unit; high-ping and corner tests |
| 5 | Path speed after a stall across the window start | fixed in f9a20a2 (window timed by client frames too); no test tells it apart, the stall trips the step 8 frame check (PNS-41) |
| 6 | Respawn not reset reliably; `Map*` comparison | fixed in f9a20a2: `onSpawned`; `timeAlive` kept for server-side spawns; map pointer removed |
| 7 | Shot with another weapon races a pickup | fixed in f9a20a2: 1 s grace for the replaced weapon; test |
| 8 | Rule predicates in the handlers | fixed in f9a20a2: `onShot`, `onDeadShot`, `onSpawnRequest`, `onPickup` |
| 9 | Parallel arrays and enum-order tables | fixed in f9a20a2: `rules` table with `static_assert`, one `Slot` per player |
| 10 | Step 8 bad windows counted at level 0 | fixed in f9a20a2; test switches 0 to 1 |
| 11 | Stuck-in-wall respawn scored | fixed in f9a20a2; test |
| 12 | Missing negative tests | fixed in f9a20a2: bazooka rockets, corner rounding, shots up to 2 s after death, a stall's burst, a pickup, high ping |
| 13 | Docs wording and line references | fixed in 70fb450 (functions named, 9.4 wording) |

## Fix proof

- #1: before the fix, `validator` died with a SegFault in the level-2 teleport test.
- With each change reverted, a test fails:
  - fire budget: `test_validator.cpp:275`
  - score per break: `:208-211`
  - send interval: `:453`
  - whole wall cell, long chords: `:486`
  - pickup grace: `:282`
  - stuck exemption: `:338`
  - level 0 counting: SegFault
  - 1 s after death: `:295`

## Not done / unsure

- Acceptance: the owner's play-test at `sv_antiCheat 1` (2026-10-12) logged no violation; the log is on the PR. It was short (five spawns), so PNS-41 and the thresholds still want a longer game.
- The thresholds come from the code and the harness, not a networked game.
