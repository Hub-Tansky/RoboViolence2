# Building and running Robo Violence 2

Pick your OS: [macOS](macos.md), [Linux](linux.md), [Windows](windows.md). Each guide covers building from source and running a downloaded package.

## Download or build

- **Download:** every CI run of the `build` workflow on GitHub (Actions → build → a green run → Artifacts; you need to be signed in to GitHub, and artifacts expire after 90 days) publishes one package per OS: `roboviolence2-win-x64-msvc.zip`, `roboviolence2-macos-arm64.zip` and `roboviolence2-linux-x64.tar.gz`. Each holds one folder with the client, `bv2dedicated`, `bv2master`, `main/`, `master.db`, `web.db`, run scripts and licences; Windows also gets `-setup.exe`, macOS a `.dmg`, Linux a `.deb` (client and `bv2dedicated` only, [ADR 0031](../decisions/0031-linux-deb-package.md)). Tagged releases (`v*`) publish the installer, the `.dmg` (only the app, [ADR 0030](../decisions/0030-app-icon-and-dmg-layout.md)), the macOS and Linux packages, the `.deb` and the source on the [releases page](https://github.com/Hub-Tansky/RoboViolence2/releases) ([ADR 0020](../decisions/0020-package-and-release-format.md)). Packages are Release builds and start fullscreen; the presets are Debug and start windowed; `<preset>-release` (e.g. `macos-arm64-release`) builds what the packages ship. The builds are unsigned; each guide says how to get past the OS warning.
- **Build:** CMake 3.25+, Ninja, Python 3 and vcpkg. The first build is slow because vcpkg compiles SDL3, miniaudio and SQLite; later builds reuse them.

## Game data (required)

- Playing needs your own copy of the original BaboViolent 2 data: the folder that contains `main/` (with `maps/`, `textures/`, `sounds/`, `fonts/`). It is never in this repository or the packages. Building and the tests work without it.
- The client finds it in this order ([ADR 0016](../decisions/0016-player-data-lookup-and-package-content.md)):
  1. `BV2_DATA_DIR` (the folder or its `main/`).
  2. The folder saved on an earlier start (`data-dir.txt` in the pref dir), or the saved placeholders choice, which skips steps 3 and 4.
  3. `original` beside the pref dir (installed by step 4), then `BaboViolent2-master` or `BaboViolent2` (the upstream repository, unzipped, also with the extra level Windows Extract All adds) in `Downloads`, `Desktop` or `Documents`; on Windows also `BaboViolent 2` or `BaboViolent2` in Program Files.
  4. On first start, a box offers "Download and play": the OS's `curl` and `tar`/`unzip` fetch the upstream data, which is checked against `tools/original-assets.sha256` and saved ([ADR 0040](../decisions/0040-first-run-downloads-original-data.md)). "Use placeholders" is remembered, also over data found later ([ADR 0042](../decisions/0042-game-assets-option-remembers-placeholders.md)); `--placeholders` skips it for one run.
  5. Options → Game assets: paste a data folder (or empty for placeholders), press Enter, restart.
- The game server reads only `BV2_DATA_DIR` for original data: it loads its maps from there, and from the package's `main/maps` (the game's own maps, `content/maps`, [ADR 0054](../decisions/0054-game-ships-its-own-maps.md)), and sends missing maps to clients.
- Launch scripts, language files, `bv2.example.cfg` and the game's own maps always come from the package's own `main/`.
- Without data the game uses generated placeholder maps, textures and sounds. That is only for CI and development: it looks rough, has no music, and doesn't match other players' maps.
- Saved settings, `bv2.db`, logs and downloaded maps go to the per-user pref dir ([ADR 0007](../decisions/0007-data-root-pref-dir-config-layers.md)); `BV2_PREF_DIR` overrides it:

  | OS | Pref dir |
  |---|---|
  | Windows | `%APPDATA%\BaboViolent2\bv2` |
  | macOS | `~/Library/Application Support/BaboViolent2/bv2` |
  | Linux | `${XDG_DATA_HOME:-~/.local/share}/BaboViolent2/bv2` |

## Playing a local game

