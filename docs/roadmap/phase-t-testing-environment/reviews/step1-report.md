SKILL: Launching skill: thermo-nuclear-code-quality-review

# Phase T step 1 review (Mac pull agent)

Diff: d13985822019269bdb7d37e9e50a07da5ef5e2cf..3867dd653ccec985780fa23543c240c8156f6055 (6 commits, 12 files, +882). Earlier records: none.

How I checked: I read the whole diff and the step file, plus `packaging/scripts/run-client.sh`, `packaging/make-package.py`, `.github/workflows/build.yml`, `game/src/AdminApi.cpp`, `game/src/main.cpp` (stdin loop), `game/src/Console.cpp` (printf output) and babonet socket setup. `bash tests/test_playtest_agent.sh <scratch>` passes locally ("all passed"). I ran shellcheck: info/warning items only, see 21. I checked two behaviours on this Mac (bash 3.2):
- `echo quit 1<>fifo` does not block when the FIFO has no reader.
- The EXIT trap runs when the agent gets SIGTERM.

I did not run the agent end to end. Nothing was edited.

## REVIEW.md section 2

- **Ground rules:** hold. No game, asset, GUI or rename changes.
- **New behaviour has a test:** partial. The pure helpers are tested: `pick_run`, `artifact_info`, `digest_ok`, `due`, `load_env`. These are not: the state-writing / attempt-increment path in `finish`, the lock, teardown, `prune`, and the play-skip rule. See 9.
- **ARCHITECTURE.md / READMEs / ADRs:** ARCHITECTURE lists every added file. ADR 0016 is added and indexed. `tools/playtest/README.md` is added, and there is no other affected module README. Small ADR wording issue in 20.
- **Mechanical vs functional commits:** not mixed. The first commit `e9b6966 docs: …` does not follow the `stepN.M:` naming (nit 22).

## Findings

1. **blocker (unless already proven on the Mac with this exact code)** — `tools/playtest/agent.sh:176`
   - **Problem:** Server readiness is detected by grepping a live `server.log` for "Server Created on port".
     - The dedicated server prints through `printf` (`game/src/Console.cpp:109`) and never calls `setvbuf`/`fflush` (grep: none in `game/src` or `engine/zeven/src`).
     - With stdout redirected to a file, stdio is fully buffered, so the line may only reach the file after about 4 KB of output, or at exit.
     - Every existing check (`build.yml:76-77`, `tests/smoke_server.py`) reads the log only after the server has quit, so none of them proves the live-poll approach.
   - **Failure scenario:** a healthy server; stage `server-start` times out after 30 s and every build fails 3 times.
   - **Proposed change:** wait for the game port, the same way the master is checked. The game server opens a TCP listener on the game port (`engine/babonet/src/cServer.cpp:113`): `wait_for 30 port_busy $GAME_PORT || fail …`. This removes the log-format dependency. Keep the log only for the report tail. If the owner's Mac run already passed with this code, record that evidence and downgrade this to should-fix (it is still fragile).

2. **should-fix** — `tools/playtest/agent.sh:206`
   - **Problem:** `alive "$SERVER_PID" && echo quit >"$FIFO"` is check-then-open. If the server exits between `kill -0` and `open(O_WRONLY)`, the open blocks forever because no reader remains. The same happens if `kill -0` succeeds on an unreaped zombie.
   - **Failure scenario:**
     - The agent hangs while holding `$ROOT/lock`, and its pid stays alive.
     - Every later launchd run then takes the silent `exit 0` at line 96, so testing stops with no notification.
   - **Proposed change:** `echo quit 1<>"$FIFO"`. Opening read-write never blocks (verified on macOS bash 3.2), and the `alive` guard and the comment can go.

3. **should-fix** — `tools/playtest/agent.sh:93-102`
   - **Problem:** Lock robustness.
     - (a) A stale lock whose pid was reused after a reboot or crash (launchd SIGKILL or power loss skips the trap) makes `alive` true. The agent then exits 0 silently on every run, forever, with nothing in the log.
     - (b) Between `mkdir "$ROOT/lock"` and `echo $$ >…/pid` (lines 95, 100), a concurrent run sees no pid, treats the lock as stale, and `rm -rf`s it. Two agents then run on the same fixed ports.
     - (c) Two runs that both see the same stale lock can each `rm -rf`, and the second can delete the first one's fresh lock.
   - **Proposed change:**
     - Check identity, not just liveness: `ps -p "$pid" -o command= | grep -q agent.sh`.
     - Log `say "locked by $pid; skipped"` instead of a silent exit.
     - Write the pid atomically (`echo $$ >"$ROOT/lock.$$" && mv`), or treat a pid-less lock younger than a minute as held.
     - Simplest alternative: `shlock -p $$ -f "$ROOT/lock"`, which ships with macOS and does the pid check and atomic create.

4. **should-fix** — `tools/playtest/agent.sh:247, 267, 280`
   - **Problem:** `play.pids` is trusted forever.
     - After a reboot or crash it holds stale pids. If any is reused, `play_running` is true and every scheduled run says "manual play running; skipped", so testing stops silently. It is logged but never notified.
     - `--stop` sends TERM to whatever unrelated processes now have those pids.
   - **Proposed change:**
     - Verify each pid is ours: `ps -p $p -o comm=` matching `bv2*`/`sleep`.
     - Delete `play.pids` when nothing in it is ours.
     - Notify once when a scheduled run is skipped because of play.

5. **should-fix** — `tools/playtest/agent.sh:219-221` (state/attempt logic)
   - **Problem:** Environmental failures after `SHA` is known count as build attempts. These are `ports-busy`, `download` (network), `artifact` (API blip) and `no-gui-session` (fast user switching).
   - **Failure scenario:** The owner runs their own server on 3333 or 10207 for 30 minutes. The newest build burns all 3 attempts and is never tested automatically: `due` is false and only `--sha` recovers it. The test at `tests/test_playtest_agent.sh:45-49` codifies "download" failures as attempts.
   - **Proposed change:**
     - Count an attempt only for stages that test the build (`verify` onward, excluding `ports-busy` and `gui-session`).
     - Route the others through the existing `infra-failure` once-per-stage path.
     - Equivalent: move `ports-busy` and `gui-session` before `begin_report`, so they fail with `SHA` unset.

