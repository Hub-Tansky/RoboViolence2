# Architecture decision records

One file per non-obvious decision, named after the scope item that makes it: `ROBO-NNN-kebab-title.md` (a second decision of the same item: `ROBO-NNN-2-kebab-title.md`). Parallel PRs took the same next number four times; a scope item's number is already unique ([ADR ROBO-248](ROBO-248-adrs-named-by-scope-item.md)). The files `0001` to `0070` keep their numbers; never add a new `NNNN-` file.

## Rules

- An ADR's id is its number (`0042`) or its scope item (`ROBO-248`, `ROBO-248-2`). Cite it as `ADR <id>`.
- An accepted ADR is never edited except for its **Status** line. To change a decision, write a new ADR and mark the old one `Superseded by <id>`.
- Keep it under one screen. Cite code as `path:line`; link the scope item (ROBO-NNN), issue, PR or file it affects.
- Add every new ADR to the index below, as a new last row.
- An ADR moved to another repository keeps its id here: its index row links the file there with status `Moved to <repo> (<id>)`, <id> being the ADR that moved it.

## Format

```markdown
# ROBO-NNN: <decision as an imperative phrase>   (a second ADR of the item: # ROBO-NNN-2: ...)

- Status: Accepted | Superseded by <id> | Deprecated
- Date: YYYY-MM-DD
- Affects: <scope item, issue, PR or file>

## Context
Facts that force the decision, with `path:line` citations. 2–5 bullets.

## Decision
What we do, in one or two sentences.

## Consequences
- What gets easier or harder; follow-up work it creates.
- Rejected options, one line each, with the reason.
```

## Index

