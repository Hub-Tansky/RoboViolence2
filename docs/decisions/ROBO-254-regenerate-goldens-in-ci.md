# ROBO-254: A workflow run by hand on a branch regenerates its screenshot goldens and shows before and after

- Status: Accepted
- Date: 2026-11-19
- Affects: `.github/workflows/goldens.yml`, `tools/golden-report.py`, `docs/build/linux.md`, `docs/build/README.md`; ROBO-254 (workflow audit WP-15 part 5, approved 2026-11-11)

## Context
- Goldens are Linux llvmpipe frames under Xvfb ([ADR 0026](0026-screenshot-goldens.md)); the `smoke` job (linux-x64-asan, `.github/workflows/build.yml`) installs Xvfb and Mesa explicitly and checks them, so its setup is the one to copy. Regenerating one needs that setup (`docs/build/linux.md` "Regenerate a screenshot golden"), which the Mac and cloud sessions lack, and a reviewer sees only a binary PNG diff.
- A job summary can't show an image from the run itself: images need a URL. The repository is public, so `raw.githubusercontent.com` serves any committed file.
- A commit pushed with `GITHUB_TOKEN` starts no CI (ADR ROBO-251-2), so a bot commit of the new goldens to a PR branch would leave the PR without checks on its head.
- A run's cache lands in the scope of the ref it runs on; caches in `main`'s scope are restored by every PR and tag run. A run started from `main` that builds a branch's code could plant vcpkg binaries there.

## Decision
`goldens.yml` runs by hand on the branch to change (`--ref <branch>`, optional golden names); it skips `main`. Two jobs:
- `render` (read-only token, `persist-credentials: false`) builds `bv2` with the linux-x64-asan preset and the `smoke` job's vcpkg cache key (restored from `main`, saved only in the branch's scope), runs `tests/screenshots.py --update` per golden, compares each new golden 3 times, and writes the summary with `tools/golden-report.py`: per golden the share of pixels beyond the test's tolerance, then before (the branch's golden at its commit), after and diff images. The new PNGs are the artifact `goldens`, laid out as `tests/golden/`. A golden that failed one of its 3 comparisons fails the job.
- `publish` (contents write, runs no branch code) checks that each image is a PNG of at most 5 MB and commits the after and diff images to `<run id>/` on the branch `golden-preview`, where the summary's links point, keeping the newest 20 runs.
Whoever ran it commits the artifact to the branch, so CI runs on the new goldens.

## Consequences
- Goldens can be regenerated and reviewed from any session, with images on the run page.
- The branch must contain `goldens.yml`, so branches older than this ADR need a merge from `main` first.
- `golden-preview` cleans itself (owner request, 2026-11-19): each push replaces it with one commit holding only the newest 20 runs' images, with a lease so concurrent runs retry. Older summaries lose their images; nothing else reads the branch.
- A new golden also needs its name in the `screenshot_` list in `tests/CMakeLists.txt`, or CI never checks it.
- Rejected: committing the goldens to the PR branch from the workflow (no CI on that commit); images as data URIs in the summary (GitHub strips them); a `branch` input on a run from `main` (its cache would land in `main`'s scope).