6. **should-fix** — `tools/playtest/agent.sh:142-143`
   - **Problem:** The install is not idempotent per SHA, and it can delete the `current` package.
     - `vdir=versions/<date>-<sha7>` followed by `rm -rf "$vdir"` deletes the live package when the same SHA is reinstalled on the same day. That happens with `--sha <current>` or `--test staging-connect --sha <current>`, and the latter does not check `play_running`.
     - A running `--play` session loses its binaries.
     - If `ditto` then fails, `current` dangles.
   - **Proposed change (code judo):**
     - Key versions by SHA alone (`versions/<sha7>`; `prune` already orders by mtime, so the date prefix buys nothing).
     - Skip download and unzip when `versions/<sha7>/BUILD` already names that SHA and run.
     - Otherwise unzip into the temp dir and `mv` it into place, so a failed unzip never leaves a half-installed or deleted version.

7. **should-fix (simplification)** — `tools/playtest/agent.sh:282, 317, 217`
   - **Problem:** `SHA=""` is used as a magic "don't record" signal, and both assignments are dead.
     - `finish` records only when `REPORT` is set, and at line 282 (before `begin_report`) and in play mode (which never calls `begin_report`) `REPORT` is empty.
     - The `-n "$SHA"` half of the condition at 217 is therefore redundant.
   - **Proposed change:** Make `REPORT` the single "a build is being tested" signal. Delete both `SHA=""` lines and the comment at 317, and test `[ -n "$REPORT" ]` at 217.

8. **should-fix (duplication)** — `tools/playtest/agent.sh:286-289` vs `303-306`, plus `291`, `308`, `310`
   - **Problem:** The cfg-writing block is copy-pasted between smoke and staging, and `"rv2-agent-${SHA:0:7}"` is spelled five times.
   - **Proposed change:**
     - Set `NAME="rv2-agent-${SHA:0:7}"` once, in `begin_report`.
     - Give `start_client <pref dir> <host> [name]` the job of writing `playtest.cfg` when a name is passed. Play passes none and keeps the default config.
     - This removes about 8 lines and one concept.

9. **should-fix (tests, REVIEW §2 "new behaviour has a test")** — `tests/test_playtest_agent.sh`
   - **Problem:** The test writes `state.tsv` lines by hand. It never exercises the code that produces them: `finish` computes `attempts_of + 1`, formats 8 tab-separated columns and picks `-` vs `$STAGE`. A column-order bug in `finish` (line 220-221) would pass every test while `due` silently breaks. `prune` (never deletes `current`) is also untested.
   - **Proposed change:**
     - Extract `record_state <sha> <run> <test> <target> <result> <stage>` (pure apart from the append) from `finish`.
     - Test that two failing records then `due` give attempts 1 and 2.
     - Add a `prune` test with a `current` symlink pointing at the oldest entry.

10. **should-fix** — `tools/playtest/agent.sh:274-275`
   - **Problem:** `load_env` runs twice, once in a subshell to capture its message and once for real, so the env file is sourced twice. At that point `REPORT` is empty and `say` goes to stdout, which is already `agent.log`, so the capture buys nothing.
   - **Proposed change:** `load_env || fail "env file"`, letting `load_env`'s own `echo` reach the log.

11. **should-fix** — `tools/playtest/agent.sh:216`
   - **Problem:** `[ -z "$PLAY" ] || [ "$RESULT" != pass ] && stop_all` relies on `||`/`&&` having equal precedence, left to right. It is correct but has to be re-derived by every reader.
   - **Proposed change:** `if [ -z "$PLAY" ] || [ "$RESULT" != pass ]; then stop_all; fi`. Better still, set a `KEEP_RUNNING=1` only on a successful play start.

12. **should-fix** — `tools/playtest/agent.sh:214-237` (play failures)
   - **Problem:** A failed manual `--play` (busy port, no current build) takes the `elif` branch. It notifies "RV2 agent FAILED at … No build tested" and overwrites `infra-failure`. That suppresses the next real unattended infra notification for the same stage, and the message is wrong for a manual command whose user is already at the terminal.
   - **Proposed change:** Skip the infra-failure branch when `PLAY` is set; the stdout `FAIL` line is enough.

13. **should-fix** — `tools/playtest/agent.sh:272` with launchd shutdown
   - **Problem:** The EXIT trap also runs on SIGTERM (verified). A run interrupted by `launchctl bootout`, logout or shutdown records a failed attempt at whatever stage it was in and sends a FAILED notification.
   - **Proposed change:** `trap 'RESULT=interrupted; exit 143' TERM INT`. `finish` then stops processes and releases the lock, but does not record an attempt or notify for `interrupted`.

14. **nit** — `tools/playtest/agent.sh:70`
   - **Problem:** `osascript -e "display notification \"$2\" …"` interpolates text into AppleScript source. A `"` or `\` in a message breaks it. The messages can carry `gh` error text, for example via `preflight`'s first line at 265.
   - **Proposed change:** `osascript - "$1" "$2" <<'EOF'` with `on run a … display notification (item 2 of a) with title (item 1 of a)`.

15. **nit (secret exposure)** — `tools/playtest/agent.sh:71`
   - **Problem:** `NTFY_TOPIC` goes into curl's argv (`https://ntfy.sh/$NTFY_TOPIC`), so it is visible in `ps` to other local users. On ntfy.sh, knowing the topic is enough to read and post.
   - **Proposed change:** Pass the URL through curl's config on stdin: `printf 'url = "https://ntfy.sh/%s"\n' "$NTFY_TOPIC" | curl -K - …`.
   - **Checked otherwise:**
     - The admin passwords reach curl only via stdin (`-H @-`).
     - `BV2_ADMIN_PASS` reaches only `bv2dedicated`'s environment. It is unexported, so `bv2master` and the client do not inherit it.
     - The GitHub token is in `gh`'s environment only.

16. **nit** — `tools/playtest/agent.sh:53`
   - **Problem:** The env file is sourced after `REPO`, `ROOT`, `PKG_NAME`, `MAX_ATTEMPTS` and the others are set, so a stray `REPO=` line in it silently overrides the main-only repo filter. The file is owner-controlled and mode 600, but the main-only guarantee should not depend on the env file's contents.
   - **Proposed change:** Make the core constants `readonly`, so sourcing an override fails loudly.

17. **nit** — `tools/playtest/agent.sh:133`
   - **Problem:**
     - A `gh_api` failure is masked by the pipe into `artifact_info` and reported as "run has no artifact".
     - The artifacts endpoint is paginated (default 30), unlike the runs query.
   - **Proposed change:** Capture first (`json=$(gh_api …) || fail "artifacts query"`), then pipe, and add `?per_page=100`.

18. **nit** — `tools/playtest/agent.sh:113`
   - **Problem:** `xattr`, `mkfifo` and `stat` are used but are not in the preflight tool list.
   - **Proposed change:** Add them, or drop tools that every macOS has from the list entirely.

