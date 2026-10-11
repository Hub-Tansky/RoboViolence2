# Step 8 review

Reviewer: fresh-context subagent, /thermo-nuclear-code-quality-review, 2026-10-11
Diff: ad5052e...15eace7
Skill evidence: `SKILL: Launching skill: thermo-nuclear-code-quality-review` (raw report: [step8-report.md](step8-report.md))

## review.sh

`tools/review.sh --full macos-arm64` and `--full macos-arm64-asan` at 207837b: all checks PASS, 13/13 ctests. The reviewer found no behaviour change in the moved checks.

## Findings

| # | Finding | Outcome |
|---|---|---|
| 1 | Frame-rate rule, knockback exemption, both resets and kick text not pinned | fixed in 410a2ff (tests pass on ad5052e) |
| 2 | Speed rule split over handler, `Player` and validator | fixed in b73d5e6 (validator owns the window; window fields stay on `Player`, Allowed paths) |
| 3 | 8.2 delivered partly (one threshold, kick only) | owner 2026-10-11: 8.2 amended in 207837b |
| 4 | Duplicated `playerID != -1` condition | fixed in b73d5e6 |
| 5 | `which` out-parameter; slot passed twice | fixed in b73d5e6 (`badSpawnWeapon`, `onCoordFrame(Player &, …)`) |
| 6 | `sv_speedHackKick` missing from the example config | owner approved the path; fixed in 207837b |
| 7 | Unclear help text | fixed in b73d5e6 |
| 8 | 30316c5 also does 8.3; 15eace7 labelled 8.4 | rejected: pushed history not rewritten |

## Fix proof

With each line reverted, `validator` fails:

- frame check off: `test_validator.cpp:113`
- knockback exemption removed: `:124`
- dead reset removed: `:142`
- new-player reset removed: `:156`
- "primary"/"secondary" swapped: `:35`
- threshold hard-coded: `speedHackKickVar`

## Not done / unsure

- `char` signedness of `vel` on Linux ARM: unchanged from before.
- A new pushed `sv_*` variable and older clients: no wire struct changed, no version bump.
