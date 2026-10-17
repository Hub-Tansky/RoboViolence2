## Round 2

SKILL: Launching skill: thermo-nuclear-code-quality-review

**Scope.** I reviewed `git diff origin/main...HEAD`: commits 642f957..4c5f8ef, not counting merge 59f81bd. The main change is the new `package-smoke` job in `.github/workflows/build.yml:133-203` (+71 lines; the file is now 283 lines, well under 1k). The rest is docs: the AGENTS.md CI list, the ARCHITECTURE.md `build.yml` row plus the three new file rows, the phase README row, the step file, PNS-69, and the round 1 record and report. I read REVIEW.md, the step file, the round 1 record (`reviews/step3.md`), the round 1 report (`step3-report.md`), the `packaging/scripts/run-*` scripts and the existing `package-check` job. The review applies the skill's rubric, then REVIEW.md §2. I did not open CI logs or run `tools/review.sh`.

**Verified as correct**
- **Allowed paths:** every changed file is inside them.
- **Anti-gaming:** none of the listed patterns appear (`continue-on-error`, `|| true` around a check).
- **Commits:** no mechanical rewrite is mixed with functional changes.
- **Round 1 fixes, all present in 0dcafa3:**
  - The job name is stable: `package-smoke (${{ matrix.preset }})`.
  - The zip name is exact (`build.yml:154`), and the debug `ls -l` is gone.
  - Client output goes to `client.log`, which is printed on both the success and the early-exit path (`:197-201`).
  - A comment says why Windows starts `bv2.exe` directly (`:193`).
- **Round 1 deferral:** PNS-69 exists and states the deferred Mesa policy. The step file now gives the source and how the hash was taken.
- **Start signal:**
  - `run-client.sh` and `run-master.sh` both `exec`, so `kill "$pid"` reaches the real process on macOS.
  - `run-server.cmd` ends with the `bv2dedicated.exe` call, so `pipefail` sees the server's exit code on Windows too.
  - `console.log` is created only by the client's second `Console` construction (`game/src/main.cpp:558`), after init. A separate pref dir per step keeps the server's `console.log` (`main.cpp:350`) out of the client check.
