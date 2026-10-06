# Step 0: Fork, rename, asset inventory and removal

**Status:** DONE (2026-10-06)

**Depends on:** nothing. Runs before every other step. **Next:** [step1-baseline-and-legacy-removal.md](step1-baseline-and-legacy-removal.md). **Index:** [README.md](README.md)

## Scope

| Field | Value |
|---|---|
| Goal | The project lives in a GitHub fork of `Daivuk/BaboViolent2`, named `RoboViolence2`. Its `main` branch contains none of the original assets listed in the asset licence notes (kept outside this repository); every removed file is catalogued for recreation; `README.txt` states the fork and the removal |
| In scope | Tasks 0.0–0.7 below |
| Out of scope | Renaming code identifiers, binaries (`bv2*`), window titles or in-game strings ([PNS-20](../possible-new-scope.md#pns-20-replacement-assets-and-rebrand)); creating replacement assets (PNS-20); all Step 1+ work |
| Allowed paths | Repository root dir name; `~/.gitconfig` include, `~/.gitconfig-hubtansky`, `~/.config/git/hubtansky-hooks/` (0.0); deletions of the paths in 0.4; `README.txt`; `docs/decisions/0004-*`; `tools/check-original-assets.py`; `tools/original-assets.sha256`; `.gitignore`; `AGENTS.md`; `docs/roadmap/**` |
| Inputs | the asset licence notes (kept outside this repository), the permission request (kept outside this repository), `AGENTS.md` |
| Deliverables | Renamed fork and local clone; the asset inventory (kept outside this repository); `tools/original-assets.sha256`; asset-removal commit; new `README.txt`; ADR 0004 |
| Definition of done | "Acceptance checks" below all pass |

**No merge requests before Step 0 is complete.** Step 0 commits go straight to `main` of the fork, in order. The first PR is Step 1.

## Context

- The GPLv3 covers code only. The assets and the name have no grant (the asset licence notes (kept outside this repository)), so the fork ships under a new name and without them.
- Asset files in the tree (2026-10-02):

  | Folder | Files |
  |---|---|
  | `Content/main/maps` | 81 `.bvm` |
  | `Content/main/textures` | 133 `.tga` |
  | `Content/main/skins` | 23 `.tga` |
  | `Content/main/models` | 26 `.DKO` + 2 `.tga` |
  | `Content/main/sounds` | 48 `.wav` + 2 `.ogg` |
  | `Content/main/fonts` | 1 `.tga` |
  | `Content/main/languages` (non-English) | 6 `.lang` |
  | `Assets`, `Design` | 62 `.max`/`.jpg`/`.dle` + 1 `.jpg` |
  | `Content/*.dll` | 4 |
  | `Content/main/License.txt`, `BV2-EULALegalForm.doc` | 2 |

- Fork history keeps the originals: a GitHub fork shares upstream objects, and upstream `master` contains every asset and the old `bv2.cfg` secrets. The removal makes `main` and every release clean. Rewriting history can't make a fork clean.
- A local repo already exists (`step1.3` baseline + 8 `harvest(...)` commits, unrelated history). 0.1 ports it onto the fork.

## Tasks

### 0.0 Project GitHub identity: `Hub-Tansky`

Every git and GitHub action for this project (commits, merges, pushes, forks, PRs) runs as **`Hub-Tansky`** (user ID 337104978). The personal `patpi` account and personal email are never used here; other projects keep `patpi`.

| Setting | Value |
|---|---|
| `user.name` | `Hub-Tansky` |
| `user.email` | `337104978+Hub-Tansky@users.noreply.github.com` |
| Push auth | SSH key `~/.ssh/id_ed25519_hubtansky`, registered on Hub-Tansky only |
| `gh` CLI | `GH_TOKEN=$(gh auth token --user Hub-Tansky)` per command; `patpi` stays the active `gh` account |

1. Scope git config to the project folder. Add this to `~/.gitconfig`:
   ```
   [includeIf "gitdir:~/Dev/BaboViolent_2_relaunch/"]
       path = ~/.gitconfig-hubtansky
   ```
   `~/.gitconfig-hubtansky` sets `user.name`, `user.email` and `core.sshCommand = ssh -i ~/.ssh/id_ed25519_hubtansky -o IdentitiesOnly=yes`. Remove the repo-local `user.*` override in the old repo's `.git/config`.
2. **User:** `ssh-keygen -t ed25519 -f ~/.ssh/id_ed25519_hubtansky`, then add the public key to Hub-Tansky (Settings → SSH keys). Run `gh auth login` and log in as Hub-Tansky; afterwards run `gh auth switch --user patpi`, so other projects stay on `patpi`.
3. Optional, for interactive shells: `~/Dev/BaboViolent_2_relaunch/.envrc` (direnv, outside the repo) exports `GH_TOKEN=$(gh auth token --user Hub-Tansky)`. Agents don't rely on direnv; they prefix each `gh` call explicitly.
4. Guards (done 2026-10-02): `~/.gitconfig-hubtansky` sets `core.hooksPath = ~/.config/git/hubtansky-hooks`, so they apply to every repo in the folder, including the new clone.
   - `pre-commit` rejects commits when `user.email` isn't the Hub-Tansky noreply address.
   - `pre-push` rejects remotes not under `github.com:Hub-Tansky/` or `github.com/Hub-Tansky/`, and any outgoing commit whose author or committer email differs.
   - Step 1 sets a repo-local `core.hooksPath = .githooks`, which overrides the folder-wide one. `.githooks/` must then include both checks next to gitleaks.

Check:

```bash
git config user.email && ssh -T -i ~/.ssh/id_ed25519_hubtansky git@github.com
```

Prints the noreply address, then `Hi Hub-Tansky!`.

### 0.1 Fork and port local work

1. Fork as Hub-Tansky; the fork is public, as all forks of public repos are:
   ```bash
   GH_TOKEN=$(gh auth token --user Hub-Tansky) gh repo fork Daivuk/BaboViolent2 --fork-name RoboViolence2 --default-branch-only --clone=false
   ```
   ```bash
   GH_TOKEN=$(gh auth token --user Hub-Tansky) gh api -X POST repos/Hub-Tansky/RoboViolence2/branches/master/rename -f new_name=main
   ```
2. Clone to `~/Dev/BaboViolent_2_relaunch/RoboViolence2-main`:
   ```bash
   git clone git@github.com:Hub-Tansky/RoboViolence2.git RoboViolence2-main
   ```
3. Port the existing local commits. Their history is unrelated, so cherry-pick everything after the root commit:
   ```bash
   git remote add old ../BaboViolent2-master && git fetch old && git cherry-pick 5eb7aad..old/main
   ```
   Conflicts are expected in `cMasterServer.cpp`, `GameVar.cpp` and `CUserLogin.cpp`, where the old baseline redacted lines; keep the redacted side. Then remove the remote: `git remote remove old`.
4. **Re-author the ported commits.** They carry the personal email, which must never be pushed:
   ```bash
   git rebase -r origin/main --exec 'git commit --amend --no-edit --reset-author'
   ```
   Check that `git log --format='%ae %ce' origin/main..` prints only the noreply address.
5. Archive the old dir outside the workspace; don't delete it until 0.7 passes.

### 0.2 Rename the local directory and project

- The working dir is `RoboViolence2-main`; the GitHub repo is `RoboViolence2`.
- Project name: **RoboViolence 2**, an unofficial fork of BaboViolent 2. Write ADR `docs/decisions/0004-project-name-roboviolence2.md` and add it to the ADR index.
- Update the first line of `AGENTS.md` and the README index. Code identifiers (`bv2`, `babo`) stay until §H.
- Claude Code keys its memory and `settings.local.json` by path. Copy `~/.claude/projects/-Users-piotr-Dev-BaboViolent-2-relaunch-BaboViolent2-master/` to the key for the new path, or start fresh.

### 0.3 Inventory every asset before removing it

A one-off script (Python 3, stdlib only, kept outside this repository) writes the asset inventory and `tools/original-assets.sha256`. It must cover **each file** that 0.4 removes, one row per file:

| Column | Source |
|---|---|
| Path | original path |
| Type | extension + format details: TGA width×height×bpp; WAV/OGG channels, rate, duration; BVM map size and version; DKO mesh count |
| Used by | `path:line` of every code reference to the file name (grep `BaboViolent2/Code`, `Engine`, the `.cfg`/`.lang` files); "unreferenced" if none |
| Purpose | What the file does in the game, written from the references (for example "rocket launcher world model", "CTF blue flag icon on minimap", "explosion sound"). `TODO` only when no reference explains it |
| Recreate | `required` (referenced by code), `content` (map or skin chosen at runtime), `source` (`.max`/`.jpg` source art; the replacement needs a source file, not this one), `drop` (unreferenced or third-party binary) |
| Replaced by | empty; filled by §H |

- Group rows by folder. Under each folder heading, record the constraints a replacement must meet (power-of-two TGA sizes, alpha usage, sample rate, `.DKO` scale and orientation, `.bvm` format reference [../../analysis/02_DATA_STRUCTURES.md](../../analysis/02_DATA_STRUCTURES.md)).
- Add a summary table at the top: counts per folder and per `Recreate` class.
- `original-assets.sha256` holds the SHA-256 of every removed file. Hashes contain no copyrighted content.
- `tools/check-original-assets.py` fails if any tracked file matches a hash in that list. Step 1 wires it into the pre-commit hook and CI, so originals can't come back, even renamed.
- Link the inventory from the asset licence notes (kept outside this repository) ("Replacement register" gets its rows from it).

### 0.4 Remove the original assets from `main`

Delete everything listed in the asset licence notes (kept outside this repository) "Scope", in one commit `step0.4: remove original assets`:

- `BaboViolent2/Content/main/{maps,models,skins,textures,fonts,sounds}/`
- `BaboViolent2/Assets/`, `BaboViolent2/Design/`
- `BaboViolent2/Content/*.dll`
- `BaboViolent2/Content/main/License.txt`, `BaboViolent2/Content/main/BV2-EULALegalForm.doc`
- Every translation except English: `BaboViolent2/Content/main/languages/{Polonais,fn,fr,hu,nl,swe}.lang`. These are the translators' work (credited in `Credits.cpp`), with no grant.
- Logos with the old name or RndLabs branding are textures (`RnDLabs.tga`, `HeadGames.tga`, …), so the textures deletion covers them.

Kept: `Content/main/languages/en.lang`, `Content/main/LaunchScript/`, `Content/main/bv2.cfg`, `Content/README.txt`.

- `en.lang` is the default and only remaining language: `languageFile` defaults to it (`BaboViolent2/Code/GameVar.cpp:267`, `bv2.cfg:4`), so removing the others breaks nothing.
- It still contains the old name (`lang_gameName`) and the RndLabs credits; §H rebrands it.
- New translations are written from `en.lang` by the project (§H).

Add to `.gitignore`, so a local copy of the original data can't be committed by accident:

```
/original-assets/
/BaboViolent2/Content/main/maps/
/BaboViolent2/Content/main/models/
/BaboViolent2/Content/main/skins/
/BaboViolent2/Content/main/textures/
/BaboViolent2/Content/main/fonts/
/BaboViolent2/Content/main/sounds/
```

Step 2 moves the remaining content to `content/`, where replacement assets are tracked normally. The hash check (0.3) is the guard there.

### 0.5 Development data without the originals

- Developers who own the original game keep its data **outside** the repo (for example `~/Games/bv2-data/main/`) and point the game at it with `BV2_DATA_DIR`. Step 4 §4.4 implements the variable; until then, symlink `main/` into the run dir.
- CI and contributors without the original data use generated placeholder content (step 2 §2.6).
- Document both in `README.txt` ("Running without the original assets") and `AGENTS.md`.

### 0.6 Replace `README.txt`

Replace the content entirely. It must say:

- RoboViolence 2 is a fork of <https://github.com/Daivuk/BaboViolent2> (BaboViolent 2 by bitHeads / RndLabs, GPLv3 code release).
- The original game assets (maps, models, textures, skins, fonts, sounds, music, source art) and the RndLabs EULA are **removed**. They aren't covered by the GPL and aren't redistributed. Licence notes and the inventory are kept outside this repository.
- How to run with your own copy of the original data (0.5).
- Code license: GPLv3 (`LICENSE.txt`). Not affiliated with or endorsed by the original authors.
- Pointers to `AGENTS.md`, `ARCHITECTURE.md` (after Step 1) and `docs/roadmap/`.

Step 1 §1.14 later turns it into `README.md`.

### 0.7 Verify and push

- Run the acceptance checks, then push `main` as Hub-Tansky.
- **User (GitHub UI):** enable secret scanning + push protection now (step 1 §1.2), before any PR.

## Critical files

- `tools/original-assets.sha256`, `tools/check-original-assets.py`
- `README.txt`, `AGENTS.md`, `.gitignore`, `docs/decisions/0004-project-name-roboviolence2.md`

## Acceptance checks

```bash
git log --format='%ae%n%ce' $(git merge-base HEAD upstream/master)..HEAD | sort -u
```
Prints only `337104978+Hub-Tansky@users.noreply.github.com`. Add the remote first: `git remote add upstream https://github.com/Daivuk/BaboViolent2.git && git fetch upstream`.

```bash
git remote get-url origin
```
Ends in `/RoboViolence2.git`, and `basename "$PWD"` is `RoboViolence2-main`.

```bash
git ls-files | grep -iE '\.(bvm|dko|tga|wav|ogg|max|dle|dll|doc)$|^BaboViolent2/(Assets|Design)/|Content/main/License\.txt'
```
Returns nothing.

```bash
git ls-files '*.lang'
```
Returns only `BaboViolent2/Content/main/languages/en.lang`.

```bash
python3 tools/check-original-assets.py
```
Passes.

- The inventory (kept outside this repository) has exactly one row per path in `original-assets.sha256`.
- `README.txt` names the fork URL and states that the original assets were removed.
- `git log --oneline` on `main` shows the ported commits, then `step0.*` commits. No PR was opened.