Run each program from its folder (a package folder, or `build/<preset>/runtime/`). Ports: game server TCP 3333 (and UDP 3333 for the server browser's ping), master TCP 10207.

| Program | Package | Arguments |
|---|---|---|
| Master server | `run-master` | none; only needed to test with a local master, with `BV2_MASTER_SERVERS` (below) |
| Game server | `run-server` | launch script name, default `CTF` (`main/LaunchScript/CTF.cfg`). The default applies only with no arguments: with `--config <file>`, also name the script |
| Client | `run-client` | one console command, e.g. `connect 127.0.0.1` |

- Server and client use the public master `master.roboviolence.com:10207` by default ([ADR 0028](../decisions/0028-built-in-public-master.md)), so a local server shows up there after 20 s. To test with a local master, start it first and set `BV2_MASTER_SERVERS=127.0.0.1:10207` for both the server and the client; `BV2_MASTER_SERVERS=none` keeps a server off every master.
- Players on other machines use the server's LAN IP instead of `127.0.0.1`.
- The server console reads commands from stdin (`maplistall`, `changemap <map>`, `quit`).
- Full test script: [Phase A manual test](https://github.com/Hub-Tansky/RoboViolence2/blob/58c91b2a660db2b290b0b5f504381980a6aedba2/docs/roadmap/phase-a-modern-portable-build/phase-a-manual-test.md).

## Tests

```bash
ctest --test-dir build/<preset> --output-on-failure
```

`ctest --test-dir build/<preset> -N` lists them.

## CI

Workflows are in `.github/workflows/`. Run them locally with `tools/review.sh` (`--full <preset>` adds build and ctest).

A PR that changes only docs-tier paths (the review gate's docs tier, REVIEW.md section 5, except `docs/build/linux.md`, which `package-check` reads) skips the build steps of `build`, `smoke`, `tidy` and `images`; the jobs still report success. Push to `main`, `release/*` and tags always runs everything ([ADR 0068](../decisions/0068-docs-only-prs-skip-the-build.md)). A new step in those jobs needs `if: env.DOCS_ONLY != 'true'` (ctest `docs_only` checks).

Required (the repository ruleset "main protection", see [AGENTS.md](../../AGENTS.md#build)):

- `build`: Windows, macOS, Linux; Debug and `-release` presets. Uploads the Release `roboviolence2-<preset>` packages, the Windows installer and the macOS `.dmg`. The separate `release` job (not required) checks on every run that the release files are there and publishes them on a `v*` tag (ADR 0020).
- `smoke`: Linux ASan, ctest and a 10 s client run under xvfb.
- `scan`: `secret-scan.yml`.
- `architecture`.
- `hygiene`: paths, ignored files, size, encoding, content case.
- `review`: `tools/review.sh` on PRs: scope, anti-gaming, protocol, review record.
- `format`: clang-format on the changed C/C++ lines of a PR or push (ADR 0045). Run locally with `tools/check-format.sh`, fix with the `git clang-format <merge-base>` command it prints.

Not required:

- `codeql`, `clang-tidy`.
- `package-check`: Linux package in a clean `ubuntu:24.04`.
- `package-smoke`: Windows and macOS packages: server, master, and client for 10 s; Mesa llvmpipe on Windows.
- `containers`: server images and the compose example; pushes `:<sha>` and `:latest` to GHCR from `main`, `:<sha>` only from `release/**` ([ADR 0061](../decisions/0061-release-branch-for-production-hotfixes.md)).
- `ci-retry`: re-runs a `build` whose job never got a hosted runner (a PR run only while it is the branch head, `main` and tag runs always; [ADR 0041](../decisions/0041-macos-ci-runner-not-acquired-retry.md)).
- `dependabot-automerge`: turns on auto-merge for Dependabot's GitHub Actions minor and patch PRs; majors and Docker stay manual, and a push by anyone else, a major or a Docker update turns auto-merge off ([ADR ROBO-250](../decisions/ROBO-250-dependabot-actions-auto-merge.md)).
- `backlog-move`: after a merge, moves the PR's backlog item in rv2-docs to Done, or Testing while a `(post-deploy)` criterion is unticked (`tools/backlog-move.py`; [ADR ROBO-249](../decisions/ROBO-249-backlog-item-moves-on-merge.md)).
- On `main`: `dependency-graph`. Nightly: `fuzz` (`fuzz.yml`, ASan fuzzer, 2 min per client message type; also by hand with `workflow_dispatch`) and `flake-watch` (`flake-watch.yml`, the `e2e_*` and `bots` tests 10 times each; a failure opens or comments on a "Flaky test: <name>" issue and pings ntfy; [ADR 0064](../decisions/0064-nightly-flake-watch.md)). Weekly: `osv-vcpkg` (OSV check of the vcpkg ports, `tools/osv-vcpkg.py`). Weekly: `token-expiry` (days left on `SYMBOLS_TOKEN`; 14 or fewer, or a rejected token, pings ntfy and fails; [ADR ROBO-252](../decisions/ROBO-252-token-expiry-alert.md)). Monthly: `vcpkg-baseline` opens a draft PR moving the vcpkg baseline, with port versions and the OSV result ([ADR ROBO-251](../decisions/ROBO-251-monthly-vcpkg-baseline-pr.md)); the owner closes and reopens it to start CI, agents may not ([ADR ROBO-251-2](../decisions/ROBO-251-2-bot-pr-ci-on-reopen.md)). Bump sooner when `osv-vcpkg` reports a port.

A newer commit cancels a PR's `build` run. Runs on `main`, `release/**` (ADR 0061) and tags always finish, and a failed `build` on `main` or `release/**` sends the run URL and failed jobs to the owner's ntfy topic (`notify` job, `NTFY_TOPIC` secret).