19. **nit** — `tools/playtest/agent.sh:251-260, 294, 325`
   - **Problem:** Argument validation is uneven.
     - `--test <unknown>` and `--play <unknown>` are rejected only after the lock and the full GitHub preflight.
     - `--sha` with no value dies on `set -u`, with a raw "unbound variable" error.
     - `--sha` together with `--play` is silently ignored.
   - **Proposed change:** Validate `TEST`/`TARGET` in the argument loop.

20. **nit** — `docs/decisions/0016-playtest-pull-agent.md:9` and `tools/playtest/agent.sh:140`
   - **Problem:** "verified-digest" overstates what the check gives. The digest and the zip come from the same GitHub API over the same TLS channel, so the check catches truncated or corrupted transfers, not a compromised source. The security boundary is the `push`/`main`/`head_repository` filter.
   - **Proposed change:** One clause in the ADR saying the digest is an integrity check and the trust anchor is the main-only run filter.

21. **nit** — shellcheck output on `tools/playtest/agent.sh`
   - **Findings:** SC2217 at line 169 (an intentional redirect on `sleep`); SC2015 at 143 and 164 (`A && B || C`); SC2086 on the unquoted pid lists (intentional word splitting); SC2012 at 106.
   - **Proposed change:**
     - At 169, use `sleep 2147483647 >"$FIFO" &` without `</dev/null`, or add a disable comment with the reason.
     - At 143 and 164, rewrite the `&&`/`||` chains as `if` blocks; they are the same pattern as finding 11.

22. **nit** — commit `e9b6966`
   - **Problem:** The commit is named `docs: …` instead of `step1.0:`/`step1.1:` per AGENTS.md ("one commit per task item (`stepN.M: <summary>`)").
   - **Proposed change:** None required; note it in the review record.

23. **nit** — `tools/playtest/agent.sh:57`
   - **Problem:** `RV2_AGENT_SOURCE_ONLY` is a test hook in production code.
   - **Alternative:** a `tools/playtest/lib.sh` that both `agent.sh` and the test source. That costs a second file to copy at install time (README line 18). Acceptable as is; listed for completeness.

## Main-only / no-PR-artifact guarantee

Holds in the code.
- The runs query filters `branch=main&event=push&status=success` server-side, and `pick_run` re-checks `event == "push"`, `head_branch == "main"`, `head_repository.full_name == REPO` and `conclusion == "success"`. The fork-PR test covers it.
- The artifact is fetched only by the selected `RUN_ID`.
- `current`, used by `--play` and staging-connect without `--sha`, is set only after a local-smoke pass of such a build.

The only weakening is nit 16 (an env-file override of `REPO`).

## Structure (skill)

- The file stays well under 1k lines (330), and the mode/stage split is readable.
- The main code-judo moves are 1 (port readiness instead of log grep), 2 (non-blocking FIFO write, which deletes the guard), 6 (SHA-keyed idempotent install), 7 (one "recording" signal) and 8 (cfg written in `start_client`). Together they remove branches and duplicated blocks rather than rearranging them.

**Not approved as is:** finding 1 must be verified or fixed, and 2–6 and 9 should be addressed or deferred with a PNS entry.

## Round 2

SKILL: Launching skill: thermo-nuclear-code-quality-review

# Phase T step 1 review, round 2 (fix commits 3867dd6..1852131)

Head: 18521310ecf8f17b9ef8b6b58eba27386ce19cec. Fix commits: 341c824 (step1.2), 65d2fd1 (step1.3), 1852131 (step1.4). Changed files: `tools/playtest/agent.sh` (+211/-? to 361 lines), `tests/test_playtest_agent.sh`, `tools/playtest/README.md`, the step file and ADR 0016.

## How I checked

