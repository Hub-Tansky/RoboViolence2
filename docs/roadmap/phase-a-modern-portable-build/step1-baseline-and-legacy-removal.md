# Step 1: Baseline, repo hygiene, secret blocking, legacy removal

**Status:** DONE (2026-10-06)

**Depends on:** [Step 0](step0-fork-rename-and-asset-removal.md). **Next:** [step2-cmake-build.md](step2-cmake-build.md). **Index:** [README.md](README.md)

## Scope

| Field | Value |
|---|---|
| Goal | A sanitised git repository with secret blocking, a modern `.gitignore`, all sources in UTF-8/LF, `AGENTS.md` and `ARCHITECTURE.md`, and no dead code, legacy variants, vendored SDKs or committed binaries |
| In scope | Tasks 1.0–1.14 below |
| Out of scope | Any build system (Step 2); dependency upgrades (Step 3); code fixes beyond removing dead branches; the loader code for layered config (Step 4 §4.5); moving directories (Step 2 §2.0) |
| Allowed paths | Whole repository (this step mostly deletes). New root files: `.gitignore`, `.gitattributes`, `.gitleaks.toml`, `.editorconfig`, `.git-blame-ignore-revs`, `.githooks/`, `.github/workflows/secret-scan.yml`, `tools/`, `config/`, `content-seed/`, `ARCHITECTURE.md`, `AGENTS.md`, `CLAUDE.md` |
| Inputs | `AGENTS.md`, [../../analysis/01_SYSTEM_OVERVIEW.md](../../analysis/01_SYSTEM_OVERVIEW.md), [../../analysis/KEY_QUESTIONS.md](../../analysis/KEY_QUESTIONS.md) |
| Deliverables | Secret blocking active on the fork; one commit per task item; `ARCHITECTURE.md` |
| Definition of done | "Acceptance checks" below all pass |

## Context

The repo is the `RoboViolence2` fork of `Daivuk/BaboViolent2` set up in [Step 0](step0-fork-rename-and-asset-removal.md); the original assets are already gone. Upstream history is public and contains the old secrets and assets. History scans therefore cover only commits after the fork point (§1.2).

The source encoding is mixed. Measured over all 421 `*.cpp/*.h/*.c/*.m` files on 2026-10-01:

| Encoding | Files |
|---|---|
| Pure ASCII | 264 |
| Latin-1 / CP1252 (French comments) | 101 |
| Already valid UTF-8 | 56 |

**No file actually has CRLF line endings**, despite what older notes say. No string or char literal contains non-ASCII characters; accents only appear in comments. Converting everything to UTF-8/LF (§1.4) therefore can't change runtime behaviour.

It also contains **secrets and server addresses** that must never enter history. A minimal sanitised baseline was committed on 2026-10-02: `.gitignore` excludes `bv2.cfg` and `*.db`, and the source rows below are already redacted. The example config, seeds and blocking from 1.2 are still open:

| Where | What |
|---|---|
| `BaboViolent2/Content/main/bv2.cfg:46` | non-empty `sv_password` |
| `BaboViolent2/Content/main/bv2.cfg:168-170` | `cl_accountUsername` / `cl_accountPassword` (a real account name and password) |
| `BaboViolent2/Content/main/bv2.cfg:178` | non-empty `cl_lastUsedIP` (a real server IP) |
| `BaboViolent2/Content/bv2.db` table `MasterServers` | a real master server IP |
| `BaboViolent2/Content/bv2.db` table `LauncherSettings` | `AccountURL` (`ladder.baboviolent.ru`) |
| `MasterServer/Source/src/cMasterServer.cpp:466` and `cMasterServer_backup.cpp:435` | hard-coded banned IP |
| `BaboViolent2/Code/GameVar.cpp:555`, `CUserLogin.cpp:89` | hard-coded account/ladder host `ladder.rndlabs.ca` |

The tree also has Direct3D, non-Pro, launcher, updater and remote-admin code that never builds, three MD5 implementations, vendored 2005–2007 SDKs, and 32-bit binaries.

## Tasks

### 1.0 Preserve the original outside the repo

- Done in Step 0: upstream `Daivuk/BaboViolent2` is the reference. Keep `~/Archive/bv2-original-2012.tar.gz` and the local original data (step 0 §0.5) outside the repo.

### 1.1 `.gitignore` and `.gitattributes` (before any `git add`)

Replace the 2013 `.gitignore` with a sectioned one. It must include at least:

- **macOS:** `.DS_Store`, `._*`, `.AppleDouble`, `.LSOverride`, `Icon?`, `.Spotlight-V100/`, `.Trashes`, `.fseventsd/`, `.DocumentRevisions-V100/`, `.TemporaryItems/`, `.VolumeIcon.icns`, `.com.apple.timemachine.donotpresent`, `.AppleDB`, `.AppleDesktop`, `Network Trash Folder`, `Temporary Items`, `.apdisk`
- **Windows:** `Thumbs.db`, `ehthumbs.db`, `Desktop.ini`, `$RECYCLE.BIN/`, `*.lnk`, `*.stackdump`
- **Linux:** `*~`, `.directory`, `.Trash-*`, `.nfs*`, `.fuse_hidden*`
- **Knowledge and editor tools:** `.obsidian/`, `.trash/` (Obsidian), `.vscode/`, `.idea/`, `.vs/`, `*.swp`, `*.swo`, `*.code-workspace`, `.history/`, `.cache/` (clangd), `.fleet/`, `*.sublime-*`
- **AI agent local state:** `.claude/settings.local.json`, `.claude/*.local.*`, `.aider*`, `.cursor/`, `.continue/`, `.windsurf/`. Shared agent config such as `.claude/settings.json` and `AGENTS.md` **stays tracked**.
- **Build outputs:** `build/`, `out/`, `cmake-build-*/`, `CMakeUserPresets.json`, `vcpkg_installed/`, `compile_commands.json`, `*.o`, `*.obj`, `*.a`, `*.lib`, `*.so`, `*.so.*`, `*.dylib`, `*.dll`, `*.exe`, `*.pdb`, `*.ilk`, `*.app/`, `*.dSYM/`, `Testing/`
- **Runtime output:** `*.log`, `screenshots/`, `crash*.dmp`, `*.core`
- **Secrets and local config:**
  - `.env`, `.env.*` (but keep `!.env.example`)
  - `*.local.*`, `config/local/`, `secrets/`
  - `*.pem`, `*.key`, `*.p12`, `*.pfx`, `*.keystore`, `*.jks`, `*.mobileprovision`, `*.provisionprofile`, `id_rsa*`, `id_ed25519*`
  - `server.cfg`, `bv2.cfg` (real runtime configs; only `*.example.cfg` is tracked)
- **Generated databases:** `*.db`, `*.sqlite`, `*.sqlite3`, `*.db-journal`, `*.db-wal`. Databases are generated from `content-seed/*.sql` (see §1.2).

Replace `.gitattributes`:

- `* text=auto eol=lf`: every text file is stored and checked out with LF on all three OSes;
- `*.ps1 *.bat *.cmd text eol=crlf` (Windows scripts only);
- binary types: `*.tga *.png *.jpg *.wav *.ogg *.mp3 *.dko *.bvm *.ttf *.zip binary`.

Delete every existing `.DS_Store` in the tree.

### 1.2 Secrets: sanitise, redesign config, block (before the first commit)

**Sanitise:**

- `bv2.cfg` becomes `config/bv2.example.cfg`, with every password, account, IP and host set to `""`. The runtime `bv2.cfg` is gitignored; Step 4 creates it in the user pref path on first run.
- `LaunchScript/*.cfg` keep only empty `sv_password` / `zsv_adminPass`. Rename them `*.example.cfg` if they're meant to be edited.
- `BaboViolent2/Content/bv2.db`:
  1. Dump the schema plus non-secret defaults to `content-seed/bv2.sql`, with an **empty** `MasterServers` table and an empty `AccountURL`.
  2. Remove the `.db` from the tree. It gets generated from the seed at build or first run (`sqlite3 bv2.db < content-seed/bv2.sql`); Step 2 adds a CMake custom command.
- Empty `BaboViolent2/Code/bv2.db` and `MasterServer/Source/src/master.db` (both 0 bytes): delete. Put the master DB schema in `content-seed/master.sql`, recovered from the SQL in `MasterServer/Source/src/cMasterServer.cpp`.
- `cMasterServer.cpp:466`: delete the hard-coded IP ban. Bans belong in the master DB ban table.
- `GameVar.cpp:555` and `CUserLogin.cpp:89`: replace the host literals with `""` plus `// TODO(step4): endpoint from local config`. Account login is disabled until configured.

**Config policy** (write it into `AGENTS.md` and `config/README.md`):

- Tracked: `config/*.example.cfg`, holding defaults and placeholders only.
- Local and untracked: `config/local/*.cfg`, the user pref path, or environment variables `BV2_MASTER_SERVERS`, `BV2_ACCOUNT_URL`, `BV2_ADMIN_PASS`, …
- Production server config and credentials live in a deployment secret store, never in the repo.
- Step 4 §4.5 implements the loader order: example defaults → pref-path `bv2.cfg` → `*.local.cfg` → environment.

