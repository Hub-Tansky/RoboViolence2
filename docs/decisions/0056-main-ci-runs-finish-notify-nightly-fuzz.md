# 0056: Main CI runs finish, a red main notifies the owner, fuzz runs nightly

- Status: Accepted
- Date: 2026-11-17
- Affects: `.github/workflows/build.yml` (`concurrency`, `notify`, `package-check`, `deb-check`), `.github/workflows/fuzz.yml`, `.github/workflows/ci-retry.yml`; amends [ADR 0041](0041-macos-ci-runner-not-acquired-retry.md) (concurrency); ROBO-221 (workflow audit WP-01, owner approved 2026-11-11) and ROBO-222

## Context
- `build` shared one concurrency group per ref with `cancel-in-progress: true`, so a merge cancelled the previous merge's run (run 1000): that merge never got a result.
- `build` on `main` failed at `fe9255a` and `3d5b3b4` on 2026-11-11 and nobody was told; the owner does not watch the Actions tab.
- The `fuzz` job ran on every push to `main` (owner, 2026-10-10) and took about 40 minutes per merge.
- `package-check` and `deb-check` failed on a transient `Could not resolve 'archive.ubuntu.com'`.

## Decision
- A PR's run is still cancelled by a newer commit. Runs on `main` and tags get a group of their own (`run_id`) and are never cancelled by concurrency. A shared group without cancelling would still drop a pending run when a third queues.
- `notify` job: when any job of a run on `main` fails, post the short sha, the failed job names and the run URL to `https://ntfy.sh/$NTFY_TOPIC` (repository secret). The topic reaches curl as config on stdin, as `tools/staging/deploy.sh` does, so it never appears in a log. Without the secret the job prints a line and succeeds. `clang-tidy` is left out: it is `continue-on-error` and never makes a run red.
- `fuzz` moves to `fuzz.yml`: nightly `schedule` and `workflow_dispatch`, unchanged steps. This replaces the 2026-10-10 placement on pushes to `main`.
- `apt-get -o Acquire::Retries=3` in `package-check` and `deb-check`.
- `ci-retry` keeps its "still the branch head" check for PR runs only. A `main` or tag run whose job got no runner is re-run even after a newer merge, since that run is the only result its commit gets.

## Consequences
- Every merge gets a `build` result; concurrent `main` runs can overlap and use more runners after a burst of merges.
- A fuzz regression shows up the next morning instead of on the merge; run `fuzz.yml` by hand for a change to message handling.
- A `main` run that ends only `cancelled` (a job with no runner) sends no ntfy message; `ci-retry` re-runs it (ADR 0041) up to twice, and a re-run that fails notifies.
- Rejected: keep cancelling on `main` (merges without a result); keep `fuzz` on `main` pushes (40 min per merge).
