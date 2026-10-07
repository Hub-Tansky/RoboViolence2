# Step 1 review

Reviewer: fresh-context subagent, /thermo-nuclear-code-quality-review, two rounds (2026-10-07, 2026-10-08)
Diff: round 1 b01f7a3..a350528; round 2 b01f7a3..4ef26cd (adds the round 1 fixes and the owner-requested guide changes)
Skill evidence: round 1, none recorded (the reviewer's report didn't quote the skill result, and its session can no longer be asked). Round 2: `SKILL: Launching skill: thermo-nuclear-code-quality-review` (the tool's result, quoted by the reviewer).

## review.sh

Before the fixes, `tools/review.sh --full macos-arm64` at a350528: architecture, hygiene, content-case, original-assets, secrets, diff-rules, configure, build, ctest all PASS.

After round 2 fixes, `tools/review.sh --full macos-arm64` at 1563ba7: architecture, hygiene, content-case, original-assets, secrets, diff-rules, configure, build, ctest all PASS.

## Findings (round 1)

| # | Finding | Outcome |
|---|---|---|
| 1 | `docs/build/linux.md` and `tools/setup-dev.sh` say both servers link GL/GLU; `bv2master` links neither | fixed in f38b2fb, 09ceb25 |
| 2 | `run-server.sh` passes only `$1`, `run-server.cmd` passes all; `--config` breaks on Unix | fixed in 71c7821 |
| 3 | `package-check` drops the server's exit code (no pipefail), greps a banner printed before init, only checks the master is alive | fixed in 09ceb25: `shell: bash`, `set -euo pipefail`, grep "Server Created on port 3333", TCP connect to 10207 |
| 4 | macOS package has two `main/` copies; undocumented which program reads which | confirmed with owner: keep both, document; fixed in f38b2fb (macos.md) |
| 5 | `windows.md` runs `tools\setup-dev.ps1`, blocked by the default execution policy | fixed in f38b2fb |
| 6 | `.cmd` instead of `.ps1` and the package format have no ADR; task 1.2 not amended | task 1.2 amended in 54a6625; ADR deferred → PNS-23 (owner: check further first) |
| 7 | Nothing checks Windows/macOS package contents; script glob can silently ship none | fixed in 3c86354: explicit list, fails if anything is missing |
| 8 | Guides don't give the pref dir paths (task 1.3) | fixed in f38b2fb (README table) |
| 9 | Replacing `Resources/main` in a packaged bundle breaks the signature | fixed in f38b2fb (re-sign step) |
| 10 | AGENTS.md drops `bv2.db`, which the build still generates but the game never reads | doc fixed in f38b2fb; removal deferred → PNS-24 |
| 11 | Three archive branches with two external tools | fixed in 3c86354: `ditto` on macOS, `shutil.make_archive` (zip / gztar) otherwise |
| 12 | Package-only scripts in `tools/` fail when run from the repo | fixed in b279bec: moved to `packaging/scripts/` |
| 13 | `package-check` waits for the whole build matrix | rejected: it is a separate, non-required job; isolation from the build leg is worth the wait |
| 14 | Runtime package list duplicated in `linux.md` without a check; CI `sed`/`eval` brittle | fixed in 09ceb25: script prints names only; CI installs them and fails if `linux.md` doesn't list them |
| 15 | Download instructions omit GitHub sign-in and artifact expiry | fixed in f38b2fb |
| 16 | `.gitignore` changed (c38f6e3) before the approval was written into Allowed paths (a350528); `/build/` no longer ignores nested `build/` dirs | confirmed with owner: approved 2026-10-07 before the change, written down late; CMake builds only into the root `build/` |

## Findings (round 2)

| # | Finding | Outcome |
|---|---|---|
| 1 | Status set to DONE before the post-review commits (e8d9d7a, 857e42e, 4ef26cd) were reviewed or run through `--full` | fixed in 6bef875 (back to IN PROGRESS); this round reviews them; `--full` rerun below |
| 2 | Record claims the skill without evidence | fixed in this record (skill evidence line); REVIEW.md/`review.sh` evidence rule follows in a separate PR (owner decision) |
| 3 | Two acceptance checks unproven: the owner ran the package, not the source build in `macos.md`; no link-check tool exists | confirmed with owner: the package run is accepted (acceptance check amended); manual link check recorded under Not done / unsure |
| 4 | Step 2 task 2.4 touches `game/src/Client.cpp` for a non-blocking request without recorded approval | fixed in 6bef875 (approval next to the path) |
| 5 | Task 2.4 wording: folder creation already exists; manual-only proof; HiDPI framebuffer size | fixed in 6bef875 |
| 6 | PNS-25 says colour depth is ignored; `dkw.cpp:352–355` uses it as minimum sizes | fixed in 6bef875 |
| 7 | PNS-18 lists 2 of 4 `glPolygonMode` calls in `Player.cpp` | fixed in 6bef875 |
| 8 | `run-server --config x.cfg` starts no game (default only with no arguments) | fixed in 504797d (documented in the README table) |
| 9 | Guides start `run-master`, which does nothing without `BV2_MASTER_SERVERS` | fixed in 504797d (optional, for the server browser) |
| 10 | "Data required" conflicts with building and tests working without it; `ls …/main/maps` fails for `BV2_DATA_DIR=…/main` | fixed in 504797d ("required to play"; note for `main/` itself) |
| 11 | Fix proof cites a CI run without an id | fixed in this record |

## Fix proof

These fixes change scripts, CI and docs, not game code, so the proof is the check that now fails on the old behaviour:

- 7: with `packaging/scripts/run-master.sh` removed, `make-package.py` fails (`FileNotFoundError … run-master.sh`) instead of shipping a package without it.
- 3, 14: verified by the `package-check` job of Actions run 37609568413 (https://github.com/Hub-Tansky/RoboViolence2/actions/runs/37609568413); the job fails if the server doesn't reach "Server Created", the master doesn't listen, or `linux.md` lacks the package line.
- 2: `run-server.sh --config x.cfg CTF` now reaches `bv2dedicated` unchanged (`exec ./bv2dedicated "$@"`).

## Owner acceptance (2026-10-08)

- The owner ran the macOS package from this PR's CI with original data (`BV2_DATA_DIR` in `~/.zshrc`): server, master and client started, a CTF game played, smoothly.
- Owner request after the run, applied after this review: original data is a required first step in every guide (`docs/build/*.md`). Docs only; no code changed.
- Play-test findings moved on: screenshot/stats keys → step 2 task 2.4; colour depth → PNS-25; `nbVertex` print → PNS-19; GL software fallback → PNS-18.

## Not done / unsure

- The owner ran the macOS **package**; the **build-from-source** path of `docs/build/macos.md` hasn't been followed by a person. The owner accepted the package run for the acceptance check (2026-10-08).
- Links in `docs/build/` were checked by hand (both reviewers); there is no link-check tool.
- The Windows package and guide have never been run on Windows (step 2).
- The Linux package has only run in CI's clean container, not on a desktop (step 2).