| ADR | Decision | Status |
|---|---|---|
| [0001](0001-ninja-generator-on-all-presets.md) | Use the Ninja generator on all presets, MSVC included | Accepted |
| [0002](0002-compile-out-libcurl.md) | Compile out libcurl in Phase A | Accepted |
| [0003](0003-keep-opengl-2.1-then-sdl-gpu.md) | Keep OpenGL 2.1 for Phase A; isolate the renderer, then move it to SDL_GPU | Accepted (target re-checked per 0062) |
| [0004](0004-project-name-roboviolence2.md) | Name the project RoboViolence 2, an unofficial fork of BaboViolent 2 | Superseded by 0008 |
| [0005](0005-runtime-main-data-root.md) | Keep the `main/` data root; the runtime dir assembles it | Accepted (amended by 0016) |
| [0006](0006-sdl3-miniaudio-glad-platform-layer.md) | SDL3, miniaudio and glad behind the unchanged dk* APIs | Accepted |
| [0007](0007-data-root-pref-dir-config-layers.md) | Data root, per-user pref dir and layered config | Accepted (amended by 0016, 0028) |
| [0008](0008-display-name-robo-violence-2.md) | Spell the display name "Robo Violence 2" | Accepted (identifier timing amended by 0009; in-game name by 0017) |
| [0009](0009-rename-internal-identifiers-continuously.md) | Rename internal identifiers continuously; interface names wait for §H | Accepted (refactor items only since 0058) |
| [0010](0010-bv2-asset-compat-and-gui-freeze.md) | Keep BV2 asset compatibility and the BV2 interface until replacement assets ship | Accepted (amended by 0017, 0038; asset loading superseded by 0069) |
| [0011](0011-gettext-po-translations.md) | Use gettext PO for translations, keep .lang loading, default to the OS language | Accepted |
| [0012](0012-windows-10-22h2-minimum.md) | Support Windows 10 22H2 and later as the minimum Windows target | Accepted |
| [0013](0013-r3-restore-normal-and-pro-rulesets.md) | Restore the Normal and Pro rulesets (fix R3) | Accepted (amended by 0052) |
| [0014](0014-libsodium-hmac-for-credentials.md) | Use libsodium for the credential HMAC and nonces | Accepted (amended by 0038) |
| [0015](0015-admin-http-api-behind-tls-proxy.md) | Remote admin is an HTTP API behind a TLS reverse proxy | Accepted |
| [0016](0016-player-data-lookup-and-package-content.md) | The client finds the player's BV2 data; the package supplies its own content | Accepted (amended by 0033, 0040, 0042, 0054) |
| [0017](0017-playtest-pull-agent.md) | Test main builds on the owner's Mac with a pull agent | Accepted |
| [0018](0018-staging-pull-deploy.md) | Staging deploys itself by pull, pinned to a commit, only when empty | Accepted |
| [0019](0019-in-game-name-and-fork-credits.md) | The game shows its own name and credits the original as an unofficial fork | Accepted |
| [0020](0020-package-and-release-format.md) | Package and release format; engine-only public releases | Accepted |
| [0021](0021-cstring-one-argument-is-text.md) | A one-argument CString is text; formatting needs arguments | Accepted |
| [0022](0022-advertised-server-address.md) | The master lists an advertised address only from private sources | Accepted |
| [0023](0023-step-review-subagent-in-repo.md) | The step-closing review subagent and its rubric live in the repo | Superseded by 0037 |
| [0024](0024-github-project-is-the-backlog.md) | The private GitHub Project is the backlog; no roadmap files in the repo | Superseded by 0044 |
| [0025](0025-client-autotest-mode.md) | Test the real client through a scripted autotest mode | Accepted |
| [0026](0026-screenshot-goldens.md) | Screenshot goldens are Linux llvmpipe frames compared within a tolerance | Accepted (amended by 0043) |
| [0027](0027-several-game-servers-per-host.md) | Several game servers on one host share one image, one env file and one admin proxy | Accepted |
| [0028](0028-built-in-public-master.md) | The client and server ship with the public master built in | Accepted |
| [0029](https://github.com/Hub-Tansky/rv2-website/blob/main/docs/decisions/0029-static-website-on-github-pages.md) | The website is static HTML on GitHub Pages; downloads come from the GitHub releases API | Moved to rv2-website (0036) |
| [0030](0030-app-icon-and-dmg-layout.md) | One committed application icon for every OS; the .dmg shows only the app and Applications | Accepted |
| [0031](0031-linux-deb-package.md) | A .deb with the client and bv2dedicated beside the Linux .tar.gz | Accepted |
| [0032](0032-discord-release-post.md) | The release job posts each release to Discord through a webhook | Accepted |
| [0033](0033-first-run-dialog-recommends-bv2-zip.md) | The first-run data dialog recommends the original BV2 zip and can open it | Superseded by 0040 |
| [0034](0034-player-data-retention-and-privacy-policy.md) | Player IPs stay on the server, personal data on disk expires, and a privacy policy says so | Accepted |
| [0035](https://github.com/Hub-Tansky/rv2-website/blob/main/docs/decisions/0035-website-tailwind-install-page-agent-files.md) | The website uses Tailwind from its CDN, has a separate install guide and publishes files for AI agents | Moved to rv2-website (0036) |
| [0036](0036-website-in-rv2-website-repository.md) | The website lives in Hub-Tansky/rv2-website | Accepted |
| [0037](0037-review-rubric-single-repo-copy.md) | The review rubric has one repo copy, and the gate checks its hash | Superseded by 0047 |
| [0038](0038-dead-accounts-hidden-admin-and-ban-secrets-hardened.md) | Dead account pages are hidden; admin and master ban secrets meet NIST SP 800-63B-4 | Accepted |
| [0039](0039-widescreen-stretch-shows-more-map.md) | Widescreen (Stretch) shows more map sideways instead of stretching a 4:3 view | Accepted |
| [0040](0040-first-run-downloads-original-data.md) | The first-run box downloads the original BV2 data with the OS's own curl and unzip tools | Accepted (amended by 0042) |
| [0041](0041-macos-ci-runner-not-acquired-retry.md) | Build branches through their PR only, and re-run a build whose job never got a runner | Accepted (amended by 0056) |
| [0042](0042-game-assets-option-remembers-placeholders.md) | Options "Game assets" sets the BV2 data folder, and "Use placeholders" is remembered | Accepted |
| [0043](0043-first-start-fullscreen-desktop-and-game-browser.md) | A new player starts fullscreen at the desktop resolution, on the Game Browser tab | Accepted (amended by 0055) |
| [0044](0044-backlog-in-the-rv2-docs-vault.md) | The backlog is one Markdown file per scope item in the private rv2-docs vault | Accepted |
| [0045](0045-cpp-lint-pre-commit-on-changed-lines.md) | clang-format and clang-tidy run on commit through pre-commit, on changed lines only; clang-format also in CI | Accepted |
| [0046](0046-main-menu-shows-release-version-from-tag.md) | The main menu shows the release version from the tag, not the protocol number | Accepted |
| [0047](0047-two-axis-review-replaces-thermo-nuclear.md) | PR review runs isolated Standards and Spec workers from a vendored skill | Accepted (amended by 0057, 0059) |
| [0048](0048-client-crash-report-in-pref-dir.md) | Write client crash reports as a text file in the pref dir | Accepted |
| [0049](0049-server-side-bots-for-empty-servers.md) | Server-side bots fill empty servers to a target population | Accepted |
| [0050](0050-client-frame-pacing-and-sleep.md) | Client frame pacing follows vsync and window state; the display sleeps outside play | Accepted |
| [0051](0051-release-symbols-kept-privately-per-tag.md) | Keep release debug symbols privately, per tag in the rv2-docs vault | Accepted |
| [0052](0052-public-servers-play-pro-ruleset.md) | The shipped launch scripts play the Pro ruleset | Accepted |
| [0053](0053-three-public-servers-and-owner-defaults.md) | Three public servers, the owner's defaults, disabled secondaries hidden | Accepted (amended by 0054) |
| [0054](0054-game-ships-its-own-maps.md) | The game ships its own maps in content/maps | Accepted |
| [0055](0055-browser-refresh-uses-a-fresh-master-session.md) | The server browser's Refresh uses a fresh master session | Accepted |
| [0056](0056-main-ci-runs-finish-notify-nightly-fuzz.md) | Main CI runs finish, a red main notifies the owner, fuzz runs nightly | Accepted |
| [0057](0057-review-record-keyed-to-patch-id-post-deploy-criteria.md) | The review record is keyed to the diff's patch-id; post-deploy criteria don't block | Accepted |
| [0058](0058-agent-rule-order-ground-rules-3-4-refactor-only.md) | Agent rules have an order; ground rules 3 and 4 apply only in refactor items | Accepted |
| [0059](0059-slim-review-docs-gate-only-incremental-rounds.md) | Docs PRs get no review worker; later rounds review only the new commits | Accepted |
| [0060](0060-merge-by-risk-tier-with-owner-brief.md) | Merge by risk tier; MEDIUM and HIGH PRs carry an owner brief | Accepted (amended by ROBO-250) |
| [0061](0061-release-branch-for-production-hotfixes.md) | Refactor on main; production hotfixes on a release branch | Accepted |
| [0062](0062-asset-packs-mode-modules-client-independent-server.md) | Asset packs, mode modules and a client-independent server are the long-term direction | Accepted |
| [0063](0063-architecture-lists-modules-not-files.md) | ARCHITECTURE.md lists module directories, not files | Accepted |
| [0064](0064-nightly-flake-watch.md) | A nightly workflow repeats the e2e and bots tests and files one issue per flaky test | Accepted |
| [0065](0065-asset-pack-manifest.md) | An asset pack is a folder with a pack.tsv manifest; the base pack's is content/base-pack.tsv | Accepted |
| [0066](0066-game-loads-assets-by-id.md) | Game code loads assets by id; the base manifest's ids are compiled in | Accepted |
| [0067](0067-font-bytes-and-ttf-baking.md) | Text is one byte per glyph; bytes 160-255 hold 96 extra letters; a TTF is baked to babo.tga widths | Accepted |
| [0068](0068-docs-only-prs-skip-the-build.md) | A docs-only PR skips the build steps; its required checks still report success | Accepted |
| [0069](0069-asset-packs-stack-over-the-base-pack.md) | Asset packs stack over the base pack; the BV2 data is an optional pack on top | Accepted |
| [0070](0070-gltf-converts-to-dko.md) | New models are authored as glTF and converted to .DKO offline | Accepted |
| [ROBO-248](ROBO-248-adrs-named-by-scope-item.md) | Name new ADRs after their scope item, not the next number | Accepted |
| [ROBO-249](ROBO-249-backlog-item-moves-on-merge.md) | A merged PR's backlog item moves to Done or Testing by workflow | Accepted |
| [ROBO-250](ROBO-250-dependabot-actions-auto-merge.md) | Dependabot's GitHub Actions minor and patch updates merge themselves when green | Accepted |
| [ROBO-251](ROBO-251-monthly-vcpkg-baseline-pr.md) | A monthly workflow opens a draft PR that bumps the vcpkg baseline | Accepted (amended by ROBO-251-2) |
| [ROBO-252](ROBO-252-token-expiry-alert.md) | A weekly check pings the owner 14 days before SYMBOLS_TOKEN expires | Accepted |
| [ROBO-251-2](ROBO-251-2-bot-pr-ci-on-reopen.md) | The owner starts CI on the bot's vcpkg PR by closing and reopening it | Accepted |
| [ROBO-253](ROBO-253-weekly-workflow-metrics.md) | A weekly workflow appends PR and CI numbers to a CSV in rv2-docs | Accepted |
| [ROBO-254](ROBO-254-regenerate-goldens-in-ci.md) | A workflow run by hand on a branch regenerates its screenshot goldens and shows before and after | Accepted |
