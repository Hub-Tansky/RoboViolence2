# Step 3 review (Phase T, CI package smoke): round 1

SKILL: NOT LOADED – Skill tool refused: "Skill thermo-nuclear-code-quality-review cannot be used with Skill tool due to disable-model-invocation. Ask the user to run /thermo-nuclear-code-quality-review themselves — it cannot be invoked via the Skill tool. Do not replicate this skill's workflow by other means — it is reserved for explicit user invocation." Because of that instruction I did not read or apply the SKILL.md file. What follows is a plain review against REVIEW.md section 2 and the step file.

**Impact:** the DONE gate in REVIEW.md §1 and §5 needs a report with the skill's load line and no `SKILL: NOT LOADED`. This report can't close the step. The owner has to run `/thermo-nuclear-code-quality-review` themselves (REVIEW §5.2: "If the skill isn't available, stop and tell the owner").

Diff: `baf8ac7..c48e65c` (6 commits; `build.yml` +70 lines, AGENTS.md, ARCHITECTURE.md, phase README, step file).

## Checks that hold

- The changed files are all inside the Allowed paths.
- No anti-gaming patterns (`continue-on-error`, `|| true` around a check).
- The start signal is sound. The `Console` constructor truncates `console.log` (`game/src/Console.cpp:51-53`). The client builds the console only at `game/src/main.cpp:558`, after window, input, GL, audio and network init. A separate pref dir keeps the server's own `console.log` (`main.cpp:350`) apart.
- The Mesa download is pinned to a version and checked with SHA-256 (`sha256sum -c`). Workflow permissions are `contents: read`.
- ARCHITECTURE.md has a row for the new step file and updates the `build.yml` row. AGENTS.md lists the job in the CI section. The phase README has the step row.
- No mechanical rewrite is mixed with functional changes.
- The job itself is the test for this new behaviour (a CI job), and it passed on c48e65c on both OSes.

## Findings

1. **Medium: Mesa llvmpipe is a new CI dependency with no ADR and no refresh policy** (`.github/workflows/build.yml:161-170`).
   - Problem: CI downloads a third-party prebuilt binary (mesa-dist-win 26.0.3). REVIEW §3 counts any decision that changes a dependency as non-obvious. The choice has a rejected alternative (no client test on Windows, or WARP / another software GL) and a later agent might reverse it. Only one row in the plan records it (`docs/roadmap/20261014_testing_environment_plan.md:47`). Nothing says who bumps `MESA` or when, and `osv-vcpkg` does not cover it.
   - The hash comment says "first download, 2026-10-17". So the hash was taken on first use. It pins the file but does not prove it is authentic, which falls short of the plan's "documented source and hash" (plan:169).
   - Fix: write a short ADR covering the source, how the hash was obtained, the bump policy (for example with the monthly vcpkg baseline bump) and the rejected alternatives. Or, if the owner accepts the plan row as the record, add a PNS entry for the bump policy.

2. **Low: the job name doesn't match the step file, and it will change when the job becomes required** (`build.yml:135`; step file "Acceptance checks").
   - Problem: the job is named `package-smoke (${{ matrix.preset }}, not required)`. The acceptance checks name `package-smoke (win-x64-msvc)` and `package-smoke (macos-arm64)`. Once the owner makes the check required, the name has to change, which renames the status check.
   - Fix: drop `, not required` from `name:` (the comment on line 133 already says it), or update the acceptance-check names to match.

3. **Low: a debug leftover and a filename glob with no comment** (`build.yml:150-152`).
   - Problem: `ls -l` in the Extract step was added while hunting for the downloaded file name (commits 9dd2973, b599caa). `zip=$(ls roboviolence2-…zip*)` matches by prefix without saying why. If more than one file matches, `$zip` holds several names and extraction fails in a confusing way.
   - Fix: remove `ls -l`, and add a one-line comment naming the real downloaded file name (or use it directly).

4. **Low: a failed client run leaves no diagnostics** (`build.yml:189-203`).
   - Problem: the client's stdout and stderr are not captured. On failure, `ls -l console.log` gives only "No such file". The server step does print `server.log`.
   - Fix: redirect the client's output to `client.log`, and on failure print it along with `console.log` if it exists. For example, end with `cat "$BV2_PREF_DIR/console.log"` in place of `ls -l`, and `cat client.log` in the early-exit branch.

5. **Low: on Windows the job starts `bv2.exe` directly, and nothing says why** (`build.yml:195`).
   - Problem: on Windows the packaged `run-client.cmd` is never run. It uses `start ""`, which detaches, so the pid can't be tracked. That reason is not written down, so a later agent may "fix" the job to use the script, or assume the script is tested.
   - Fix: add a one-line comment, `# run-client.cmd detaches (start ""); its pid can't be tracked`.

6. **Info: task 3.2 has four commits, and a step3.3 commit sits between them** (2f74571, 9dd2973, b599caa, c48e65c).
   - Problem: AGENTS.md asks for "one commit per task item". The follow-up commits fix the job itself, so they are not review fixes named `stepN.M: review:`.
   - Fix: squash them into 2f74571 before merge, or accept as is. This is not a repeat of step 1 finding 22, which was about published plan-PR history.

None of these findings repeats one from the step 1 or step 2 records.

## Not done / unsure

- The skill was not loaded (see the SKILL line). REVIEW's DONE gate rejects this report, so the owner has to run the fresh-context review with `/thermo-nuclear-code-quality-review` themselves.
- I did not open the CI logs. The results on c48e65c (both package-smoke jobs green, `console.log` written) come from the caller.
- I did not run `tools/review.sh`.
