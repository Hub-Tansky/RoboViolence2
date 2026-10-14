# Step 3 review

Reviewer: fresh-context subagent, /thermo-nuclear-code-quality-review, two rounds (2026-10-14)
Diff: round 1 f70b068..c308425; round 2 c308425..cb517dc
Skill evidence: both rounds `SKILL: Launching skill: thermo-nuclear-code-quality-review` (raw reports: [step3-report.md](step3-report.md)). The ADR is 0020 and the new PNS entries are PNS-60 and PNS-61: Phase T took 0017–0018 and PNS-56–58 on `main` while this step ran; the raw reports keep the old numbers.

## review.sh and CI

- `tools/review.sh` passes after merging step 2 and `main`.
- CI on 58bd6fa: `build` green on all three OSes, including the Windows step that installs, upgrades (the `master.db` marker survives) and uninstalls (`master.db`, `web.db` stay). The `review` check was red only on step 2's files, now merged.
- macOS (local): fresh `macos-arm64-release` with the overlay triplet links without "built for newer macOS" warnings; `vtool` shows `minos 12.0`; the `.dmg` mounts, holds `licenses/` and an `Applications` link, and the game starts from it.

## Findings (round 1)

| # | Finding | Outcome |
|---|---|---|
| 1 | OS branches scattered through `main()` | fixed in cb517dc (`PLATFORMS` table), simplified in round 2 |
| 2 | Installer never executed; licences unchecked | fixed: CI installs, upgrades, uninstalls; `package-check` checks the licences (made strict in round 2) |
| 3 | Upgrade overwrites `master.db` | fixed: `onlyifdoesntexist uninsneveruninstall`, tested in CI |
| 4 | All-users install option | fixed (removed) |
| 5 | Tag name expanded in `run:` | fixed (`env: VERSION`) |
| 6 | `0.0.0-dev` in two places | fixed (version argument required) |
| 7 | `v` prefix in the installer version | fixed (`removeprefix`) |
| 8 | GPL accept gate | fixed (`InfoBeforeFile`) |
| 9 | No glad notice | fixed (`licenses/glad.txt`); its source file deferred: PNS-61 |
| 10 | vcpkg cache key ignores the triplet | deferred: PNS-60 (`.github/actions/` outside Allowed paths) |
| 11 | Servers can't run from the read-only `.dmg` | fixed (macOS guide) |
| 12 | Inno Setup on the runner unverified | verified: runner image readme lists InnoSetup 6.7.1; CI built and ran the installer |
| 13 | Sentinel licence unexplained | fixed (comment) |
| 14 | Tasks 3.1–3.3 in one commit | rejected: all in `make-package.py`; noted here, as in steps 1 and 4 |

## Findings (round 2)

| # | Finding | Outcome |
|---|---|---|
| 1 | `&&` chains enforce only their last `test` under `bash -e` | fixed in 83327d9 (`ls` over all files) |
| 2 | Database preservation untested | fixed in 83327d9 (marker survives upgrade; files survive uninstall) |
| 3 | ADR and guide miss the new decisions | fixed in 83327d9 |
| 4 | Generated glad notice can drift | deferred: PNS-61 |
| 5 | Closure factory for the client | fixed (plain functions, `Platform` NamedTuple) |
| 6 | Output lists stale files | fixed (`ship_*` return what they wrote) |
| 7 | Unused `version` in two `ship_*` | rejected: uniform table signature, as the reviewer allows |
| 8 | `//` switches rely on MSYS conversion | fixed (`MSYS_NO_PATHCONV`, `cygpath -m`) |
| 9 | Installer test never ran | done: passed on 58bd6fa |

## Not done / unsure

- The release job runs only on a `v*` tag; it has not run. The owner creates the first tag.
- The Windows installer was exercised by CI (silent mode), not by a person on Windows 10.