- Read `git diff 3867dd6..1852131`, all of `tools/playtest/agent.sh` and `tests/test_playtest_agent.sh`, the step file, the plist and `packaging/scripts/run-client.sh`.
- Ran `bash tests/test_playtest_agent.sh <scratch>`: "all passed".
- Ran shellcheck: info-level items only (SC2012, SC2013, SC2015 at 198, SC2086 on the intentional pid lists). No warnings.
- On this Mac (bash 3.2.57) I checked:
  - The heredoc at agent.sh:91-95 is tab-indented, so `<<-` strips it correctly.
  - `osascript - "$1" "$2"` with an `on run argv` script on stdin passes `"` and `\` through intact.
  - With the readonly constants, an env file line `REPO=evil/x` is refused ("REPO: readonly variable", on stderr, which goes to agent.log through the plist), and REPO stays the same. `load_env` still returns 0.
  - **macOS `/usr/bin/shlock` never takes over a lock held by a dead pid.** With `-d` it prints "process N is dead" and then "lock time changed … >= 0", returns 1, and does the same for a lock file dated 2026-01-01. In practice the 30-minute `find -mmin +30` rule is the only stale-lock recovery. See finding 24.
- `run-client.sh` `exec`s the `bv2` binary, so `ours()` (`comm` matching `bv2|sleep`) recognises all play pids.
- I did not run the agent end to end. Nothing was edited.

## Round-1 findings

| # | Status | Note |
|---|---|---|
| 1 | Resolved | Readiness now waits for `GAME_PORT` to listen (agent.sh:209-210). The log grep is gone. |
| 2 | Resolved | `echo quit 1<>"$FIFO"` (245); the alive guard is gone. |
| 3 | Partly resolved | (b) is resolved: shlock creates the lock atomically with the pid. A held lock is now logged. (a) is resolved only through the 30-minute rule, because shlock does not do the dead-pid check (see 24). (c), the double-remove race on a stale lock, remains (nit 31). |
| 4 | Resolved, apart from the part deliberately not done | `ours()` checks each pid, `play_running` forgets a stale file, and `--stop` kills only our pids. Not notifying when a scheduled run skips because of play is acceptable: play is started by hand, the owner knows it is running, and the stale-pid case that made the skip permanent is fixed. |
| 5 | Resolved | `INFRA_STAGES`/`is_infra`. `ports-busy`, `no-gui-session`, `artifact`, `download`, `select` and `preflight` notify once and record nothing. `install` (unzip, e.g. disk full) and `local-start` (cp/mkfifo) still count as attempts. Acceptable. |
| 6 | Mostly resolved | Versions are keyed by SHA, reused when `BUILD` matches, and unzipped beside the old copy before the swap. Residuals are in nits 29 and 30. |
| 7 | Resolved | `REPORT` is the only "a build is being recorded" signal; the `SHA=""` lines are gone. |
| 8 | Resolved | `NAME` is set once in `begin_report`; `start_client` writes the cfg when given a name. |
| 9 | Resolved | `record_state` is extracted and tested (attempts 1, 2, 3, column order, pass → `-`); `prune` is tested against `current`; `is_infra` is tested. The `finish` branches (infra, interrupted) have no test, which is acceptable for an orchestration shell function. |
| 10 | Resolved | `load_env \|\| fail "env file"`. |
| 11 | Resolved | `KEEP_RUNNING`. |
| 12 | Resolved | Infra notification is skipped when `PLAY` is set. A residual for preflight failures in play mode is in nit 27. |
| 13 | Resolved | A TERM/INT trap sets `RESULT=interrupted`; `finish` records and notifies nothing for it. |
| 14 | Resolved | `osascript -` with argv (verified). |
| 15 | Resolved | `curl -K -` reads the URL from stdin. `-d "$2"` would read a file if a message began with `@`, but no message does. |
| 16 | Resolved | `readonly`. The override is refused and logged, but `load_env` does not fail (verified). That is enough. |
| 17 | Resolved | The artifacts JSON is captured first; `per_page=100`. |
| 18 | Resolved | `shlock`, `mkfifo` and `xattr` are added. `stat` is not, which is fine because it is in every macOS. |
| 19 | Resolved | `--sha`, `--test` and `--play` values are validated in the argument loop; `--play` together with `--sha` is rejected. |
| 20 | Resolved | ADR and comment now say "transfer integrity; the trust anchor is the main-only run filter". |
| 21 | Resolved / acceptable | Remaining items are info level. SC2015 at 198 (`mkdir … && cp … \|\| fail`) behaves correctly. |
| 22 | Nothing to change | Note it in the review record. |
| 23 | Not done, deliberately | Acceptable, as round 1 said. |

## REVIEW.md section 2 (fix commits)

- **Ground rules:** hold. Every changed file is inside the Allowed paths.
- **New behaviour has a test:** yes, for `record_state`, `is_infra` and `prune`.
- **Docs:** the README and the step-file deviation table are updated for every behaviour change (port readiness, infra stages, SHA-keyed versions, interrupt). The ADR wording is fixed. `ARCHITECTURE.md` has an uncommitted edit adding `reviews/step1*.md` rows; it needs to be committed with the review record.
- **Commits:** named `step1.N:`. They are not mixed mechanical/functional rewrites.
- **Structure (skill):** the file is 361 lines, far from 1k. The fixes deleted more concepts than they added: the magic `SHA=""`, the duplicated cfg block, the double env load, the log-format dependency and the FIFO guard are all gone. `INFRA_STAGES` with `is_infra` is a small, explicit policy table, not scattered ifs. `finish` is the busiest function: three outcomes and about 25 lines. It is still readable, but the two simplifications in 27 and 28 would remove two of its special cases.

## New findings

24. **should-fix** — `tools/playtest/agent.sh:124-126`
   - **Problem:** The code and the round-2 rationale assume `shlock` recovers a lock left by a dead pid. On macOS it does not: it returns 1 for a dead pid even when the lock file is months old (verified with `shlock -d`).
   - **Effect:** After a SIGKILL, launchd ExitTimeOut, a crash or power loss (the trap never runs), every run logs "locked by pid N; skipped" until the lock is 30 minutes old. This is bounded (about 3 skipped runs), so it is not a blocker. But the comment ("an older lock is stale even if its pid now belongs to another process") describes only the pid-reuse case and hides that this is the sole recovery path.
   - **Proposed change:** Pick one of these.
     - Reword the comment to "macOS shlock never takes over a dead pid's lock; a lock older than 30 min is removed (no run takes that long)".
     - Code judo: drop the 30-minute heuristic and remove the lock when its pid is dead or not ours: `p=$(cat "$ROOT/lock" 2>/dev/null); [ -n "$p" ] && ! ps -p "$p" -o command= | grep -q agent.sh && rm -f "$ROOT/lock"` before `shlock`. That gives immediate recovery.

25. **nit** — `tools/playtest/agent.sh:272, 274-276`
   - **Problem:** `infra-failure` is cleared only when a build result is recorded.
   - **Failure scenario:**
     - A `download` failure notifies once.
     - The network comes back, but no build is due, so every run takes the `exit 0` path at 324 and nothing clears the marker.
     - Next week the network fails again at `download` while a new build appears. That incident is silent.
   - **Proposed change:** Also `rm -f "$ROOT/infra-failure"` on any run that exits without `RESULT=fail`. For example, make the `finish` tail `[ "$RESULT" = fail ] || rm -f "$ROOT/infra-failure"`, and drop the removal inside the record branch.

26. **nit** — `tools/playtest/agent.sh:315` and `finish`
   - **Problem:** The TERM/INT trap stays active while `finish` runs. A second TERM during `stop_all`'s waits (launchd sends TERM, then the user hits Ctrl-C, or launchd re-signals) runs `exit 143` inside `finish`, after `trap - EXIT`. The lock and the processes are left behind, and the lock blocks runs for 30 minutes (see 24).
   - **Proposed change:** Start `finish` with `trap '' TERM INT` next to `trap - EXIT`.

27. **nit (simplification)** — `tools/playtest/agent.sh:83, 274, 348`
   - **Problem:** `PLAY` duplicates `MODE=play`, and it is set late (348). A `--play` run that fails at `preflight` (for example a locked keychain) still sends "RV2 agent FAILED at preflight". It also writes `infra-failure=preflight`, which suppresses the next unattended preflight notification: the round-1 finding 12 scenario through a different stage.
   - **Proposed change:** Delete `PLAY` and test `[ "$MODE" != play ]` in `finish`. Better, use `[ "$MODE" = smoke ]`: `--test staging-connect` is also manual and has no reason to touch the unattended infra marker.

28. **nit (simplification)** — `tools/playtest/agent.sh:217-218`
   - **Problem:** `stage gui-session` is followed by `{ STAGE=no-gui-session; fail …; }`. The override exists only so that the stage name hits `INFRA_STAGES`.
   - **Proposed change:** Put `gui-session` in `INFRA_STAGES` and drop the inline `STAGE=` reassignment. The only `fail` in that stage is the console check. One concept goes.

29. **nit** — `tools/playtest/agent.sh:162, 180`
   - **Problem:** The rest of finding 6. Reinstall triggers whenever `BUILD` differs. For the same `sha7`, that means a different `RUN_ID`: a second push run of the same commit, or a sha7 collision. In that case `rm -rf "$vdir" && mv` still deletes the live `current` package under a running `--play`, the same hazard round 1 described.
   - **Proposed change:** Accept and note it (rare), or compare only the SHA: `read -r s _ <"$vdir/BUILD"; [ "$s" = "$SHA" ]`. Any successful main run of the same commit gives the same package for test purposes, which deletes the case outright.

30. **nit** — `tools/playtest/agent.sh:179-182`
   - **Problem:** The layout check runs after the swap and after `BUILD` is written. A malformed package is therefore installed and marked as that build, and the next run takes the "already installed" path at 162 and skips the check. The failure still surfaces later (local-start/server-port) and still counts, so the impact is small, but the invariant "`BUILD` exists ⇒ package is valid" does not hold.
   - **Proposed change:** Check `"$dl/new/$PKG_NAME/bv2dedicated"` and the rest before writing `BUILD` and swapping.

31. **nit** — `tools/playtest/agent.sh:125-126`
   - **Problem:** The rest of finding 3(c). Two manual or launchd runs that both see a lock over 30 minutes old can each `rm -f` it, and the second removal can delete the first run's fresh shlock lock. launchd never overlaps its own job, so this needs a manual run at exactly that moment.
   - **Proposed change:** Accept. Option B in 24 (remove only when the pid is dead or not ours) narrows the race further.

## Verdict

**Approved.** Every round-1 blocker and should-fix is resolved, or deliberately and acceptably not done (4's skip notification, 23). The new findings are one should-fix (24, a misleading lock-recovery assumption whose impact is bounded to about 30 minutes) and nits.

Before closing the step:
- Fix or reword 24.
- Commit the pending `ARCHITECTURE.md` reviews rows.
- Complete the owner's on-Mac acceptance run, which neither review round performed: `--sha <latest>` READY TO TEST, ports-busy notification, `--play`/`--stop`.

Findings 25–31 can be fixed in passing or recorded in `possible-new-scope.md`.

## Round 3

SKILL: Launching skill: thermo-nuclear-code-quality-review

# Phase T step 1 review, round 3 (fix commit 1852131..77bb07c)

Head: 77bb07c (`step1.2: review round 2: lock recovery by pid identity, infra marker, signal-safe cleanup, SHA-only reuse, layout check before swap`). Changed files: `tools/playtest/agent.sh` (+17/-12, now 366 lines) and `tests/test_playtest_agent.sh` (+1).

## How I checked

- I read `git diff 1852131..77bb07c` and the whole current `tools/playtest/agent.sh`. I also grepped the README, the step file and ADR 0016 for references the change could have made stale (`no-gui-session`, `PLAY`, the lock, `BUILD`). None are stale.
- `bash tests/test_playtest_agent.sh <scratch>` prints "all passed". `shellcheck -S warning` reports nothing.
- I tested these on this Mac (bash 3.2, `/usr/bin/shlock`), in a scratch directory:
  - **shlock lock file format:** the file holds the pid followed by a newline (`31440\n`), so `p=$(cat lock)` gives a clean pid.
  - **Dead-pid lock:** a lock holding 99999 is removed by the new check, and `shlock` then succeeds (rc 0). Finding 24's recovery works.
  - **Garbage lock (`abc`):** `ps -p abc` fails, so the lock is removed and recovered. Correct.
  - **Empty lock file:** `shlock -p $$ -f <empty file>` returns 1. The new code skips removal when `$p` is empty (`[ -n "$p" ]`), so an empty lock is never recovered. See 32.
  - **`read -r installed _ <missing 2>/dev/null`:** rc 1 and `installed` stays empty, so the logic is correct. But redirections are applied left to right: the failed `<` happens before `2>/dev/null` takes effect, and bash prints `…/BUILD: No such file or directory` to stderr. With `2>/dev/null <file` nothing is printed. See 33.
  - **`trap '' TERM INT` and finish's exit status:** a false last command in the EXIT trap does not change the script's exit code after `exit 1`, `exit 0` or a fall-through end. So the trailing `[ -n "$LOCKED" ] && rm …` (false when there is no lock) and the `[ "$MODE" = smoke ] && …` line cannot turn a pass into a failure, and a fail keeps 1. Ignoring TERM/INT inside `finish` is correct: the first signal's handler already set `RESULT=interrupted` and called `exit 143`. The only child that inherits the ignored disposition is the short `sleep 1` in `wait_gone`. launchd's SIGKILL after ExitTimeOut still applies.
- I did not run the agent end to end. Nothing was edited, committed or pushed.

## Round-2 findings

| # | Status | Note |
|---|---|---|
| 24 | Resolved | The fix took option B: the lock is removed when its pid is not a running `agent.sh` (agent.sh:124-126), and the 30-minute heuristic is gone. Dead and garbage pids now recover at once. The new comment is accurate. There is one gap: an empty lock file (32). |
| 25 | Resolved | The marker is cleared by any `MODE=smoke` run whose result is not `fail` (agent.sh:283). This covers the "not due → `exit 0`" and "play running" paths. One small residual regression is in 35. |
| 26 | Resolved | `trap '' TERM INT` at agent.sh:259. Verified as described above. |
| 27 | Resolved | `PLAY` is deleted. The infra notification and the marker are gated on `MODE=smoke`, so `--play` and `--test staging-connect` never touch the unattended marker. |
| 28 | Resolved | `gui-session` is in `INFRA_STAGES`, the `STAGE=no-gui-session` override is gone, and there is a test (test:58). The README's "no console session" wording still matches. |
| 29 | Resolved | Reuse compares only the SHA (agent.sh:163-164), so a second main run of the same commit no longer deletes the live package. Residual: the recorded run id can differ from the installed one (38). |
| 30 | Resolved | The layout check runs on `$dl/new` before `BUILD` is written and before the swap (agent.sh:181-185). The invariant "`BUILD` exists ⇒ the package is complete" now holds for new installs. |
| 31 | Accepted in round 2; narrowed | The double-remove race now needs a lock whose pid is dead and two runs at the same moment. No action. |

No earlier finding is repeated below.

## Structure (skill)

The commit deletes concepts: the `PLAY` flag, the `STAGE=` override, the 30-minute staleness heuristic and the post-swap check. It adds nothing new. `finish` is still the busiest function, at about 28 lines with three outcomes plus the marker and lock tail, and still readable. The file is 366 lines, far below 1k. There is no structural regression and no missed code-judo move of any size.

## New findings

32. **should-fix** — `tools/playtest/agent.sh:124-127`
   - **Problem:** an empty lock file is never removed (`[ -n "$p" ]` short-circuits), and macOS `shlock` refuses an empty lock file (verified: rc 1). Every run would then log `locked by pid ; skipped` and `exit 0`, with no notification, forever. The round-2 code had the 30-minute rule as a backstop for this case; the new code has none. shlock writes a temp file and then `link`s it, so an empty file is unlikely. A crash or power loss between the write and the flush, or a manual `touch`, can still leave one, and the effect is a permanent silent stop of the agent.
   - **Proposed change:** delete `[ -n "$p" ] &&`. `ps -p ""` fails ("Invalid (zero-length) process id", rc 1, verified), so an empty or missing lock is removed, and `rm -f` on a missing file does nothing. This removes a condition instead of adding one:
     `p=$(cat "$ROOT/lock" 2>/dev/null); ps -p "$p" -o command= 2>/dev/null | grep -q agent.sh || rm -f "$ROOT/lock"`

33. **nit** — `tools/playtest/agent.sh:163`
   - **Problem:** `read -r installed _ <"$vdir/BUILD" 2>/dev/null` writes `…/versions/<sha7>/BUILD: No such file or directory` to stderr, and so to `agent.log` through the plist, on every first install of a build. Bash applies redirections left to right, and the input redirect fails before stderr is redirected (verified). The logic is unaffected.
   - **Proposed change:** `read -r installed _ 2>/dev/null <"$vdir/BUILD"` (verified silent).

34. **nit** — `tools/playtest/agent.sh:126`
   - **Problem:** `grep -q agent.sh` matches any command line containing that substring, for example `test_playtest_agent.sh` (the ctest run), `vim …/agent.sh` or `less agent.sh`. If a dead holder's pid is reused by such a process, the lock is kept until that process exits. This needs pid reuse plus that specific process, so it is very unlikely and only delays runs.
   - **Proposed change:** accept, or tighten it to `grep -q '/bin/bash .*/agent\.sh'` (the plist and manual runs both go through `/bin/bash`).

35. **nit** — `tools/playtest/agent.sh:283`
   - **Problem:** before this commit, a recorded build result (pass or fail) also cleared the infra marker. Now a recorded build failure does not, even though it proves the infrastructure worked up to `verify` or later.
   - **Scenario:**
     - `download` fails, the owner is notified, and `infra-failure=download`.
     - The network recovers, and build A installs and fails at `client-join`. This is recorded, and the marker stays.
     - Build B appears during A's retry window, and its download fails again. That incident is silent.
   - The window is narrow because it closes on the first not-due or passing run.
   - **Proposed change:** accept, or clear the marker unless this run is an infra failure: `[ "$MODE" = smoke ] && ! { [ "$RESULT" = fail ] && is_infra "$STAGE"; } && rm -f "$ROOT/infra-failure"`. That reuses the predicate already used at agent.sh:264.

36. **nit** — `tools/playtest/agent.sh:333-334, 221-222`
   - **Problem:** `gui-session` is now an infra stage, but the console check runs after `start_local`. When another user holds the console (fast user switching), every 10-minute run starts master and server, fails at `gui-session`, and tears them down again. This is harmless but wasteful. It predates this commit; the commit only made the stage infra.
   - **Proposed change:** accept, or move the console check before `start_local`, for example into the smoke path before `begin_report`.

37. **nit** — `tools/playtest/agent.sh:164, 291, 266`
   - **Problem:** this is a consequence of the SHA-only reuse in 29. When a build is reused, `state.tsv` and the report header record the `RUN_ID` from `select_run`, while the installed package (and `BUILD`, which `use_current` later reads) comes from the earlier run. The report's run link can point to a run whose artifact was never downloaded. The commit is the same, so the test meaning is unchanged.
   - **Proposed change:** accept. If you want the record to be exact, set `RUN_ID` from `BUILD` on the reuse path: `read -r installed RUN_ID …`, then compare `installed` with `SHA` and restore the original run id on a mismatch. That adds a line, so accepting is probably better.

38. **info** — `tools/playtest/README.md:45`
   - **Problem:** "Failures before the build is tested … notify once" now applies only to scheduled and `--sha` smoke runs. In `--test` and `--play`, an infra failure prints to the terminal only. This is consistent with 27's intent, and both modes are attended.
   - **Proposed change:** none required. Optionally add "(scheduled runs)" to the sentence.

## Verdict

**Approved.** All round-2 findings 24-30 are resolved, and 31 is accepted and narrowed. The commit removes complexity (`PLAY`, the stage override, the time heuristic) without adding any.

Recommended before closing, each a one-line change:
- **32:** drop `[ -n "$p" ] &&` so an empty lock file cannot silently stop the agent for good.
- **33:** reorder the redirect.

Findings 34-38 can be accepted or recorded in `possible-new-scope.md`.

The open items from round 2 still stand:
- Commit the pending `ARCHITECTURE.md` reviews rows. It is still uncommitted, along with `reviews/`.
- The owner's on-Mac acceptance run.

## Round 4

SKILL: Launching skill: thermo-nuclear-code-quality-review

# Phase T step 1 review, round 4 (fix commit 77bb07c..da92600)

Head: da92600. Changed files: `tools/playtest/agent.sh` (4 lines changed) and `tools/playtest/README.md` (1 line changed).

## How I checked

- I read `git diff 77bb07c..da92600`, the surrounding code in `agent.sh` (`lock`, `finish`, `is_infra`, the main flow at agent.sh:318-335) and the plist's `ProgramArguments`, which are `/bin/bash`, `__HOME__/RV2-playtest/bin/agent.sh`.
- `bash tests/test_playtest_agent.sh <scratch>` prints "all passed". `shellcheck -S warning tools/playtest/agent.sh` reports nothing.
- I tested these on this Mac (bash 3.2, `/usr/bin/shlock`) in a scratch directory, using a stub `agent.sh` with a `#!/bin/bash` shebang, and read each one's `ps -p <pid> -o command=`:

