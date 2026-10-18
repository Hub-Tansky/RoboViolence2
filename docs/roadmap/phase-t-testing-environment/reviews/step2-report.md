SKILL: Launching skill: thermo-nuclear-code-quality-review

# Step 2 review (Phase T, staging deploy): round 1

Diff: 015abbe..6c795ed (5 commits, step2.1 to step2.5). Read: the full diff, `tools/staging/deploy.sh`, `tools/staging/README.md`, `packaging/docker/{compose.example.yml,Caddyfile}`, `.github/workflows/containers.yml`, `docs/ops/runbook.md`, `game/src/AdminApi.cpp:110-123`, and the step 1 records (`reviews/step1.md` and `reviews/step1-report.md`).

Checks I ran:
- `bash tests/test_staging_deploy.sh <scratch>`: all passed.
- `shellcheck`: only SC2012 (`ls -t` used for pruning) and SC1091. Both are info level.
- Scratch experiment: assigning a readonly variable inside a sourced file **does not** abort bash. It prints an error and continues (see finding 9).

## Structure (skill)

- The diff is small and flat. `deploy.sh` is 179 lines with a clear helper section, then the run section, one `stage` per phase, and one `finish` doing record, rollback and notify. No file goes near 1k lines.
- `deploy.sh` copies about 8 helpers from `tools/playtest/agent.sh`: `pick_run`, `failures_of`, `is_infra`, `file_mode`, `load_env`, `lock`, `notify`, `wait_for`. A shared lib was already rejected in step 1 (#23: an extra install step for a curl-installed script), so this is not a new finding. It is a known cost: a fix to one copy (e.g. step 1's lock fixes #24, #32, #39) must be ported by hand to the other.
- Code-judo opportunity, see finding 4: serve `/deployed` from a bind-mounted file per release instead of `docker compose cp` into the container. That deletes the `cp` step, the "not published" warning branch and the CI `cp`. It also fixes rollback and container recreation for free.

## Findings

1. **should-fix**: the staging admin port and public certificate are documented but can't be configured.
   - Where: `packaging/docker/compose.example.yml:38`, `tools/staging/README.md` (Needs and env file), `tools/staging/deploy.sh:70`, `tools/playtest/agent.sh:347,351`.
   - Problem: compose hard-codes `ports: ["8443:443/tcp"]`. The runbook (`docs/ops/runbook.md:55`) says a public name needs you to "publish 443 and 80 instead of 8443", i.e. edit the compose file. `deploy.sh` re-fetches the compose file from GitHub for every commit, so a staging host can never make that edit.
   - Failure scenario: the staging README offers `BV2_ADMIN_HOST=<public name>` and `RV2_STAGING_ADMIN_PORT` (443). Following it, Caddy tries ACME with only 8443 published, gets no certificate and fails every TLS handshake. `/status` and `/deployed` never answer. The new Mac-agent `RV2_STAGING_ADMIN_PORT=443` points at a port nothing publishes.
   - Proposed change, either:
     - (a) parametrise compose: `"${BV2_ADMIN_PORT:-8443}:443"`, plus an optional `80` mapping or a documented override file under `$ROOT` passed as a second `-f`; or
     - (b) drop the 443/public-name option from the staging README and from the Mac agent (keep 8443 + `-k`), and leave public certificates to a later step.

2. **blocker**: when `/status` doesn't answer, the script deploys even if people are playing.
   - Where: `tools/staging/deploy.sh:153` (`if [ -z "$FORCE" ] && status=$(admin_status 2>/dev/null); then … # no answer: nothing running`).
   - Problem: any `admin_status` failure counts as "nothing running", and the deploy goes ahead. The step's core rule is "never during a game".
   - Concrete triggers:
     - (a) The owner rotates `BV2_ADMIN_PASS` in `/etc/rv2-staging.env`. The running server still has the old value, so `curl -f` gets 401 and staging is redeployed mid-game.
     - (b) The documented public-name setup from finding 1: TLS always fails, so every new `main` commit deploys over a live game. Verify then fails and the script rolls back, three times per commit, restarting the server each time.
     - (c) Caddy is down or restarting while the game server runs.
   - Proposed change: decide "nothing to protect" from the container, not from the admin API. For example, `docker ps -q -f label=com.docker.compose.project=roboviolence2 -f label=com.docker.compose.service=server -f status=running` is empty. If the server runs and `/status` fails, wait (or fail as infra and notify once), unless `--force`.

3. **should-fix**: the 6-hour "waiting" notice restarts whenever a new commit arrives.
   - Where: `tools/staging/deploy.sh:156-163`.
   - Problem: the `waiting` file is keyed by SHA, so each new `main` commit resets `since`.
   - Failure scenario: a ghost or long-idle connection keeps `players 1` for days while commits land more often than every 6 h. The owner is never told, and staging silently stays on an old build. That is the case the notice exists for.
   - Proposed change: keep `since` when the waiting SHA changes and update only the SHA. Or store only "waiting since" and clear it on pass or on empty. This also removes the `wsha` comparison.

4. **should-fix**: `/deployed` is lost on rollback and whenever the proxy container is recreated.
   - Where: `tools/staging/deploy.sh:105-107,174`; `.github/workflows/containers.yml:90-97`.
   - Problem: `/srv/deployed` lives only in the container's writable layer. Each release's compose mounts `./Caddyfile` from a different directory (`releases/<sha7>/`), so `admin-proxy` is recreated on every `up`, including the rollback in `finish`.
   - Failure scenario: after a rollback, `$ROOT/deployed` still names PREV but `/deployed` returns 404. The same happens after any manual `compose up` that recreates Caddy, or a Caddy image bump. The Mac agent then reports "unknown build" and can't detect a version mismatch.
   - Proposed change (code judo): mount a directory, e.g. `./srv:/srv:ro`, in the compose file. For non-staging users Docker creates an empty directory, so they get a 404 as today. `deploy.sh` writes `releases/<sha7>/srv/deployed` after verify. Rollback to PREV then serves PREV's own file automatically. This deletes the `compose cp` line, its warning branch and the CI `cp`; the CI check becomes "write the file into `packaging/docker/srv/`".

5. **should-fix**: a commit from before this step deploys `:latest` while `deployed` says the old SHA.
   - Where: `tools/staging/deploy.sh:142-150,173`; `docs/decisions/0018-staging-pull-deploy.md` (Consequences).
   - Problem: the ADR accepts that "deploying one runs `:latest`". But `deploy.sh` still pulls `:<old sha>` and then publishes `<old sha>` in `deployed`, while the stack actually runs `:latest`.
   - Failure scenario: a manual rollback with `--sha` to a commit before this step reports a build that isn't running. On an arm64 host the pull fails anyway (those commits have amd64-only tags), so the failure surfaces as an infra failure.
   - Proposed change: after fetch, `grep -q 'BV2_IMAGE_TAG' "$rel/compose.example.yml" || fail "commit predates BV2_IMAGE_TAG"`. Fix the ADR consequence to say such commits are refused.

6. **should-fix**: the README install steps leave the state directory owned by root.
   - Where: `tools/staging/README.md` (Install).
   - Problem: `sudo mkdir -p /var/lib/rv2-staging` makes the directory root-owned, while the README says to run as root *or a `docker` group member*.
   - Failure scenario: for a non-root user, the cron redirect `>>/var/lib/rv2-staging/deploy.log` fails, so the job never runs and nothing is logged. Run by hand, `lock` fails in `chmod 700` and in `set -C` writing the lock file. The script then prints "locked by pid ; skipped" and exits 0 forever (`deploy.sh:84-87`).
   - Proposed change:
     - README: `sudo install -d -o <user> -m 700 /var/lib/rv2-staging`.
     - `lock`: treat a failed `mkdir`/`chmod` as a hard error, not "locked".

7. **should-fix (tests; REVIEW §2 "new behaviour has a test"; repeats the step 1 #9 pattern)**: the run logic is untested.
   - Where: `tests/test_staging_deploy.sh`.
   - Problem: the tests cover the pure helpers only. Untested: the waiting/notify state machine (`deploy.sh:156-164`), the once-per-stage infra marker (`finish`, 95-102), the rollback decision (103-110) and release pruning (179). These are where findings 3 and 4 live.
   - Proposed change: extract `waiting_update <sha> <now>` (returns notify yes/no) and `prune_releases <keep-sha>` as helpers and test them, as step 1 did with `record_state` and `prune`.

8. **nit**: the weekly rebuild re-tags `:<sha>`, and staging never picks it up.
   - Where: `.github/workflows/containers.yml:105-107,159-165`.
   - Problem: the weekly `schedule` run rebuilds `main`'s head and overwrites the same `:<sha>`, `:<sha>-<arch>` and `:latest` tags. Commit tags are therefore mutable. `deploy.sh` exits when `SHA == PREV` (`deploy.sh:139`), so staging keeps the old digest. The "base-image security fixes land without a code change" purpose (workflow header) never reaches staging.
   - Proposed change, either:
     - accept it and say so in ADR 0018; or
     - have `deploy.sh` compare the pulled image ID to the running one when `SHA == PREV` (pull happens later today; it would have to move before that check).

9. **nit**: a test comment wrongly claims the shell aborts.
   - Where: `tests/test_staging_deploy.sh:52`.
   - Problem: the comment says assigning a readonly variable "aborts the sourcing shell: the run stops". In bash (non-POSIX mode) it doesn't: an error is printed and sourcing continues with `REPO` unchanged (verified in scratch). The assertion still holds, but the stated behaviour is wrong.
   - Proposed change: comment "a readonly variable keeps its value; the line only prints an error".

10. **nit**: an old `--sha` can't be found.
    - Where: `tools/staging/deploy.sh:134`.
    - Problem: the runs query uses `per_page=30`, so a `--sha` older than the last 30 successful `main` runs fails with "no successful main push run". That is a manual rollback failing exactly when it is needed.
    - Proposed change: when `WANT_SHA` is a full SHA, add `&head_sha=$WANT_SHA` to the query (the API supports it).

11. **nit**: an infra failure at the `env` stage can't notify.
    - Where: `tools/staging/deploy.sh:129-131` with `finish`.
    - Problem: if `load_env` fails, `NTFY_TOPIC` was never loaded, so the "notify once" for that infra failure goes only to the log.
    - Proposed change: none needed beyond documenting it, or log-only by design.

12. **nit**: stale docs.
    - `containers.yml:20`: the comment "amd64 is published; arm64 checks the build" is now wrong.
    - `docs/ops/runbook.md:12`: "CI pushes `latest` and the commit SHA" should mention multi-arch (the new runbook row does).
    - `ARCHITECTURE.md`: the ADR inventory lists 0018 before 0017.

13. **nit**: Docker images are never pruned on the staging host.
    - Where: `tools/staging/deploy.sh:179`.
    - Problem: releases are pruned to the last 5, but the pulled images are never removed. Every `main` commit adds two tags, so disk use grows without bound.
    - Proposed change: after a pass, `docker image prune -f` plus removing `:<sha>` tags of pruned releases. Or accept it and document it in the README.

## Checked and fine

- **Caddyfile.** `request_body` sorts before `handle` in Caddy's directive order, so the 4 KB limit still applies to both branches. The catch-all `handle { reverse_proxy server:8080 }` keeps the admin API identical. `handle /deployed` is an exact-path match, `root * /srv` + `file_server` serves only `/srv/deployed`, and non-GET methods get 405.
- **Compose and `BV2_IMAGE_TAG`.**
  - The `${BV2_IMAGE_TAG:-latest}` default keeps manual use unchanged.
  - The shell prefix in `compose()` overrides any value in the env file.
  - `name: roboviolence2` makes every release directory the same project.
  - `./Caddyfile` resolves to the release directory.
- **CI.**
  - Per-runner cache scopes match the images job.
  - `manifest` needs both matrix legs. If either fails, the run fails and is never selected, so a half-published `:<sha>` can't be deployed.
  - Concurrency cancellation also leaves the run unselected.
- **Selection and attempts.**
  - Selection is main-only (fork PR filter, the same as step 1).
  - The attempt limit is bypassed only with `--sha`.
  - Infra stages (env, select, fetch, pull) leave the stack untouched.
  - Rollback is skipped when there is no PREV or PREV equals SHA.
  - An interrupt records nothing.
  - Pruning never removes the current SHA, and the next rollback target is the current SHA.
- **Lock.** The pattern matches `/bin/bash /opt/rv2-staging/deploy.sh`. The stale-lock double-remove race was already rejected in step 1 (#31); not re-raised.
- **Mac agent.** The port change is a correct substitution; its usefulness depends on finding 1.

## REVIEW.md section 2

- **Ground rules:** no game, asset or GUI change.
- **Tests:** helper tests and a CI `/deployed` check exist. The run logic is untested (finding 7). The Mac acceptance runs can't be verified from the diff; the record must show them.
- **Docs:** `ARCHITECTURE.md`, the staging README, the runbook, ADR 0018 and the decisions index are updated. Stale lines are in finding 12, and the ADR consequence needs correcting (finding 5).
- **Commits:** no mechanical rewrite is mixed in, and commit names follow `stepN.M:`.

## Verdict

**Changes requested.**
- Must fix: finding 2 (blocker). The empty-server guard fails open, so the step's central promise "never during a game" breaks on a password rotation or on the documented public-name setup.
- Should fix before DONE: findings 1, 3, 4, 5, 6 and 7. Finding 4's bind-mount approach is the preferred fix: it deletes code and fixes rollback.
- Findings 8 to 13 are nits.

## Round 2

SKILL: Launching skill: thermo-nuclear-code-quality-review

# Step 2 review (Phase T, staging deploy): round 2

Diff: 6c795ed..90658df (4 commits, step2.2 to step2.5, 9 files, +85/-43). Read: the full diff, all of `tools/staging/deploy.sh` (205 lines), `tools/staging/README.md`, `packaging/docker/{Caddyfile,compose.example.yml}`, `.github/workflows/containers.yml` (trigger and `/deployed` step), the runbook port lines, `tools/playtest/agent.sh:347,351`, and the step file.

Checks I ran:
- `bash tests/test_staging_deploy.sh <scratch>`: all passed, including the new `waiting_update`, `prune_releases` and `wait-empty` cases.
- Scratch experiment: a round-1 format `waiting` file (`<sha> <since> <told>`) makes `waiting_update` fail with an arithmetic error ("value too great for base"). No host ran round-1 code (the step file says the host is chosen later), so this is not a finding.

## Structure (skill)

- `deploy.sh` stays flat at 205 lines.
- The round-1 code-judo move was taken: the bind-mounted `srv/deployed` deleted the `compose cp` step, its warning branch and the CI `cp`.
- The two new helpers are small, pure enough and tested.
- `waiting_update` drops the SHA from the state, so the `wsha` comparison is gone. That is a real simplification, not code moved around.
- `server_running` is a one-liner in the right place.
- I see no new spaghetti and no bolted-on special cases.

## Round-1 findings

| # | Status | Note |
|---|---|---|
| 1 | Resolved | `${BV2_ADMIN_PORT:-8443}:443` in compose; `admin_status` uses the same variable; README updated. Stale port text remains in other docs (finding 14). |
| 2 | Resolved | `server_running` uses compose labels with `status=running`. A running server whose `/status` fails now fails `wait-empty` as infra: no deploy, notify once, `--force` to override. |
| 3 | Resolved | `waiting` holds only `<since> <told>`. It is kept across commits and cleared only when the newest SHA is already running or the deploy proceeds. |
| 4 | Resolved | `./srv:/srv:ro` plus `releases/<sha7>/srv/deployed`; rollback serves PREV's own file. |
| 5 | Resolved | grep for `BV2_IMAGE_TAG` after fetch; ADR consequence corrected. |
| 6 | Resolved | `install -d -o "$USER" -m 700`; `lock` exits 1 when `$ROOT` isn't writable. |
| 7 | Resolved | `waiting_update` and `prune_releases` extracted and tested. The infra marker and rollback decision in `finish` are still untested; acceptable, since the requested extraction was done. |
| 8 | Accepted | Documented in ADR 0018 and at `deploy.sh:165`. |
| 9 | Resolved | |
| 10 | Resolved | Uses `head_sha` for a full SHA. A short SHA older than 30 runs still fails (finding 19). |
| 11 | Accepted (log-only) | Commented at `deploy.sh:154` and in the README. |
| 12 | Resolved | `containers.yml` comment, runbook multi-arch line and ADR inventory order all fixed. |
| 13 | Resolved | `docker rmi` of pruned releases' tags. One gap remains: nothing is pruned while deploys wait (finding 16). |

## Checks on the new code

- **`server_running`.**
  - Multiple `-f` filters with different keys are ANDed, and `PROJECT` matches `name:` in compose.
  - A restarting or crash-looping server counts as "not running", so a deploy proceeds. That is fine: no one can be playing on it.
  - If `docker ps` fails because the daemon is down, it reads as "not running". But `pull` already failed as infra one stage earlier, so this isn't reachable in practice.
- **`wait-empty` as infra.**
  - A failure notifies once and records no attempt, and `SHA` is not written to `state.tsv`. Correct.
  - The `--max-time 5` curl timing out under load now blocks the deploy. That is the safe direction.
  - A password rotation blocks deploys until `--force`. This is documented in the README and is the intended trade.
- **`waiting_update`.**
  - Its semantics are correct: notify once after 6 h of continuous "players present" across commits; reset when a deploy proceeds or nothing is pending.
- **`prune_releases` and `docker rmi`.** Neither the running stack nor the rollback target can lose its image:
  - Pruning runs only after a pass, with `keep = SHA`, which is the running stack and the next PREV.
  - The previous PREV's dir may be pruned, but its containers were just replaced, and rollback after a *failed* deploy never reaches the prune.
  - `docker rmi` removes a tag, not a shared image ID, and without `-f` it refuses an image a container still uses.
  - Directory mtime doesn't change on re-fetch, but the explicit `keep` argument covers the `--sha` case.
- **`./srv` bind mount.**
  - Compose resolves `./srv` against the release directory. `deploy.sh` creates `$rel/srv` before `up`, so on staging it is owned by the deploy user, and Caddy reads it through the mount (the 700 on `$ROOT` doesn't matter to the root daemon).
  - For non-staging Linux users, Docker creates an empty root-owned `packaging/docker/srv/` in the checkout. Git ignores it and the owner can `rmdir` it, so it is harmless.
  - CI's `sudo tee` works because that directory is root-owned on the runner. It is simpler to avoid needing it (finding 17).
- **`BV2_ADMIN_PORT` and the TLS-ALPN claim.**
  - Compose interpolates the variable from `--env-file`.
  - With only 443 published, HTTP-01 fails and Caddy completes TLS-ALPN-01 on 443, so the runbook claim is correct.
  - Other docs still say 80 and 443 are needed (finding 14).
  - The Mac agent keeps its own `RV2_STAGING_ADMIN_PORT`. That is correct: it is a different host's env file.
- **Refusing commits without `BV2_IMAGE_TAG`.** The first deploy after the merge is possible:
  - `containers.yml` runs on every push. The merge commit on `main` contains the new compose file, and its run publishes multi-arch `:<sha>`.
  - Until that run succeeds, the newest successful run is a pre-PR commit. It is refused at `fetch`, which is infra (notify once, retried every 10 min), and the first post-merge success then deploys.
  - The refused commit leaves an empty release dir, which `prune_releases` removes later without printing a SHA. Fine.
  - PREV is always a post-merge commit, so rollback is never refused.
- **`head_sha` query.**
  - `[ ${#WANT_SHA} = 40 ]` is correct, and the API supports `head_sha` together with `branch`, `event` and `status`.
  - `pick_run`'s `startswith` still filters.

## New findings

14. **nit**: stale port and ACME text contradicts the new runbook line.
    - Where:
      - `packaging/docker/Caddyfile:2`: "ACME needs ports 80 and 443 reachable"
      - `docs/ops/runbook.md:48`: "or 443 and 80 for a public certificate"
      - `docs/ops/runbook.md:52`: "terminates TLS on port 8443"
    - Problem: `runbook.md:55` and the staging README now say `BV2_ADMIN_PORT=443` alone is enough (TLS-ALPN). The three lines above disagree.
    - Proposed change:
      - Caddyfile: "ACME needs 443 reachable (`BV2_ADMIN_PORT=443`)".
      - Runbook line 48: "TCP `BV2_ADMIN_PORT` (8443; 443 for a public name)".
      - Runbook line 52: "on `BV2_ADMIN_PORT` (default 8443)".

15. **nit**: with a public name, a slow or failed certificate counts as a build failure.
    - Where: `tools/staging/deploy.sh:198`, `finish`.
    - Problem: verify calls `admin_status` with SNI set to the public name. On a host's first deploy (empty `caddy-data`), or when ACME is rate-limited or the DNS isn't propagated, the TLS handshake fails for longer than 60 s. Verify then fails as a build failure, uses up an attempt, and after 3 tries the commit is never retried without `--sha`.
    - Proposed change: document it in the README ("the first deploy with a public name needs a certificate within 60 s; `--sha` retries"). The alternative is a longer verify window when `BV2_ADMIN_HOST` isn't `localhost`. Documenting it is enough.

16. **nit**: release dirs and images accumulate while deploys wait.
    - Where: `tools/staging/deploy.sh:169-189,203`.
    - Problem: `fetch` and `pull` run before `wait-empty`, so every new `main` commit creates a release dir and pulls 2 images even while players hold the deploy. Pruning runs only after a pass. In the 6-hour ghost-player case (the reason the `waiting` notice exists), disk use grows with every commit until someone uses `--force`.
    - Proposed change: run the same prune at the waiting exit as well, with `keep = PREV`, which protects the running stack:
      `for old in $(prune_releases "${PREV:0:7}"); do …rmi…; done`
      Better still, move the prune-and-rmi loop into a helper `prune <keep>` and call it from both places.

17. **nit**: the CI `/deployed` step depends on Docker creating a root-owned directory.
    - Where: `.github/workflows/containers.yml:94`.
    - Problem: `sudo tee` is needed only because Docker created `packaging/docker/srv` as root at `compose up`. That is not the staging path, where the directory exists first and is owned by the deploy user.
    - Proposed change: `mkdir -p packaging/docker/srv` before the step that starts the stack, then a plain `echo "$GITHUB_SHA ci" > packaging/docker/srv/deployed`. That drops `sudo` and matches what `deploy.sh` does.

18. **nit**: a test name describes a case the function can't see.
    - Where: `tests/test_staging_deploy.sh:138`.
    - Problem: `waiting_update` takes no SHA, so "a new commit keeps the start time" tests nothing commit-specific.
    - Proposed change: rename it to "the start time is kept across waits (no SHA in the state)".

19. **nit**: an old build can't be found by a short SHA.
    - Where: `tools/staging/README.md` (`--sha` row), `deploy.sh:160`.
    - Problem: a short `--sha` older than the last 30 successful runs still fails with "no successful main push run".
    - Proposed change: in the README row, say "full SHA for builds older than the last 30 runs".

No repeats of resolved or rejected findings. The step 1 #23 decision (no shared lib) still stands; not re-raised.

## Verdict

**Approved.**
- The round-1 blocker (2) and should-fix findings 1, 3, 4, 5, 6 and 7 are resolved. 8 and 11 are accepted as agreed, and 9, 10, 12 and 13 are resolved.
- The prune and rmi path cannot remove the image of the running stack or of the rollback target.
- The first deploy after merge works once the merge commit's `containers` run succeeds.
- New findings 14 to 19 are all nits. Fixing 14 (contradictory port docs) and 16 (unbounded growth while waiting) before marking the step DONE is recommended but not required.

## Round 3

SKILL: Launching skill: thermo-nuclear-code-quality-review

# Step 2 review (Phase T, staging deploy): round 3

Diff: `HEAD~1..HEAD` (632a1f0, "step2.3: review round 2: prune while waiting; port docs; CI /deployed without sudo; README notes"). It touches 6 files, +18/-11. I read the full diff, all of `tools/staging/deploy.sh` (211 lines), the `containers.yml` compose and `/deployed` steps, the compose `admin-proxy` lines, and the runbook and staging README port lines.

Check I ran: `bash tests/test_staging_deploy.sh <scratch>` passes ("all passed").

## Structure (skill)

- `deploy.sh` grows from 205 to 211 lines and stays flat.
- The new `prune` helper (`deploy.sh:100-105`) replaces the inline loop that sat after a pass. Both exits now call it, so the logic lives in one place rather than being copied. This is the decomposition round 2 asked for.
- No new branching. The waiting exit gains one line.

## Round-2 findings

| # | Status | Note |
|---|---|---|
| 14 | Resolved | `Caddyfile:2`, `runbook.md:48` and `runbook.md:52` now agree with `runbook.md:55` and the README: `BV2_ADMIN_PORT`, default 8443, 443 for a public name, TLS-ALPN on 443. The remaining `localhost:8443` mentions in the runbook (`runbook.md:41,59,60`) are local example commands using the default, so they are not a contradiction. |
| 15 | Resolved (documented) | README line 40 explains the 60 s certificate window and how to retry with `--sha`. |
| 16 | Resolved | `prune "${PREV:0:7}"` runs at the waiting exit (`deploy.sh:196`). |
| 17 | Resolved | `mkdir -p packaging/docker/srv` (`containers.yml:62`) runs before the first `compose up` (`:64`) in the same step. No earlier step starts compose. The `/deployed` step (`:95`) now writes with a plain `>` and no `sudo`. |
| 18 | Resolved | The test was renamed as proposed. |
| 19 | Resolved | The README `--sha` row says to use the full SHA for builds older than the last 30 runs. |

## Checks on the changed lines

**`prune` at the waiting exit, with `keep = ${PREV:0:7}`:**
- **Running stack:** it cannot lose its release dir. `PREV` is what `deployed` says is running, and `prune_releases` skips the dir equal to `keep` (`:53`) whatever its mtime position. That also protects it when more than 5 commits have waited.
- **Images:** they cannot be removed while in use. `docker rmi` without `-f` refuses any image referenced by a container, running or stopped. This holds even if the running stack is not `PREV`, for example after a failed deploy whose rollback also failed.
- **Next rollback target:** while waiting, that is `PREV`, which is kept.
- **The waiting commit (`SHA`):**
  - Its dir is not protected by `keep`, but it is the newest dir in practice: it was created by this or an earlier run for the newest `main` commit, so it is inside the top `KEEP=5` by mtime.
  - The one edge case is `--sha <old commit>` whose dir still exists from long ago, with an old mtime that re-fetching doesn't update. That dir and its image tag get pruned at this exit, and the next run re-fetches and re-pulls it.
  - That costs bandwidth only, never correctness, and a cron run ignores the `--sha` target anyway. It is not worth extra code.
- **`PREV` empty:**
  - This happens when there is no `deployed` file, but `server_running` is true because a stack was started by hand under the same project name.
  - `keep=""` matches no dir, so only dirs beyond the newest 5 are pruned. On a first run there are at most one or two dirs, so nothing happens.
  - The hand-started stack's images are in use, so `rmi` refuses them.
  - `${PREV:0:7}` is safe under `set -u` because `PREV` is initialised at `:73`.
- **Not covered:** the `wait-empty` infra failure (`/status` doesn't answer) exits through `fail`, so it doesn't prune. That state notifies once and needs an operator anyway. I'm noting it here, not raising it as a finding.

**CI `mkdir -p packaging/docker/srv`:**
- It runs before `compose up` and as the runner user, so Docker binds the existing directory instead of creating a root-owned one. The later `echo >` therefore works without `sudo`.
- Caddy reads the bind mount read-only; the runner-default 755/644 permissions are enough.

**Docs:**
- The port, ACME and `--sha` wording is accurate and short.
- The README's "`--sha` retries it" slightly understates things, because cron also retries automatically up to 3 times. It is still correct as the recovery action once those retries are used up. Not a finding.

## New findings

None.

## Verdict

**Approved.** Round-2 findings 14 to 19 are all resolved. The new `prune` helper removes the duplicated logic, and it cannot remove the release dir or images of the running stack or the rollback target. Its behaviour for the waiting commit and for an empty `PREV` is safe. The CI directory is created before `compose up`. There are no new findings, and nothing blocks marking the step DONE.

## Round 4

SKILL: Launching skill: thermo-nuclear-code-quality-review

## Scope

Round 4. I reviewed `git diff 3d1c872..e47179b`, which is one functional commit, 8ce1ff0, merged to `main` as PR #70 from `claude/project-thread-npesoe`. It touches three files and adds 41 lines:
- `/home/claude/RoboViolence2/tools/staging/cloud-init.yml`: new, 36 lines, read in full.
- `/home/claude/RoboViolence2/tools/staging/README.md`: +4 lines, read in full.
- `/home/claude/RoboViolence2/ARCHITECTURE.md`: +1 row.

The new file feeds `deploy.sh` and compose, so I traced those boundaries:
- `tools/staging/deploy.sh`: `load_env` (`:61-66`, mode-600 check, `--env-file` use at `:89`).
- `packaging/docker/compose.example.yml`: `BV2_PUBLIC_ADDRESS` `:27`, `BV2_ADMIN_HOST` `:41`, `BV2_ADMIN_PORT` `:42`.
- `docs/ops/runbook.md:24-28,74`.
- `game/src/Paths.cpp:363`, where `BV2_PUBLIC_ADDRESS` becomes `sv_publicAddress`.

I also read `REVIEW.md`, the step file, the records `reviews/step1.md`, `step2.md` and `step3.md`, and the commit message.

## Verified correct

- **Env file matches what `deploy.sh` accepts:**
  - The file is mode 600, owned by `ubuntu`, and contains `BV2_ADMIN_PASS`.
  - The keys `BV2_ADMIN_HOST`, `BV2_ADMIN_PORT` and `BV2_PUBLIC_ADDRESS` are the names compose interpolates.
  - `BV2_ADMIN_PORT=443` follows the README rule for a public name.
- **Cron line:** identical to the README one (`README.md:41`).
- **Order and permissions on first boot:**
  - `write_files` runs before `runcmd`.
  - `install -d -o ubuntu -m 700 /var/lib/rv2-staging` matches the round-1 fix for the root-owned state directory (step2 finding 6).
  - `sudo -u ubuntu` picks up the new `docker` group membership without a re-login, because sudo re-reads `/etc/group`.
- **No secrets in the repo:** the password is generated on the host and is never in the user data. An existing env file is not overwritten.
- **Inventory:** the `ARCHITECTURE.md` row is present and in order.
- **Scope:** every changed path is inside the step's Allowed paths (`tools/staging/**`, `ARCHITECTURE.md`).
- **File size:** no file comes near 1000 lines. No mechanical change is mixed with a functional one.

## Findings

1. **Medium: the host firewall rules are most likely dead code.** `tools/staging/cloud-init.yml:11-14`
   - Every port the stack exposes is a Docker-published port (`compose.example.yml:15,32,42`).
   - Docker DNATs external traffic to published ports in `nat PREROUTING` and filters it in `FORWARD` (its `DOCKER-USER`/`DOCKER-FORWARD` jumps go at the top of that chain). That traffic never reaches `INPUT`, so the two `INPUT` rules, the `sed` on `rules.v4` and the comment about "Docker's own rules" don't control access to 443, 3333 or 10207.
   - Fix: on the VM, check the packet counters of that rule (`iptables -L INPUT -v -n`) after a client joins. If they stay at zero, delete lines 11-14 and the "host firewall" part of the header, the `ARCHITECTURE.md` row and the README.
   - If they do count, replace the magic insert position 5 with `-I INPUT` (top of the chain). An ACCEPT rule doesn't care where it sits, and the hard-coded index ties the file to one image's rule layout. A generic Ubuntu 24.04 image, which the README promises at `README.md:13`, has neither the REJECT rule nor `/etc/iptables/rules.v4`.

2. **Medium: the repo now names the hosting provider, against the module's own rule.** `tools/staging/cloud-init.yml:3,11`
   - `tools/staging/README.md` says "The host's address, provider and access go in the private wiki ..., never in the repo."
   - The file names Oracle twice, and commit 8ce1ff0 states that the T8 host runs on it. The plan mentions Oracle only as one example among candidates (`20261014_testing_environment_plan.md:214`), so that is no exception.
   - Fix: make both comments provider-neutral, for example "paste as the VM's cloud-init user data" and "images that reject inbound traffic in iptables". Move the console path to the private wiki. If finding 1 deletes lines 11-14, only line 3 is left to fix.

3. **Medium (process): this work sits outside the step's tasks and was merged before review.** Commit 8ce1ff0.
   - The step lists tasks 2.1-2.5 and puts "Choosing and setting up the host" out of scope. This commit is host setup.
   - It is not named `stepN.M:`, came from a non-step branch (so the CI scope check couldn't run against the step), and was merged to `main` before this round.
   - Fix: add the task to the step file (for example "2.6 Host bootstrap", with the owner's approval of the scope change recorded there). In `reviews/step2.md`, record that this round covers a commit already on `main`. History can't be rewritten now; the record has to carry it.

4. **Low-medium: the new script has no test and no recorded run.** `tools/staging/cloud-init.yml:20-36`
   - `rv2-staging-setup` can be triggered by an operator, so under REVIEW.md section 3 it counts as new behaviour.
   - Nothing checks the user data parses, or that the generated env file passes `load_env`.
   - Fix: record the live VM bring-up in `reviews/step2.md` under fix proof / not done. It is the real evidence: env file accepted, first deploy healthy, `/deployed` shows the commit. If `cloud-init` is available on the CI runner, `cloud-init schema --config-file tools/staging/cloud-init.yml` is a cheap static check. Otherwise state explicitly that the live run is the only evidence.

5. **Low: two install paths now disagree on the env file.** `tools/staging/README.md:30-33`
   - The by-hand env file template lacks `BV2_PUBLIC_ADDRESS`, which `rv2-staging-setup` now writes (`cloud-init.yml:30-31`).
   - Without it, a host set up by hand is listed by the master at its compose-network address, so remote clients can't join from the browser (runbook `:74`, ADR 0022).
   - The cron line and env keys are now written in two places that must be kept in sync by hand.
   - Fix: add `BV2_PUBLIC_ADDRESS=<public IPv4>` to the README template.

6. **Low: `rv2-staging-setup` silently ignores its required argument on a re-run.** `tools/staging/cloud-init.yml:26-33`
   - When `/etc/rv2-staging.env` already exists, `<public name>` and the optional ntfy topic are not used, yet the script still demands the name and prints nothing about it. An operator re-running it to correct a mistyped DNS name gets the old name and no hint.
   - Fix: in the existing-file branch, print one line saying the env file is kept and to edit it by hand. Don't add merge logic.