**Block:**

- `.gitleaks.toml`: extend the gitleaks default rules with custom rules:
  - non-empty values of `sv_password`, `zsv_adminPass`, `cl_password`, `cl_accountPassword`, `cl_accountUsername`, `cl_lastUsedIP` in any `*.cfg`;
  - public IPv4 literals in any file (allowlist `127.0.0.1`, `0.0.0.0`, `255.*`, RFC 5737 documentation ranges `192.0.2.0/24`, `198.51.100.0/24`, `203.0.113.0/24`, and license/header files);
  - `INSERT INTO MasterServers` with values in `*.sql`;
  - `http(s)://` hosts in `config/*.cfg` and `content-seed/*.sql`, unless `example.com` / `example.org`.
- `.githooks/pre-commit`: runs `gitleaks git --pre-commit --staged --redact --config .gitleaks.toml` and fails if gitleaks is missing.
- `tools/setup-dev.sh` and `tools/setup-dev.ps1`: run `git config core.hooksPath .githooks` and check that gitleaks and cmake are installed.
- Record the fork point: `git merge-base main upstream/master` → `tools/FORK_BASE` (one line, the commit hash).
- `.githooks/pre-commit` also runs `tools/check-original-assets.py` (step 0 §0.3).
- `.githooks/pre-commit` and `.githooks/pre-push` also carry the Hub-Tansky identity checks from step 0 §0.0, because the repo-local `core.hooksPath` replaces the folder-wide hooks.
- `.github/workflows/secret-scan.yml`: gitleaks on every push and PR over `$(cat tools/FORK_BASE)..HEAD` (`fetch-depth: 0`), plus `tools/check-original-assets.py`. Upstream commits are public already and excluded. It becomes a **required status check** (see step 5 §5.4).
- For the **user (manual, in GitHub settings)**: enable secret scanning + push protection, and branch protection on `main` requiring the secret-scan check. Agents must not change repository settings.

### 1.3 Clean working tree

1. Run `gitleaks dir . --config .gitleaks.toml` over the working tree. It must report nothing, and every row of the "Context" table must be resolved.
2. `tools/setup-dev.sh`, so the hooks are active.
3. `git ls-files -ci --exclude-standard` lists nothing (no tracked file matches `.gitignore`). Untrack any hit with `git rm --cached`.
4. Commit `step1.3: remove tracked secrets and ignored files`.

### 1.4 Convert all sources to UTF-8 + LF (one scripted commit)

Do this right after §1.3, **before any other code edit**, so every later diff and every agent works on UTF-8/LF files.

- Write `tools/convert-encoding.py` (Python 3, no dependencies). For every tracked `*.cpp *.h *.c *.m *.txt *.cfg *.lang *.md` outside binary asset folders (maps, models, sounds, textures):
  1. If the bytes are pure ASCII, leave the content alone (only normalise line endings).
  2. If they already decode as UTF-8, **don't re-encode**. Running `iconv -f ISO-8859-1` over these 56 files would double-encode them (`é` → `Ã©`). This is why a blanket `iconv` run is not allowed.
  3. Otherwise decode as **CP1252**. It's a superset of ISO-8859-1 for printable characters and covers the one "Non-ISO extended-ASCII" file (bytes 0x80–0x9F such as `’` `…`). Encode as UTF-8 without a BOM.
  4. Normalise `\r\n` / `\r` to `\n`, and make sure the file ends with a newline.
  5. Print a report: per-file original encoding and any character that decoded to U+FFFD or a C1 control (must be zero).
- Run the script with `--dry-run` first and review the report. Spot-check 3 converted files: the French comments must read correctly, for example `// ne pas oublier le damage infligé` in `netPacket.h`.
- Commit exactly the script output as `step1.4: convert sources to UTF-8/LF (no functional change)`, add the commit hash to `.git-blame-ignore-revs`, and make sure the commit contains no other change.
- Game data (`content/languages/*.lang`, `*.cfg`) is currently ASCII. The script checks that and converts them too if not. If a data file turns out to hold Latin-1 text that the bitmap font (`dkf`/`CFont`) renders by byte value, leave that file as is and record it in `ARCHITECTURE.md`. Decoding UTF-8 in `dkf` is a later task.
- Update `AGENTS.md`: replace "Source files are CRLF and ISO-8859-1 … `grep` may need `-a`" with "All text files are UTF-8 (no BOM) with LF line endings. Keep them that way; CI rejects other encodings." Drop `grep -a` from all docs and commands.
- Encoding guard, so the conversion can't regress:
  - add `.editorconfig` (`charset = utf-8`, `end_of_line = lf`, `insert_final_newline = true`);
  - add `tools/check-encoding.py`, which fails on any tracked text file that isn't valid UTF-8, has a BOM or contains `\r`;
  - run it in `.githooks/pre-commit` next to gitleaks, and in CI (step 5 §5.4 `hygiene`).