| How it was started | `ps` command | Matches `'/bin/bash .*/agent\.sh'` |
|---|---|---|
| plist: `/bin/bash /abs/bin/agent.sh` | `/bin/bash /abs/bin/agent.sh` | yes |
| shebang, absolute path | `/bin/bash /abs/bin/agent.sh` | yes |
| shebang, `bin/agent.sh` | `/bin/bash bin/agent.sh` | yes |
| shebang, `./agent.sh` | `/bin/bash ./agent.sh` | yes |
| `bash bin/agent.sh` or `bash /abs/agent.sh` | `bash …/agent.sh` | **no** |
| `/bin/bash agent.sh`, from the script's own directory | `/bin/bash agent.sh` | **no** |
| ctest, `/bin/bash …/tests/test_playtest_agent.sh` | — | no (correct) |

- **Missing lock file:** `cat` fails, so `p=""`. `ps -p ""` fails, so the code runs `rm -f` on a file that isn't there, which does nothing. `shlock` then creates the lock (rc 0). No issue.
- **Empty lock file:** it is now removed, and `shlock` then succeeds. Finding 32 is fixed.
- **`read -r installed _ 2>/dev/null <missing`:** rc 1, `installed` is empty, and nothing is printed to stderr.

Nothing was edited, committed or pushed.

