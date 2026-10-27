# 0036: The website lives in Hub-Tansky/rv2-website

- Status: Accepted
- Date: 2026-10-27
- Affects: `website/`, `tests/website/`, `tools/website-markdown.mjs`, `.github/workflows/pages.yml`, ADR 0029 and 0035 (all moved); [rv2-website ADR 0001](https://github.com/Hub-Tansky/rv2-website/blob/main/docs/decisions/0001-website-in-its-own-repository.md)

## Context
- Owner request 2026-10-23 (project thread "Website move to rv2-website"): everything that builds roboviolence.com leaves the game repository; the website needs a Pages deploy only, not the three-OS CI.
- The website tests read the release file names from `.github/workflows/build.yml`, which stays here.

## Decision
The website, its tooling, tests, Pages workflow and ADRs 0029 and 0035 moved with their history to [Hub-Tansky/rv2-website](https://github.com/Hub-Tansky/rv2-website), which deploys roboviolence.com. Its tests download this repository's `build.yml` daily and on every PR.

## Consequences
- Renaming a release file in `build.yml` turns rv2-website's daily run red; update `site.js` and `install.html` there in the same window (and `packaging/discord_release_post.py` here, as before).
- ADR numbers 0029 and 0035 stay retired here; the files live in rv2-website. ADR 0032 (Discord release post) stays here with its code.
- Code comments cite the published policy, `https://roboviolence.com/privacy.html`.
- Rejected: a submodule (two repositories to update for every page edit).