- **Mesa download:** pinned by version and SHA-256. It goes only into the Windows package folder.
- **Docs:** AGENTS.md, ARCHITECTURE.md (rows for `step3-ci-package-smoke.md`, `step3.md` and `step3-report.md`) and the phase README are consistent with the job. No ADR is needed beyond PNS-69, which round 1 deferred; I do not re-raise it.
- **Repeats:** none of the findings below repeats a round 1 or earlier-step finding. The Mesa ADR (round 1 #1) and the commit split (round 1 #6) are not raised again.

### Findings

1. **Medium: the server and master start check now exists three times** (`build.yml:127-132` in `package-check`, and `:174-185` in `package-smoke`).
   - **Problem:** the server `quit` pipe, the `Server Created on port 3333` grep, the 20×0.5 s wait loop for TCP 10207, the `exec 3<>` assertion and the master kill were copied from `package-check` and then forked per OS. A future change to the start contract has to be made in three places: the port, the banner text or the wait budget. This is the skill's "copy-pasted logic instead of extracted helpers".
   - **Fix (code judo):** move the check into one script, for example `tools/package-start-check.sh <package-dir>`, that takes the server and master commands per OS, and call it from both jobs. `package-smoke` then shrinks to download, Mesa and two calls. `tools/` and composite actions are outside this step's Allowed paths. So either get the owner's approval to widen them for this one file, or defer to a new PNS entry and say so in the record.

2. **Low: Windows-specific workarounds with no comment and inconsistent handling** (`build.yml:170`, `:176`, `:185`, `:194`).
   - **`MSYS_NO_PATHCONV: "1"` (`:170`):** it is set for the server step with no reason given. Presumably it stops Git Bash from turning `cmd /c` into a path.
   - **Master start and stop (`:176`, `:185`):** the master is started as `cmd /c run-master.cmd &` and stopped with `taskkill /F /IM bv2master.exe`. The unstated reason is that `$!` is `cmd`'s pid, and killing `cmd` leaves `bv2master.exe` running. Round 1 #5 fixed the same gap for the client only.
   - **Paths (`:173`, `:194`):** the client step runs `$RUNNER_TEMP` through `cygpath -m`, but the server step passes the raw `D:\a\_temp/pref-server` and works. So either the `cygpath` is dead code, or the server step relies on luck.
   - **Why it matters:** these are exactly the lines a later agent will "simplify" and break.
   - **Fix:**
     - Add a one-line reason to `MSYS_NO_PATHCONV`.
     - Add a one-line reason to the `taskkill`.
     - Either drop `cygpath -m`, or apply it in both steps with a comment.

3. **Low: four `RUNNER_OS` branches inside two steps** (`:174`, `:185`, `:194`, plus the extract at `:155`).
   - **Problem:** the OS choice is made again and again at run time when the matrix already has it. Each step reads as two interleaved scripts.
   - **Fix:** put the per-OS commands in the matrix `include` rows, for example `server: cmd /c run-server.cmd`, `master: cmd /c run-master.cmd`, `stop-master: taskkill /F /IM bv2master.exe` and `client: ./bv2.exe`, against `./run-server.sh` etc. The step bodies then become single flows with no `if`.
   - If finding 1's script is adopted, these rows become its arguments. This is a matter of taste with a modest gain, so confirm with the owner before doing it alone.

4. **Low: nothing yet shows the start-signal assertion can fail** (`build.yml:203`; round 1 record "Fix proof" says "none yet").
   - **Problem:** REVIEW §2 wants new behaviour tested. Here the job *is* the test, but no run shows that `ls -l "$BV2_PREF_DIR/console.log"` fails when init fails. Without that, the job could stay green while the client sits in an init-failure message box.
   - **Fix:** run CI once, on a throwaway commit or a manual run, with the Mesa step skipped on Windows. Expect no `console.log` and a red job, and link that run (or paste its `ls: cannot access` line) under "Fix proof" in `reviews/step3.md`.

5. **Info: the record does not yet cover the step's final head** (`reviews/step3.md`).
   - **Problem:** its `Diff:` line is `baf8ac7..c48e65c`, but the step now runs to 4c5f8ef: 0dcafa3 changed `build.yml`, and 4c5f8ef changed docs. `tools/review.sh --full <preset>` has not been run on the final head.
   - **Fix:** update `Diff:` to the final head. Add this round's findings, paste the `--full` summary (or CI evidence, if the owner accepts that), and keep Status at IN PROGRESS until then (REVIEW §5.5).

## Round 2, second run (separate thread, same head 59f81bd)


SKILL: Launching skill: thermo-nuclear-code-quality-review

#### Scope

I reviewed `git diff 5034107adfc21b7caf7b6fd3c0511f077ec31be9..HEAD` with HEAD at 59f81bd, the merge of `main` into the PR #68 head. The range has eight step commits, 642f957 to 4c5f8ef, plus the merge, and touches 8 files (+220/-3). The merge's own changes (ADR 0023, REVIEW.md, the agent and skill files) come from `main` and are not in the range. I read these in full or over their whole changed region:
- `.github/workflows/build.yml` lines 1–283, which covers the whole `build`, `release`, `package-check`, `package-smoke` and `smoke` jobs.
- The changed rows in `AGENTS.md` and `ARCHITECTURE.md`.
- The phase README, the step file, `possible-new-scope.md` PNS-69, `reviews/step3.md` and `reviews/step3-report.md`.
- `REVIEW.md` and the step 1 and step 2 records.

Because the job depends on the client binary, I also followed it into these files:
- `packaging/scripts/run-{client,server,master}.{sh,cmd}`.
- `game/src/main.cpp:455-495` and `:545-565`, the init order and where the console is built.
- `game/src/Console.cpp:48-53`, where `console.log` is created with `userFile`.
- `tools/review.sh:40-80`, the scope check, and `hygiene.yml:25-26`.
- Plan rows T2 (`docs/roadmap/20261014_testing_environment_plan.md:47`, `:167-170`).

I did not open CI logs or run `tools/review.sh`.

#### Verified correct

- **Start signal.** `console.log` is truncated in the `Console` constructor (`Console.cpp:51-53`). The constructor is first reached at `main.cpp:558`, after window, input, GL, sound, network and the BaboNet version check. Every earlier failure returns through `dkwShowMessage` before the file exists. The client has its own `BV2_PREF_DIR`, so the server step cannot leave a `console.log` that hides a missing one.
- **Process handling.** `run-client.sh` uses `exec`, so `$!` is the client on macOS. `run-client.cmd` uses `start ""`, so starting `bv2.exe` directly on Windows is required, and the reason is written down at `build.yml:193`. `cmd /c run-master.cmd &` cannot be stopped through `$!`, so `taskkill /IM bv2master.exe` is needed. With `defaults.run.shell: bash`, every step runs with `-eo pipefail`.
- **Extraction.** `skip-decompress` plus `ditto -x -k` keeps the exec bits and the bundle symlinks on macOS.
- **Mesa download.** It is pinned to a version and a SHA-256 checked with `sha256sum -c`. The workflow has `permissions: contents: read`.
- **Docs.** The AGENTS.md CI list and the ARCHITECTURE.md `build.yml` row are updated. The step file, record and report rows are present, and the phase README has the step row. No module README is affected; `docs/build/linux.md` only describes `package-check`.
- **Round 1 fixes** are in 0dcafa3: the stable job name, the exact zip name, `client.log` printed on both paths, and the `bv2.exe` comment.
- **Rules.** No anti-gaming patterns. No mechanical rewrite is mixed with functional changes. Status is still IN PROGRESS, so the DONE gate is not triggered yet.
- **Not re-raised:**
  - Mesa ADR, bump policy and hash: deferred → PNS-69.
  - Task 3.2 spread over several commits: rejected in step3.md #6.

#### Findings

1. **Medium: the server and master start check is copied, with OS branches added** (`.github/workflows/build.yml:173-185` duplicates `:126-132`).
   - **Problem:** the server pipe, the `Server Created on port 3333` grep, the 20×0.5 s TCP 10207 wait loop and the master stop now exist twice: once in `package-check` and once in `package-smoke`, where they are interleaved with `RUNNER_OS` branches. The readiness criteria are the contract of "the package starts", and they now drift independently. If the port, the log line or the timeout changes, someone has to remember both places.
   - **Fix (code judo):** move the check into one script, for example `packaging/scripts/` or `tools/package-start-check.sh <pkgdir>`, called by both jobs. Both files are outside this step's Allowed paths, so get the owner's approval to widen them, or record the outcome as `deferred → PNS-n`.

2. **Low–Medium: OS differences are written as inline `if` branches instead of matrix data** (`build.yml:155`, `:174-180`, `:185`, `:194`, `:190`).
   - **Problem:** the job has four `if [ "$RUNNER_OS" = Windows ]` branches. Each one picks a value or a command that is fixed per matrix entry: the extractor, the server and master launchers, the client launcher, and how the master is stopped.
   - **Problem:** `GALLIUM_DRIVER: llvmpipe` is set in a shared step on macOS, where it means nothing.
   - **Fix:** put the values in the matrix `include` entries, for example:
     - `client: ./bv2.exe` / `./run-client.sh`
     - `server: cmd /c run-server.cmd` / `./run-server.sh`
     - `extract: 7z x -bd` / `ditto -x -k`

     Then the steps are a single straight-line script, and the only branch left is the master stop. Set `GALLIUM_DRIVER` only where Mesa is installed, for example as a matrix `env` value or written to `$GITHUB_ENV` in the Mesa step.

3. **Medium: no run has shown that the start signal fails when init fails** (`build.yml:202-203`; `reviews/step3.md` "Fix proof").
   - **Problem:** the job's central claim is "console.log exists only after init succeeded". The record says "Evidence that the start signal fails without a working init: none yet". Under REVIEW §2, a new CI job is new behaviour, and a test that has never been seen red does not prove it can detect anything. The code reading above supports the claim. What is missing is an observed negative run, for example Windows without the Mesa step, where GL init must fail.
   - **Fix:** run the job once on a throwaway commit with the Mesa step removed or skipped. Expect `console.log` missing (`ls: cannot access …`) while the process is still alive in the message box. Paste that output into `reviews/step3.md` under Fix proof.

4. **Medium (process): the branch name skips the CI scope check** (branch `claude/project-thread-9izxz9`; `tools/review.sh:51-55`, `:66-79`; `hygiene.yml:26`).
   - **Problem:** AGENTS.md requires `refactor/<phase>-stepN-<slug>`. `review.sh` derives the step only from that pattern. On this PR, the CI `review` check therefore runs without a step: the Allowed-paths check does not run, and the `.github/workflows/build.yml` change only prints a guard NOTE. The only scope evidence is the author's local `--step` run.
   - **Fix:** push the step from `refactor/phase-t-step3-ci-package-smoke` so CI enforces scope. If the PR stays on this branch, state in `reviews/step3.md` that CI scope was not enforced and paste the local `tools/review.sh --step …` output.

5. **Low: the two steps handle `$RUNNER_TEMP` differently** (`build.yml:173` vs `:192-195`).
   - **Problem:** the server step passes `$RUNNER_TEMP` unconverted to a native Windows exe through `BV2_PREF_DIR`, and that works. The client step converts the same variable with `cygpath -m` before using it for `BV2_PREF_DIR` and `--config`. One of the two treatments is unnecessary. On a GitHub Windows runner, `RUNNER_TEMP` is already a Windows path, so the `cygpath` is likely the extra one; I have not confirmed this on a runner. The inconsistency makes a reader wonder which one is load-bearing.
   - **Fix:** drop the `cygpath` and the `tmp` variable, and use `$RUNNER_TEMP` directly as the server step does. If it turns out to be needed, apply it in both steps and say why.

6. **Low: the step record's diff range is out of date** (`docs/roadmap/phase-t-testing-environment/reviews/step3.md:4`).
   - **Problem:** it says `baf8ac7..c48e65c`, but the step now spans `5034107..59f81bd`, after the merge of `main` and the round-1 fix commits 0dcafa3 and 4c5f8ef.
   - **Fix:** update the Diff line when this round is added. REVIEW §5.5 requires the last round to cover every commit.

Verdict: not approved yet. Findings 3 and 4 are evidence and process gaps that should be closed before DONE. Finding 1 needs an owner decision, either widening the Allowed paths or deferring to PNS. Findings 2, 5 and 6 are small fixes inside the Allowed paths.
