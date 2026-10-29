# Cutting a release

How a `v*` tag becomes a public release: the gate before the tag, the tag, the notes, production and Discord. The package and release format is [ADR 0020](../decisions/0020-package-and-release-format.md); the Discord post is [ADR 0032](../decisions/0032-discord-release-post.md). The release item on the Project board holds the per-release acceptance criteria; this page is the procedure behind them.

## 1. Pick the commit

- One `main` commit whose `build` run is green, including `package-check`, `deb-check`, `package-smoke` and `release` (on `main` it only checks that every release file is there). Look at the run on `main` itself: the tag run's `release` job needs only `build`, so `package-check`, `deb-check` and `package-smoke` don't stop a release.
- Nothing open that the release needs. Merge it first and wait for that merge's run.
- Download that run's artifacts for the manual test (step 2). They are built from the same source as the release, but a `main` run stamps version `0.0.0-dev` (installer `AppVersion`, `.deb` version); only the tag run stamps the release version.

## 2. Manual install test

On a real machine per OS, with the packages from step 1. Before each run, remove what an earlier test left behind, using the lookup order and pref dirs in [Game data](../build/README.md#game-data-required): delete the pref dir, and move away every folder the automatic lookup (its step 3) would find, including an original BaboViolent 2 installed in Program Files on Windows.

Check that no `BV2_*` variable is set (`set BV2_` on Windows, `env | grep BV2_` elsewhere): `BV2_DATA_DIR` and `BV2_MASTER_SERVERS` hide what the test is for, and `BV2_PREF_DIR` moves the pref dir.

On each OS, in this order:

1. **Install.** Windows: run the installer and leave "Don't contact the public server list" unticked. macOS: drag the app from the `.dmg` to Applications, then Open Anyway in System Settings → Privacy & Security. Linux `.deb`: `sudo apt install ./roboviolence2-linux-x64.deb`. Linux `.tar.gz`: unpack it and run `./run-client.sh`.
2. **First-run download.** Start the game. The box offers "Download and play" and "Use placeholders". Click "Download and play": a progress window, then the menu with the original art and music. `data-dir.txt` in the pref dir points at `original` beside it ([ADR 0040](../decisions/0040-first-run-downloads-original-data.md)).
3. **Data is kept.** Quit and start again: no box, original art.
   Then the fallback: quit, delete `original` and `data-dir.txt`, go offline and start. "Download and play" ends in the failure box; "Open in browser" opens the `master.zip` download (finish it online). Unzip it into Downloads, start again: no box, original art.
4. **Server browser.** Open the server list. It shows both production servers (CTF on port 3333 and TDM on 3334), served by `master.roboviolence.com`.
5. **Play.** Join one server and play one full round (to the round-end scoreboard).

Windows also gets an **upgrade test**: uninstall, delete the pref dir, install the previous release, start it once and quit it from the menu, then run the new installer over it. Apps & features lists one "Robo Violence 2", the Start-menu entry starts the new version, and steps 4 and 5 pass without touching any setting. When the previous release is 0.0.1 or older, its `bv2.cfg` in the pref dir holds `cl_masterServers ""`; check that it does before upgrading, since that saved value must not hide the server list ([ADR 0028](../decisions/0028-built-in-public-master.md)).

macOS upgrades from 0.0.1 leave its `bv2.app` in Applications next to the new `Robo Violence 2.app`; delete it by hand.

## 3. Tag

From the owner's Mac (cloud sessions can't push tags). Never create the tag on GitHub's release page: the job's `gh release create` then fails on the existing release.

```
git fetch origin
git tag -a vX.Y.Z <full sha from step 1> -m "Robo Violence 2 vX.Y.Z"
git push origin vX.Y.Z
```

The tag's `build` run ends with the `release` job: it publishes the files (a pre-release for `v0.*` and `v*-*`), writes notes generated from the merged PRs, and posts to Discord `#announcements`.

Then check what was published:
- `package-check`, `deb-check` and `package-smoke` passed on the tag run too (they don't block the release job).
- The installer from the release page shows the release version in Apps & features.
- Website: on each OS, https://roboviolence.com's main button names that OS and links this release's file (`-setup.exe`, `.dmg`, `-linux-x64.tar.gz`).

If one fails: mark the release as a draft (`gh release edit vX.Y.Z --draft=true`) so the website falls back to the previous release, delete the Discord post instead of publishing it (step 6), skip step 5, fix it on `main` and release the next patch version. Never move a published tag; a draft keeps the tag and its files.

If the `release` job fails before a release is published (no release page, no Discord post), nothing reached players: check `gh release list` and delete a leftover draft with `gh release delete vX.Y.Z` (keep the tag until the step below), then fix it on `main`, wait for a green run, then delete the tag (`git push origin :refs/tags/vX.Y.Z` and `git tag -d vX.Y.Z`) and tag the fixed commit with the same version. Re-running the job can't help: it runs the tagged commit's own `build.yml`. If the fix touched only CI, the step 2 test of the earlier commit still holds; otherwise repeat step 2.

## 4. Release notes

Put this header above the generated list (edit the release on GitHub, or `gh release edit vX.Y.Z --notes-file notes.md`, with the generated part pasted at the end):

```
## What's new
<three to six player-facing lines; no internal ids>

## Update
<"Required: older clients can't join servers running vX.Y.Z" for a protocol change (an announced hard cut),
otherwise "Recommended" and why>

## Known issues
<what a new player will hit; link the public issue when there is one>

## Platforms and data
Windows 10 22H2+ x64, macOS 12+ on Apple Silicon, Linux x64 (Ubuntu 24.04, Mint 22, Debian 13 or newer).
Installers are unsigned: see https://roboviolence.com/install.html for the SmartScreen and Gatekeeper steps.
The game ships with placeholder art. On first start, "Download and play" fetches the original BaboViolent 2 data
from https://github.com/Daivuk/BaboViolent2 for the real look.

## Community
Discord: <the invite in README.md>
Bugs: the #bug-reports forum on Discord, or https://github.com/Hub-Tansky/RoboViolence2/issues
```

## 5. Production

Deploy the tag's full SHA as in [runbook: Production](runbook.md#production) ("Deploy a commit"), unless `/deployed` already shows it.

## 6. Discord

The release job's post waits in `#announcements`. Check its links, then click **Publish** so following servers get it (webhooks can't publish). If the job's Discord step failed, the release still exists: post by hand, since re-running the job fails on the existing release. No post and a green job: the log says `No DISCORD_RELEASE_WEBHOOK secret`; add the secret for the next release and post this one by hand.