## Earlier findings

| # | Status | Note |
|---|---|---|
| 32 | Resolved | The `[ -n "$p" ]` guard is gone, so an empty or missing lock is removed and recovered. The trailing comment is accurate. |
| 33 | Resolved | The redirect order is `2>/dev/null <"$vdir/BUILD"` (agent.sh:163). Verified silent. |
| 34 | Resolved, with a regression | The pattern is tighter, and editors, pagers and `test_playtest_agent.sh` no longer match. But it now also misses `bash <path>/agent.sh`. See 39. |
| 35 | Resolved | agent.sh:283 now clears the marker unless this run is an infra failure, reusing `is_infra`. Round 3's scenario (build A fails at `client-join`) now clears the marker. Runs that are `interrupted`, or that exit unexpectedly before `begin_report` (`RESULT` empty), also clear it. The worst case is one extra notification, never a silent incident, which is the right way to fail. |
| 38 | Resolved | The README now says "In these scheduled runs, …". |
| 36, 37 | Accepted earlier | Not re-raised. |

## Structure (skill)

The commit adds no concepts and deletes one condition (32). The file is still about 366 lines.

One small duplication: the predicate `! { [ "$RESULT" = fail ] && is_infra "$STAGE"; }` now appears twice in `finish` (agent.sh:264 and agent.sh:283). See 40. There is no structural regression and no missed code-judo move.

