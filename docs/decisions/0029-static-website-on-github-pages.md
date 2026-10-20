# 0029: The website is static HTML on GitHub Pages; downloads come from the GitHub releases API

- Status: Accepted
- Date: 2026-10-20
- Affects: `website/`, `.github/workflows/pages.yml`, Project item "Website on GitHub Pages at roboviolence.com"

## Context
- Owner decision 2026-10-20 (map "First public release delivery"): the site lives at roboviolence.com, registered at Cloudflare; the apex and `www` point at GitHub Pages of this repository, DNS only.
- Downloads are the assets of the newest GitHub release; every `v0.*` release is a pre-release (`.github/workflows/build.yml`, `release` job), so `releases/latest` never finds them.
- Hosting and domain together must stay at or under EUR 10 a month.

## Decision
`website/` holds plain HTML, CSS and one ES module (`site.js`), with no build step. `pages.yml` tests `site.js` with `node --test` on PRs and deploys `website/` to Pages on push to `main`. The download button asks `api.github.com/repos/Hub-Tansky/RoboViolence2/releases` from the browser, takes the first non-draft release (pre-releases included) and picks the asset by the detected OS; without an answer it links to the releases page.

## Consequences
- Pages source must be "GitHub Actions" and the custom domain is set in the repository's Pages settings (no `CNAME` file: Actions deploys ignore it).
- The API allows 60 unauthenticated requests per hour per visitor IP; beyond that a visitor gets the releases-page fallback.
- The asset suffixes in `site.js` must follow the release job's file names: `tests/website/site.test.mjs` reads them from `build.yml`, and `pages.yml` runs on changes to it.
- Menu and footer are copied into each page (no build step); the same test fails when they differ.
- The Discord invite is one constant in `site.js`; the server status block (`#server-status`) is an empty hook for the status feed item.
- Rejected: a static site generator (a build step for three pages); hosting on the game VPS (couples the site to game server uptime); `releases/latest` links (skip pre-releases).
