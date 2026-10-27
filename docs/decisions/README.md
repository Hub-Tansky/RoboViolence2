# Architecture decision records

One file per non-obvious decision: `NNNN-kebab-title.md`, numbered in order, never reused.

## Rules

- An accepted ADR is never edited except for its **Status** line. To change a decision, write a new ADR and mark the old one `Superseded by NNNN`.
- Keep it under one screen. Cite code as `path:line`; link the Project item, issue, PR or file it affects.
- Add every new ADR to the index below and to the `ARCHITECTURE.md` inventory.
- An ADR moved to another repository keeps its number here: its index row links the file there with status `Moved to <repo> (NNNN)`, NNNN being the ADR that moved it.

## Format

```markdown
# NNNN: <decision as an imperative phrase>

- Status: Accepted | Superseded by NNNN | Deprecated
- Date: YYYY-MM-DD
- Affects: <Project item, issue, PR or file>

## Context
Facts that force the decision, with `path:line` citations. 2–5 bullets.

## Decision
What we do, in one or two sentences.

## Consequences
- What gets easier or harder; follow-up work it creates.
- Rejected options, one line each, with the reason.
```

## Index

| # | Decision | Status |
|---|---|---|
| [0001](0001-ninja-generator-on-all-presets.md) | Use the Ninja generator on all presets, MSVC included | Accepted |
| [0002](0002-compile-out-libcurl.md) | Compile out libcurl in Phase A | Accepted |
| [0003](0003-keep-opengl-2.1-then-sdl-gpu.md) | Keep OpenGL 2.1 for Phase A; isolate the renderer, then move it to SDL_GPU | Accepted |
| [0004](0004-project-name-roboviolence2.md) | Name the project RoboViolence 2, an unofficial fork of BaboViolent 2 | Superseded by 0008 |
| [0005](0005-runtime-main-data-root.md) | Keep the `main/` data root; the runtime dir assembles it | Accepted (amended by 0016) |
| [0006](0006-sdl3-miniaudio-glad-platform-layer.md) | SDL3, miniaudio and glad behind the unchanged dk* APIs | Accepted |
| [0007](0007-data-root-pref-dir-config-layers.md) | Data root, per-user pref dir and layered config | Accepted (amended by 0016, 0028) |
| [0008](0008-display-name-robo-violence-2.md) | Spell the display name "Robo Violence 2" | Accepted (identifier timing amended by 0009; in-game name by 0017) |
| [0009](0009-rename-internal-identifiers-continuously.md) | Rename internal identifiers continuously; interface names wait for §H | Accepted |
| [0010](0010-bv2-asset-compat-and-gui-freeze.md) | Keep BV2 asset compatibility and the BV2 interface until replacement assets ship | Accepted (amended by 0017) |
| [0011](0011-gettext-po-translations.md) | Use gettext PO for translations, keep .lang loading, default to the OS language | Accepted |
| [0012](0012-windows-10-22h2-minimum.md) | Support Windows 10 22H2 and later as the minimum Windows target | Accepted |
| [0013](0013-r3-restore-normal-and-pro-rulesets.md) | Restore the Normal and Pro rulesets (fix R3) | Accepted |
| [0014](0014-libsodium-hmac-for-credentials.md) | Use libsodium for the credential HMAC and nonces | Accepted |
| [0015](0015-admin-http-api-behind-tls-proxy.md) | Remote admin is an HTTP API behind a TLS reverse proxy | Accepted |
| [0016](0016-player-data-lookup-and-package-content.md) | The client finds the player's BV2 data; the package supplies its own content | Accepted (amended by 0033) |
| [0017](0017-playtest-pull-agent.md) | Test main builds on the owner's Mac with a pull agent | Accepted |
| [0018](0018-staging-pull-deploy.md) | Staging deploys itself by pull, pinned to a commit, only when empty | Accepted |
| [0019](0019-in-game-name-and-fork-credits.md) | The game shows its own name and credits the original as an unofficial fork | Accepted |
| [0020](0020-package-and-release-format.md) | Package and release format; engine-only public releases | Accepted |
| [0021](0021-cstring-one-argument-is-text.md) | A one-argument CString is text; formatting needs arguments | Accepted |
| [0022](0022-advertised-server-address.md) | The master lists an advertised address only from private sources | Accepted |
| [0023](0023-step-review-subagent-in-repo.md) | The step-closing review subagent and its rubric live in the repo | Accepted |
| [0024](0024-github-project-is-the-backlog.md) | The private GitHub Project is the backlog; no roadmap files in the repo | Accepted |
| [0025](0025-client-autotest-mode.md) | Test the real client through a scripted autotest mode | Accepted |
| [0026](0026-screenshot-goldens.md) | Screenshot goldens are Linux llvmpipe frames compared within a tolerance | Accepted |
| [0027](0027-several-game-servers-per-host.md) | Several game servers on one host share one image, one env file and one admin proxy | Accepted |
| [0028](0028-built-in-public-master.md) | The client and server ship with the public master built in | Accepted |
| [0029](https://github.com/Hub-Tansky/rv2-website/blob/main/docs/decisions/0029-static-website-on-github-pages.md) | The website is static HTML on GitHub Pages; downloads come from the GitHub releases API | Moved to rv2-website (0036) |
| [0030](0030-app-icon-and-dmg-layout.md) | One committed application icon for every OS; the .dmg shows only the app and Applications | Accepted |
| [0031](0031-linux-deb-package.md) | A .deb with the client and bv2dedicated beside the Linux .tar.gz | Accepted |
| [0032](0032-discord-release-post.md) | The release job posts each release to Discord through a webhook | Accepted |
| [0033](0033-first-run-dialog-recommends-bv2-zip.md) | The first-run data dialog recommends the original BV2 zip and can open it | Accepted |
| [0034](0034-player-data-retention-and-privacy-policy.md) | Player IPs stay on the server, personal data on disk expires, and a privacy policy says so | Accepted |
| [0035](https://github.com/Hub-Tansky/rv2-website/blob/main/docs/decisions/0035-website-tailwind-install-page-agent-files.md) | The website uses Tailwind from its CDN, has a separate install guide and publishes files for AI agents | Moved to rv2-website (0036) |
| [0036](0036-website-in-rv2-website-repository.md) | The website lives in Hub-Tansky/rv2-website | Accepted |