## New findings

39. **should-fix (low)**: `tools/playtest/agent.sh:126`
   - **Problem:** the new pattern requires the literal `/bin/bash ` followed later by `/agent.sh`. The plist run and every shebang run match. A manual `bash ~/RV2-playtest/bin/agent.sh --sha <sha>` (or `--test`, or `/bin/bash agent.sh` run from `bin/`) does not match. The old pattern, a plain `agent.sh` substring, did.
   - **Scenario:**
     - The owner runs `bash ~/RV2-playtest/bin/agent.sh --sha X` for the acceptance check.
     - The 10-minute launchd run starts during it. It sees a live lock whose holder "is not an agent", deletes it and takes the lock.
     - Two agents now run at once, racing on `versions/` and the ports. The scheduled run likely fails at `ports-busy` as an infra failure and sends a spurious notification.
     - When the first agent finishes, its `finish` deletes the lock the second one holds.
   - The README documents running the script directly (shebang), so this needs the `bash …` habit. That is why the severity is low. But the step-1 change made a valid lock unsafe for that habit, which is the one thing the lock must prevent.
   - **Proposed change:** match the script by basename after any `bash`, without matching other `*agent.sh` names:
     `ps -p "$p" -o command= 2>/dev/null | grep -qE 'bash ([^ ]*/)?agent\.sh( |$)' || rm -f "$ROOT/lock"`
   - Verified: it matches all nine invocation forms above, including `bash agent.sh`, `bash /abs/agent.sh --play local` and `/opt/homebrew/bin/bash agent.sh`. It rejects `/bin/bash …/test_playtest_agent.sh`, `vim …/agent.sh` and `less agent.sh`.

40. **nit**: `tools/playtest/agent.sh:264, 283`
   - **Problem:** the expression "this run is an infra failure" is spelled out twice in `finish`. The next change to that rule has to be made in both places.
   - **Proposed change:** compute it once at the top of `finish`, then use it in both places:
     - `local infra=""; [ "$RESULT" = fail ] && is_infra "$STAGE" && infra=1`
     - agent.sh:264: `elif [ -n "$REPORT" ] && [ -z "$infra" ]; then`
     - agent.sh:283: `[ "$MODE" = smoke ] && [ -z "$infra" ] && rm -f …`
   - Accepting this as is is also fine.

## Verdict

**Approved, with 39 recommended before closing.**

- Findings 32, 33, 35 and 38 are resolved correctly.
- 34 is resolved but introduced 39: the tighter lock pattern no longer recognises an agent started as `bash <path>/agent.sh`, so a scheduled run can steal a live manual run's lock. It is a one-line fix.
- 40 is optional.

Open items carried over from round 3:
- Commit the `ARCHITECTURE.md` reviews rows and `reviews/`.
- The owner's on-Mac acceptance run. If 39 is not fixed, that run should use `~/RV2-playtest/bin/agent.sh --sha …`, not `bash …`.

## Round 5

SKILL: Launching skill: thermo-nuclear-code-quality-review

# Round 5 review: step 1 (commit c8c33a2, `tools/playtest/agent.sh` only, +6/-4)

## Status of earlier findings

- **39: resolved.** `agent.sh:126` now uses the proposed pattern `grep -qE 'bash ([^ ]*/)?agent\.sh( |$)'`. I checked it on this Mac:
  - It matches `/bin/bash /Users/a/RV2-playtest/bin/agent.sh`. That is the plist form: `ProgramArguments` = `/bin/bash`, `__HOME__/RV2-playtest/bin/agent.sh`. It is also the shebang form, because the shebang is `#!/bin/bash`.
  - It also matches `bash agent.sh --sha x` and `/opt/homebrew/bin/bash ./agent.sh`.
  - It rejects `/bin/bash /x/tests/test_playtest_agent.sh`, `/bin/bash /x/agent.sh.bak` and `vim agent.sh`.
  - The group `([^ ]*/)?` must end in `/` right before `agent.sh`, so `*_agent.sh` names cannot match.
  - The macOS `ps` does not truncate `-o command=` when output goes to a pipe. The match also sits at the start of the line, so truncation could not hide it anyway.
- **40: resolved.** `finish` now computes `infra` once (`agent.sh:262-263`). The record branch (`:266`), the notify branch (`:279`) and the marker clear (`:285`) all use it.

## Correctness of the changed lines

**Record branch.** `[ -n "$REPORT" ] && [ -z "$infra" ]` gives exactly the old result, since `! { fail && is_infra }` ⇔ `-z infra`. No change.

**Infra-notify branch.** This line changed meaning: the condition went from `RESULT = fail` to `infra` (fail and an infra stage). Only one kind of run behaves differently: smoke mode, `RESULT=fail`, `REPORT` empty, a non-infra stage. That run would now get neither a record nor a notification. I checked whether such a run can occur:
- In smoke mode, `REPORT` stays empty until `begin_report`. Before that the agent only goes through `stage preflight` (`:324`) and `select_run`, which sets `stage select` (`:152`). Both are in `INFRA_STAGES` (`:19`).
- `due` sets no stage.
- `begin_report` runs before `fetch_install`, `start_local` and `start_client`. So every non-infra stage (`verify`, `install`, `local-start`, `server-port`, `master-port`, `client-start`, `client-join`) runs with `REPORT` set and takes the record branch.
- The script uses only `set -u`, not `set -e` (`:9`). An unexpected exit before the report leaves `RESULT` empty, not `fail`. That gives no record and no notify, both before and after this commit.
- Non-smoke modes (`test`, `play`) never reached this branch; it was guarded by `MODE = smoke` before and still is. Those modes have their own non-infra failures without a report: `play` "RV2_STAGING_HOST is not set" (`:362`), and `use_current` failing under `stage preflight`. They were silent before as well.

