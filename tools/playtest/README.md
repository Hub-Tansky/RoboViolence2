# Playtest agent (macOS)

Installs each successful `main` build on the owner's Mac, tests it against a disposable local server and notifies pass or failure. Scope and stages: [step 1](../../docs/roadmap/phase-t-testing-environment/step1-mac-pull-agent.md). Decision: [ADR 0017](../../docs/decisions/0017-playtest-pull-agent.md).

Never point it at pull request builds: it accepts only successful `push` runs on `main` of `Hub-Tansky/RoboViolence2`.

## Needs

macOS on Apple silicon, `gh` logged in as Hub-Tansky, and `jq`, `curl`, `ditto`, `shasum`, `nc`, `osascript` (all in macOS except `gh`).

## Install

```bash
mkdir -p ~/RV2-playtest/bin ~/.config && chmod 700 ~/RV2-playtest
```

```bash
cp tools/playtest/agent.sh ~/RV2-playtest/bin/
```

```bash
printf 'BV2_ADMIN_PASS=%s\n' "$(openssl rand -hex 16)" > ~/.config/rv2-playtest.env && chmod 600 ~/.config/rv2-playtest.env
```

Optional keys in the env file: `NTFY_TOPIC` (phone notifications via ntfy.sh), `RV2_STAGING_HOST`, `RV2_STAGING_ADMIN_PASS` and `RV2_STAGING_ADMIN_PORT` (staging tests; port default 8443, 443 for a host with a public certificate). Keep real values out of the repo; private notes go in `docs/prv_wiki/docs_restricted/testing-environment/`.

```bash
~/RV2-playtest/bin/agent.sh --preflight
```

```bash
sed "s|__HOME__|$HOME|g" tools/playtest/com.roboviolence2.playtest.plist > ~/Library/LaunchAgents/com.roboviolence2.playtest.plist
```

```bash
launchctl bootstrap gui/$(id -u) ~/Library/LaunchAgents/com.roboviolence2.playtest.plist
```

The agent runs at load, then every 10 minutes while the Mac is awake; a build that finished during sleep is tested at the first run after waking. Its first run is the preflight under `launchd`: if `gh` can't read the keychain there, it notifies `FAILED at preflight`.

## Use

| Command | Does |
|---|---|
| `agent.sh` | `local-smoke` on the newest successful `main` build not yet passed or failed 3 times (what `launchd` runs). In these scheduled runs, failures before the build is tested (GitHub, download, busy port, no console session) notify once and don't count |
| `agent.sh --sha <sha>` | `local-smoke` on that `main` build, ignoring the attempt limit |
| `agent.sh --play local` | master, server and client from the last passing build, for manual play; stays up |
| `agent.sh --play staging` | client from the last passing build, joined to `RV2_STAGING_HOST` |
| `agent.sh --stop` | ends `--play` |
| `agent.sh --test staging-connect [--sha <sha>]` | client joins staging; checks its `/status` |
| `agent.sh --preflight` | env file, tools and GitHub access only |

Files under `~/RV2-playtest/`: `versions/<sha7>/` (packages, reused if the same build is tested again), `pref/` (client config per build), `run/` (disposable servers: databases, logs), `reports/<sha7>.txt`, `state.tsv`, `current` (last passing build), `agent.log`.

Fixed ports: 3333 (game), 10207 (master), 18080 (admin API, plain HTTP on 127.0.0.1). The packaged master and server listen on all interfaces while a test runs.

## Uninstall

```bash
launchctl bootout gui/$(id -u)/com.roboviolence2.playtest
```

```bash
rm ~/Library/LaunchAgents/com.roboviolence2.playtest.plist
```