### 1.5 Agent entry points and `ARCHITECTURE.md`

- `AGENTS.md` is canonical. `CLAUDE.md` contains only `@AGENTS.md` (already done). Rewrite `AGENTS.md` to cover:
  - layout (current, then target from [README.md](README.md));
  - build commands;
  - conventions;
  - **agent rules** (scope files, allowed paths, commit naming, no secrets, never `--no-verify`, update `ARCHITECTURE.md`);
  - pointers to `docs/`.
- Create `ARCHITECTURE.md` at the root with two parts:
  1. **Summary** (≤ 1 page): deliverables, supported platforms, module map, tick model (30 Hz), network model (TCP, raw structs, who is authoritative), config and secrets policy, known-defect pointer (`docs/analysis/KEY_QUESTIONS.md`), build entry points.
  2. **File inventory**: every tracked file grouped by directory, as a table `| path | purpose (one line) |`.
     - Source, config, doc and script files are listed individually.
     - Replacement asset folders (once §H adds them) are listed per directory with file count, formats and purpose. The per-file catalogue is the asset inventory (kept outside this repository) (step 0 §0.3).
- `tools/check-architecture.sh` (plus `.ps1`): compares `git ls-files` with the paths in `ARCHITECTURE.md` and fails on unlisted or stale entries. CI enforces it from Step 5.
- Generate the inventory **at the end of this step**, after §1.6–1.14, so it describes the cleaned tree.

### 1.6 Remove the Direct3D path (`_DX_`)

- Scope: 222 lines in 55 files. Find them with `grep -rn "_DX_" --include='*.cpp' --include='*.h' BaboViolent2/Code Engine`.
- For every `#ifdef _DX_ … #else … #endif`, keep the OpenGL branch. Delete `#ifdef _DX_`-only blocks. Watch for `#ifndef _DX_` inversions.
- Delete `BaboViolent2/lib/*_dx.lib` and any DX-only sources (check `CMaterial.cpp`, `CSnow.*`, `screengrab.cpp`).

### 1.7 Collapse to the Pro ruleset (remove `_PRO_`)

- Scope: 134 lines in 35 files. Keep the `#ifdef _PRO_` branches.
- In `Server.h` / `Client.h`, keep a single `GAME_VERSION_SV/CL`. Step 4 bumps it.
- Remove the **client binary checksum challenge**, since it can't work with open-source builds. See `BaboViolent2/Code/ClientRecv.cpp:38-80`, the server side in `ServerRecv.cpp`, and the `BadChecksum` insert at `ServerRecv.cpp:1198`, which is also an SQL injection (Q-S3). A real integrity approach is in [future-phases.md](../future-phases.md) §C.
- Keep the Minibot weapon (`MINIBOT`, 33 lines). It's a deployable drone weapon, not an AI player.
- Read [../../analysis/KEY_QUESTIONS.md](../../analysis/KEY_QUESTIONS.md) Q6 first.

### 1.8 Remove Visual Leak Detector

- Delete `#include <vld.h>` (12 engine files) together with the surrounding `#ifdef`. Step 2 adds ASan presets in its place.

### 1.9 Remove vendored SDKs

- `BaboViolent2/inc/{sqlite3.h,curl/,fmod/,dinput.h,glext.h,gl/}`
- `BaboViolent2/lib/` (all of it)
- `Engine/DukZeven/Code/gl/`
- `Engine/DukZeven/Code/SDLMain.m`
- **Keep** the project's own headers: `baboNet.h`, `dk*.h`, `cMSstruct.h`, `LinuxHeader.h`, `platform_types.h`. Step 2 moves them.
- After this item the code won't build until Steps 2 and 3 land. That's expected.

### 1.10 Remove old build and project files

- Delete every `*.vcproj`, `*.sln` (including `Zeven.sln`), and `Engine/*/Code/install.sh` (it edits `/etc/ld.so.conf.d`).
- Delete `Engine/DukZeven/Code/readme.md` (Eclipse and apt notes).
- **Keep** `BaboViolent2/Code/BaboViolent2.vcxproj` and the root `Makefile` until Step 2 has copied their source lists.