So no failing run falls through without a record or an infra notification. The change is behaviour-equivalent for every reachable state. It also makes the predicate cleaner: "notify an infra failure" now reads as exactly that, where before it was "any fail that wasn't recorded".

**Marker clear.** `[ "$MODE" = smoke ] && [ -z "$infra" ]` is the exact rewrite of the old expression. No change.

**Structure.** One local flag replaces two copies of the same compound test, and no new branch was added. `finish` is now simpler to read than in round 4. Nothing in this diff calls for structural change.

## Non-findings (corner cases, not raised under AGENTS.md "Ignore Corner Cases")

- `bash -x agent.sh` (a debug run) and a `$HOME` path containing spaces do not match the lock pattern. A scheduled run could then take over that run's lock. Both need a deliberate unusual invocation or an unusual home path.
- A smoke run that exits early clears the infra marker even though select never ran: "manual play running; skipped" (`:331`). This was already the behaviour before this commit, and the diff does not touch it.

## New findings

None.

## Verdict

**Approved.** Findings 39 and 40 are resolved correctly, the diff introduces no regression, and there are no new findings.

Open items carried over from round 4 (not code):
- Commit the `ARCHITECTURE.md` reviews rows and `reviews/`.
- The owner's on-Mac acceptance run. Since 39 is fixed, starting it with `bash …/agent.sh` is now safe.

## Round 6

SKILL: Launching skill: thermo-nuclear-code-quality-review

## Round 6: phase T step 1 (c8c33a2..refactor/phase-t-step1-mac-pull-agent, step's own files only)

Scope reviewed: 015abbe (`--placeholders`), the ADR renumbering 0016 to 0017 from the merge resolution, the review-record commits (eec4cb3, 261885b), and the step's rows in `ARCHITECTURE.md`, `docs/decisions/README.md` and the step file. I did not review Phase C's own files (ADR 0016, Paths, DataDirPicker, osv-vcpkg and their inventory rows).

### Checks

1. **`--placeholders` only when a player name is given: correct.**
   - The only callers that pass a name are the automated tests: local-smoke at `tools/playtest/agent.sh:337` and staging-connect at `:349`. Both are unattended, and `has_player` waits 30 s, so a modal first-run dialog would make them fail. The `--play` callers at `:361` and `:364` pass no name. They get the dialog, which saves into `$ROOT/play-pref`. That is the correct interactive behaviour.
   - On the branch, the client skips the flag when building the console command (`game/src/main.cpp:579`), and `chooseDataDir` returns before the saved, detected or dialog lookup (`game/src/DataDirPicker.cpp:80`). `run-client.sh` passes `"$@"` through unchanged, so the flag reaches the binary.
   - Linking "name given" to "automated" through `cfg` merges two concepts into one variable. With exactly two caller shapes, the updated header comment ("a name marks an automated test") makes the rule explicit. A separate mode flag would add a parameter without removing a branch, so I don't ask for one.
2. **`${cfg:+--placeholders --config "$cfg"}` is safe.** I checked it on the target shell (`/bin/bash` 3.2.57):
   - With `cfg="/a b/*c.cfg"` it gives exactly three words: `--placeholders`, `--config`, `/a b/*c.cfg`. The quoted path is not split or globbed.
   - With an empty `cfg` it gives nothing.
   - The `ROOT` default `$HOME/RV2-playtest` has no spaces, but a custom `RV2_PLAYTEST_ROOT` with spaces is also safe.
3. **ADR numbering is consistent.**
   - Every reference to the playtest agent's ADR now says 0017: `agent.sh:2`, `tools/playtest/README.md:3`, step file Deliverables and §1.5, `ARCHITECTURE.md:252` and `:786`, and `docs/decisions/README.md:51`. The 0017 file is a pure rename plus its title line.
   - The remaining 0016 references in `ARCHITECTURE.md` (`:24`, `:63`, `:251`, `:650`) and `agent.sh:230` correctly mean Phase C's player-data ADR.
   - No stale `0016-playtest-pull-agent` link remains outside the historical raw report.
4. **Step file deviation row (`step1-mac-pull-agent.md:63`).** The row is accurate and states the reason. One gap is finding 41 below.

### New findings

**41: Low.** `docs/roadmap/phase-t-testing-environment/step1-mac-pull-agent.md:63`
- Problem: the row says builds before `67fd59f` "don't know the flag" but not what happens. On those builds, `main.cpp` (as at c8c33a2) joins every argument into one console command. The client therefore runs `--placeholders connect 127.0.0.1`, never connects, and `has_player` fails.
- Impact: `agent.sh --sha <pre-67fd59f>` or `--test staging-connect --sha <old>` records a build failure and uses up an attempt, but the build itself is not broken. The newest `main` already contains `67fd59f`, so scheduled runs are not affected.
- Proposed change: end the row with "...don't know the flag: `--sha` on them fails at the join check". No code change; a version guard would be defensive code for a rare manual case.

**42: Low (note).** `tools/playtest/agent.sh:231`
- Problem: `chooseDataDir` checks `BV2_DATA_DIR` before `--placeholders` (`DataDirPicker.cpp:78-82`). A manual `agent.sh` run from a shell that exports `BV2_DATA_DIR` therefore tests on the original assets, while the LaunchAgent run tests on placeholders. That contradicts the new comment at `:230` ("run windowed on placeholder content").
- Proposed change: either soften the comment to "...on placeholder content unless `BV2_DATA_DIR` is set", or start the automated client with `env -u BV2_DATA_DIR`. The comment fix is enough under the MVP rule.

Review-record commits: `reviews/step1.md` correctly maps findings 1-40 to their fixes and rejections, and both new files have inventory rows. The record still covers the review only up to c8c33a2. When it is updated, it should add this round (015abbe and the renumbering) and its review.sh run. That is an expected bookkeeping step, not a finding.

No structural regressions: the code change is one argument added to an existing call, plus comments. There are no new branches, no file-size change and no layer leaks.

### Verdict

**APPROVED.** Findings 41 and 42 are optional Low wording fixes and do not block. The step stays IN PROGRESS only for the owner-dependent LaunchAgent acceptance check already listed in `reviews/step1.md`.
