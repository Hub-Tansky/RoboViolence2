# Cutting a release

How a `v*` tag becomes a public release: the gate before the tag, the tag, the notes, production and Discord. The package and release format is [ADR 0020](../decisions/0020-package-and-release-format.md); the Discord post is [ADR 0032](../decisions/0032-discord-release-post.md). The release item on the Project board holds the per-release acceptance criteria; this page is the procedure behind them.

## 1. Pick the commit

- One `main` commit whose `build` run is green, including `package-check`, `deb-check` and `package-smoke`. Those three are not required checks on PRs, so look at the run on `main` itself.
- Nothing open that the release needs. Merge it first and wait for that merge's run.
- Download that run's artifacts for the manual test (step 2). The tag rebuilds the same commit, so they match what the release job publishes.

## 2. Manual install test

On a real machine per OS, with the packages from step 1. Before each run, remove what an earlier test left behind:

| OS | Pref dir (delete it) | BV2 data (move it away) |
|---|---|---|
| Windows | `%APPDATA%\BaboViolent2\bv2` | `BaboViolent2-master` in Downloads, Desktop, Documents |
| macOS | `~/Library/Application Support/BaboViolent2/bv2` | the same three folders |
| Linux | `${XDG_DATA_HOME:-~/.local/share}/BaboViolent2/bv2` | the same three folders |

Check that `BV2_MASTER_SERVERS` and `BV2_DATA_DIR` are not set (`echo %BV2_MASTER_SERVERS%` on Windows, `env | grep BV2_` elsewhere). Either one hides what the test is for.

On each OS, in this order:

1. **Install.** Windows: run the installer and leave "Don't contact the public server list" unticked. macOS: drag the app from the `.dmg` to Applications, then Open Anyway in System Settings → Privacy & Security. Linux `.deb`: `sudo apt install ./roboviolence2-linux-x64.deb`. Linux `.tar.gz`: unpack it and run `./run-client.sh`.
2. **First-run dialog.** Start the game. The dialog appears with the zip URL. Click "Download BV2 data": the browser downloads `BaboViolent2-master.zip` and the dialog comes back. Click "Use placeholders": the menu opens with placeholder art. Quit.
3. **Data in Downloads is found.** Unzip the zip in Downloads (giving `Downloads/BaboViolent2-master/`). Start again: no dialog, and the menu shows the original art and plays music. `data-dir.txt` now exists in the pref dir.
4. **Server browser.** Open the server list. It shows both production servers (CTF on port 3333 and TDM on 3334), served by `master.roboviolence.com`.
5. **Play.** Join one server and play one full round (to the round-end scoreboard).

Windows also gets an **upgrade test**: uninstall, install the previous release, start it once, then run the new installer over it. Apps & features lists one "Robo Violence 2", the Start-menu entry starts the new version, and steps 4 and 5 pass without touching any setting (a saved `cl_masterServers ""` from 0.0.1 must not hide the server list, [ADR 0028](../decisions/0028-built-in-public-master.md)).

macOS upgrades from 0.0.1 leave its `bv2.app` in Applications next to the new `Robo Violence 2.app`; delete it by hand.

Website: on each OS, open https://roboviolence.com. After the tag (step 3) the main button names this OS and links this release's file: `-setup.exe`, `.dmg`, `-linux-x64.tar.gz`. Before the tag it still offers the previous release.

## 3. Tag

From the owner's Mac (cloud sessions can't push tags). Never create the tag on GitHub's release page: the job's `gh release create` then fails on the existing release.

```
git fetch origin
git tag -a vX.Y.Z <full sha from step 1> -m "Robo Violence 2 vX.Y.Z"
git push origin vX.Y.Z
```

The tag's `build` run ends with the `release` job: it publishes the files (a pre-release for `v0.*` and `v*-*`), writes notes generated from the merged PRs, and posts to Discord `#announcements`.

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
The game ships with placeholder art. For the real look, use the original BaboViolent 2 data:
https://github.com/Daivuk/BaboViolent2/archive/refs/heads/master.zip, unzipped into Downloads.

## Community
Discord: <invite from website/site.js DISCORD_INVITE>
Bugs: the #bug-reports forum on Discord, or https://github.com/Hub-Tansky/RoboViolence2/issues
```

## 5. Production

Pin production to the tagged commit (skip it if `/deployed` already shows that SHA), see [runbook: Production](runbook.md#production):

```
ssh rv2-prod
rv2-prod-deploy --sha <full sha of the tag>
```

Then https://admin.roboviolence.com/deployed shows that SHA, and the server browser lists CTF and TDM again. A run while anyone plays deploys nothing and says `waits`: run it again later.

## 6. Discord

The release job's post waits in `#announcements`. Check its links, then click **Publish** so following servers get it (webhooks can't publish). If the job's Discord step failed, the release still exists: post by hand, since re-running the job fails on the existing release.
