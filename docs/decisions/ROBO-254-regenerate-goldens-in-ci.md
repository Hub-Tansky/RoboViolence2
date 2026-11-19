# ROBO-254: A workflow run by hand regenerates screenshot goldens on a branch and shows before and after

- Status: Accepted
- Date: 2026-11-19
- Affects: `.github/workflows/goldens.yml`, `tools/golden-report.py`, `docs/build/linux.md`, `docs/build/README.md`; ROBO-254 (workflow audit WP-15 part 5, approved 2026-11-11)

## Context
- Goldens are Linux llvmpipe frames under Xvfb ([ADR 0026](0026-screenshot-goldens.md)); the `smoke` job (linux-x64-asan, `.github/workflows/build.yml`) installs Xvfb and Mesa explicitly and checks them, so its setup is the one to copy. Regenerating one needs that setup (`docs/build/linux.md` "Regenerate a screenshot golden"), which the Mac and cloud sessions lack, and a reviewer sees only a binary PNG diff.
- A job summary can't show an image from the run itself: images need a URL. The repository is public, so `raw.githubusercontent.com` serves any committed file.
- A commit pushed with `GITHUB_TOKEN` starts no CI (ADR ROBO-251-2), so a bot commit of the new goldens to a PR branch would leave the PR without checks on its head.

## Decision
`goldens.yml` (`workflow_dispatch`: branch, optional names) has two jobs:
- `render` (read-only token, `persist-credentials: false`) checks out the branch, builds `bv2` with the linux-x64-asan preset (vcpkg cache key `goldens-linux-x64-asan`, which `smoke` never restores, since this job runs unreviewed branch code), runs `tests/screenshots.py --update` per golden, then compares each new golden 3 times, and runs `tools/golden-report.py` (from the workflow's own commit, since the branch may predate it, with the branch's `tests/screenshots.py` comparison) for the summary Markdown and the after and diff PNGs. The new PNGs go to the artifact `goldens`, laid out as `tests/golden/`.
- `publish` (contents write, builds nothing) commits the after and diff PNGs to `golden-preview/<run id>/` on the branch `golden-preview` and writes the summary: per golden the share of pixels beyond the test's tolerance, then before (the branch's golden at its commit), after and diff images. A golden that failed one of its 3 comparisons fails the run.
The person or agent who ran it commits the artifact to the branch, so CI runs on the new goldens.

## Consequences
- Goldens can be regenerated and reviewed from any session, with images on the run page.
- `golden-preview` grows by the changed images of each run; nothing reads it but the summaries, so it can be deleted at any time (old summaries then lose their images).
- Rejected: committing the goldens to the PR branch from the workflow (no CI on that commit); images as data URIs in the summary (GitHub strips them); another preset (llvmpipe draws the same frame; the `smoke` job's setup is the one written down).
