# Step 1 review

Reviewer: fresh-context subagent, /thermo-nuclear-code-quality-review, 2026-10-14 (5 rounds)
Diff: d13985822019269bdb7d37e9e50a07da5ef5e2cf..c8c33a2 (plus this record, the raw report and their `ARCHITECTURE.md` rows)

## review.sh

`tools/review.sh --full macos-arm64` on c8c33a2: all PASS except architecture (this record did not exist yet). Rerun with this record and the report staged: architecture, hygiene, content-case, original-assets, secrets, diff-rules, configure, build, ctest all PASS.

## Findings

Raw report: [step1-report.md](step1-report.md).

| # | Finding | Outcome |
|---|---|---|
| 1 | Server readiness grepped from a block-buffered log | fixed in 341c824: wait for the game port |
| 2 | `echo quit >fifo` can block forever | fixed in 341c824: `1<>` open |
| 3 | Lock: reused pid, pid-less window, double remove | fixed in 341c824 (shlock), 77bb07c, da92600, c8c33a2 (pid identity, empty file) |
| 4 | Stale `play.pids` trusted forever | fixed in 341c824 (`ours`, stale file dropped); "notify when skipped for play" rejected: play is started by hand, the owner knows |
| 5 | Environment failures use up build attempts | fixed in 341c824 (`INFRA_STAGES`) |
| 6 | Reinstall deletes the live package | fixed in 341c824, 77bb07c (SHA-keyed, swap, SHA-only reuse) |
| 7 | `SHA=""` as a magic signal | fixed in 341c824 |
| 8 | Duplicated cfg block and player name | fixed in 341c824 |
| 9 | State writing and prune untested | fixed in 65d2fd1 (`record_state`, `is_infra`, `prune` tests) |
| 10–20 | Double env load; precedence; play infra notify; SIGTERM counted as failure; AppleScript injection; ntfy topic in argv; env file overriding constants; masked artifacts error and pagination; preflight tool list; argument validation; ADR digest wording | fixed in 341c824 and 1852131 |
| 21 | shellcheck items | fixed in 341c824; remaining info-level items are intended |
| 22 | First commit not named `stepN.M:` | rejected: published history of the plan PR, not rewritten |
| 23 | Test hook `RV2_AGENT_SOURCE_ONLY` in the script | rejected: a separate lib file adds an install step |
| 24 | macOS shlock never recovers a dead pid's lock | fixed in 77bb07c |
| 25–30 | Infra marker never cleared; second signal in cleanup; `PLAY` duplicate; stage override; same-commit reinstall; layout check after swap | fixed in 77bb07c |
| 31 | Double remove of a stale lock | rejected: needs two manual runs at the same moment; narrowed by 24 |
| 32–35, 38 | Empty lock file; stderr on first install; loose agent match; marker after a recorded failure; README scope | fixed in da92600 |
| 36 | Console check after the local server starts | rejected: only wasteful under fast user switching |
| 37 | Reused install records the new run id | rejected: same commit, same package |
| 39 | Tighter lock pattern missed `bash agent.sh` | fixed in c8c33a2 |
| 40 | Infra predicate written twice | fixed in c8c33a2 |

Round 5 verdict: approved, no new findings.

## Fix proof

None required: every fix is to code added in this step, not to a defect in merged code. New behaviour is covered by `tests/test_playtest_agent.sh` (ctest `playtest_agent`, 27 checks) and by live runs on the owner's Mac against build `5cfcfee`: pass, re-run reusing the install, busy port (infra, no attempt), interrupt (nothing recorded), concurrent run skipped, stale and empty lock recovered, `--play`/`--stop`, stale `play.pids`, staging `not-configured`, bad arguments.

## Not done / unsure

- Acceptance check "after the owner installs the LaunchAgent, the next `main` build is reported within 20 minutes" needs the owner: installing a LaunchAgent is persistent configuration on their Mac. Whether `gh` can read the keychain under `launchd` is only known after that. The step stays IN PROGRESS until then.
- ntfy notifications not exercised (no topic configured).
