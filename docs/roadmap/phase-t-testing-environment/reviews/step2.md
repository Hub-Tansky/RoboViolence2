# Step 2 review

Reviewer: fresh-context subagent, /thermo-nuclear-code-quality-review, 2026-10-14 (3 rounds); subagent thermo-nuclear-code-quality-review, 2026-10-18 (round 4)
Diff: 015abbe..632a1f0 (stacked on step 1; plus this record, the raw report and their `ARCHITECTURE.md` rows). Round 4: 3d1c872..e47179b (task 2.6, commit 8ce1ff0, reviewed after it was merged as PR #70)

## review.sh

`tools/review.sh --base refactor/phase-t-step1-mac-pull-agent --full macos-arm64` on 6c795ed: all PASS. Rerun on the final head (632a1f0) with this record and the report staged: architecture, hygiene, content-case, original-assets, secrets, diff-rules, configure, build, ctest all PASS.

Round 4 (`tools/review.sh --step docs/roadmap/phase-t-testing-environment/step2-staging-deploy.md`, cloud container): architecture, hygiene, content-case, original-assets, diff-rules PASS; secrets SKIP (no gitleaks; CI `secret-scan` runs it). `--full` not run: no vcpkg there, and the round changes only `tools/staging/` and docs, which no build target or ctest reads. CI `build`, `smoke` (ctest) and `package-check` passed on 8ce1ff0.

## Findings

Raw report: [step2-report.md](step2-report.md).

| # | Finding | Outcome |
|---|---|---|
| 1 | Admin port and public certificate documented but not configurable | fixed: `BV2_ADMIN_PORT` in compose |
| 2 | Unreadable `/status` treated as an empty server (blocker) | fixed: decided by the running container; unreadable `/status` blocks the deploy as an infrastructure failure |
| 3 | Waiting notice reset by each new commit | fixed: `waiting_update` keeps the start time |
| 4 | `/deployed` lost on rollback and proxy recreation | fixed: bind-mounted `releases/<sha7>/srv/deployed` |
| 5 | Pre-step commits deploy `:latest` under another name | fixed: refused at `fetch`; ADR 0018 updated |
| 6 | Root-owned state dir; unwritable dir looked like a held lock | fixed: README `install -d -o`; hard error in `lock` |
| 7 | Run logic untested | fixed: `waiting_update`, `prune_releases`, `wait-empty` tests |
| 8 | Weekly rebuild doesn't reach staging | accepted: documented in ADR 0018 and the script |
| 9 | Wrong test comment | fixed |
| 10 | Old `--sha` not found | fixed: `head_sha` query for a full SHA |
| 11 | Env-file failure can't notify | accepted: logged only, documented |
| 12 | Stale docs | fixed |
| 13 | Images never pruned | fixed: `docker rmi` of pruned releases |
| 14–19 | Port docs; certificate window; prune while waiting; CI `sudo`; test name; short `--sha` | fixed in 632a1f0 |
| R4-1 | Host iptables INPUT rules are dead: Docker forwards published ports via PREROUTING/FORWARD | fixed in 136fd70: rules removed; header says to open the provider's firewall |
| R4-2 | `cloud-init.yml` names the hosting provider | fixed in 136fd70 (the 8ce1ff0 commit message keeps the name; history is not rewritten) |
| R4-3 | Host bootstrap outside the step's tasks, merged before review | confirmed with owner: added as task 2.6 (owner asked for it on 2026-10-18); this round covers the merged commit |
| R4-4 | `rv2-staging-setup` has no test or recorded run | fixed: live run recorded below; static schema check deferred → PNS-71 |
| R4-5 | README env template lacks `BV2_PUBLIC_ADDRESS` | fixed in 136fd70 |
| R4-6 | Re-run of `rv2-staging-setup` silently keeps the old env file | fixed in 136fd70: prints that the file is kept |

Round-1 fixes: 7ce4279 (1, 4, 12), 08ed565 (2, 3, 5, 6, 10, 13), a529e8b (7, 9), 90658df (5, 6, 12 docs). Round 3 verdict: approved, no new findings. Round 4 (task 2.6) fixes: 136fd70.

## Fix proof

None required: every fix is to code added in this step, not to a defect in merged code. New behaviour is covered by `tests/test_staging_deploy.sh` (ctest `staging_deploy`), the CI `/deployed` check, and a local run on the owner's Mac (Docker Desktop, round-1 code, amd64 images under emulation): deploy of `f1ec80b` healthy; repeat run no-op; `67fd59f` waited while a client was connected and deployed after it left; arm64 pull of amd64-only images failed at `pull` as infrastructure. After the fixes: a pre-step commit is refused at `fetch` without touching Docker.

Task 2.6, live on the staging host (2026-10-18, arm64 VM, Ubuntu 24.04): cloud-init on first boot installed Docker, compose 2.40 and `deploy.sh`; the env file was written by hand with the script's own `printf` line (mode 600, accepted by `load_env`), then `rv2-staging-setup` kept it, installed cron and ran the first deploy; its env-writing branch itself was not exercised. The first deploy of `3d1c872` came up healthy with `/deployed` matching. The round-4 edits (136fd70) were not re-run on a fresh VM.

## Not done / unsure

- After merge (2026-10-14): `containers` on `907d94b` pushed both architectures and the multi-arch manifest (`imagetools inspect`: linux/amd64, linux/arm64 for server and master). On the owner's Mac (native arm64, `deploy.sh` from `main`): deploy `907d94b`, update to `d35743d` (`/deployed` matched each time), manual rollback `--sha 907d94b`, and with the env-file password changed under a running server the run stopped at `wait-empty` (infra, no attempt, no deploy) and deployed after the password was restored. Stack torn down afterwards.
- Owner checks on the staging host (2026-10-18): the Mac agent's `staging-connect` passed against `3d1c872` over the public certificate on 443; `e47179b`, merged 14:58Z, was live on staging at 15:10Z with nobody connected (target: 30 minutes).
- Automatic rollback after a failed `up` or verify was not exercised live (manual rollback was).