### 1.11 Master server cleanup

- Delete the committed binaries and generated files:
  - `MasterServer/compiled_linux_x86.zip`, `src/linuxmaster`, `libBaboNet.a`, `libDKC.a`, `src/.deps/`
  - the autotools outputs: `configure`, `config.status`, `config.h`, `libtool`, `Makefile`, `Makefile.in`, `aclocal.m4`, `stamp-h1`, `optimized/`, `*.kdevelop`, `Makefile.cvs`
  - `cMasterServer_backup.*`, and the stray `Source/{cClient,cServer}.cpp`
- **Forked babonet/dkc** in `MasterServer/Source/src/` (`baboNet`, `cPacket`, `cPeer*`, `cUDPpacket`, `cConnection`, `cIncConnection`, `cDNSquery`, `CClient`, `CServer`, `CThread`, `md5*`, `dkc*`):
  1. Diff each against `Engine/`.
  2. Port any master-only fixes into `Engine/`.
  3. Delete the forks.
- Keep `cMasterServer.*`, `cNetManager.*`, `cBV2game.*`, `cPlayer.*`, `cUDPserver.*` (if it differs from `Engine/`), `cMSstruct.h` and `main.cpp`.

### 1.12 Remove unbuilt auxiliary tools

- `BaboViolent2/Bv2Launcher/`, `BaboViolent2/Bv2UpdateServer/` (32-bit ELF + `fl.db`), `BaboViolent2/Bv2RemoteAdmin/` (wxWidgets).
- A secure admin replacement is in [future-phases.md](../future-phases.md) §D.

### 1.13 Consolidate MD5 and drop OpenSSL

- Keep babonet's `md5class`. Switch `CMaster.cpp:28` (`<openssl/md5.h>`) and the `md5_2.h` users to it, then delete `md5_2.h`. OpenSSL is no longer a dependency.
- Replacing MD5 for passwords is part of [future-phases.md](../future-phases.md) §B.

### 1.14 Content and READMEs

- The remaining content is `Content/main/{languages,LaunchScript}` plus config (the assets went in Step 0).
- Turn the step 0 `README.txt` into `README.md`. Keep the fork and asset-removal statement word for word, and add platforms and build pointers. Remove the rndlabs.ca download instructions from `Content/README.txt`.

## Not in this step (keep for now)

- The account/ladder UI code (`CUserLogin`, `CRegisterClan`); only its hosts are removed.
- The babonet UDP / `cPeer2Peer` path.

## Critical files

- `.gitignore`, `.gitattributes`, `.gitleaks.toml`, `.githooks/pre-commit`, `tools/setup-dev.*`, `tools/check-architecture.*`, `.github/workflows/secret-scan.yml`, `tools/convert-encoding.py`, `tools/check-encoding.py`, `.editorconfig`, `.git-blame-ignore-revs`
- `config/bv2.example.cfg`, `content-seed/bv2.sql`, `content-seed/master.sql`
- `AGENTS.md`, `CLAUDE.md`, `ARCHITECTURE.md`, `README.md`
- `BaboViolent2/Code/{Server.h,Client.h,ClientRecv.cpp,ServerRecv.cpp,CMaster.cpp,GameVar.cpp,CUserLogin.cpp}`, `MasterServer/Source/src/cMasterServer.cpp`

## Acceptance checks

```bash
gitleaks git --config .gitleaks.toml --log-opts="$(cat tools/FORK_BASE)..HEAD"
```
Finds nothing in any commit after the fork point.

```bash
python3 tools/check-original-assets.py
```
Passes.

```bash
grep -rln "_DX_\|_PRO_\|vld.h\|openssl/" --include='*.cpp' --include='*.h' BaboViolent2 Engine MasterServer
```
Returns nothing.

```bash
git ls-files | grep -iE '\.(db|dll|lib|a|so|exe|zip)$|\.DS_Store|settings\.local\.json|/bv2\.cfg$'
```
Returns nothing.

```bash
tools/check-architecture.sh
```
Passes.

- A test commit that adds `sv_password "x"` to a `.cfg`, or a public IP literal to a `.cpp`, is **rejected** by the pre-commit hook. Revert it afterwards.
```bash
python3 tools/check-encoding.py
```
Passes: every tracked text file is UTF-8 without a BOM, with LF only.

- `git log` shows the Step 0 commits, `step1.3`, then the single `step1.4` encoding commit (listed in `.git-blame-ignore-revs`), then one commit per task item. No commit other than `step1.4` rewrites whole files.
