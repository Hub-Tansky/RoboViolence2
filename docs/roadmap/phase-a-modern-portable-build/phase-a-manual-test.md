# Phase A manual test

**Status:** DONE (2026-10-06). The Results table was never filled in; see [possible-new-scope.md](../possible-new-scope.md) PNS-1.

Covers the two "Phase A done when" items CI can't check ([README.md](README.md#phase-a-done-when)): gameplay with original data on each OS, and the master server listing. Run it once per OS (Linux, macOS, Windows 10 22H2 or 11) and fill in [Results](#results).

## Setup

- Build the full preset, client included: `cmake --preset <preset> && cmake --build --preset <preset>`. `ctest --test-dir build/<preset> --output-on-failure` passes.
- Run everything from `build/<preset>/runtime/`. The macOS client is `./bv2.app/Contents/MacOS/bv2` (`open bv2.app` drops the env vars).
- Data dirs, outside the repo:
  - `<data>`: your original data (contains `main/`).
  - `<data-client>`: a copy of `<data>` with one map deleted from `main/maps/`. Call that map `<dl-map>`; it tests map download.
- Pref dirs, empty, one per process so configs don't collide: `<pref-srv>`, `<pref-cl1>`, `<pref-cl2>`.
- Two clients: two machines, or two instances on one machine. A second player is needed to test hits, damage and teams.

Set env vars per process:

| Shell | Syntax |
|---|---|
| bash/zsh | `BV2_DATA_DIR=<data> BV2_PREF_DIR=<pref-srv> BV2_MASTER_SERVERS=127.0.0.1:10207 ./bv2dedicated` |
| PowerShell | `$env:BV2_DATA_DIR="<data>"; $env:BV2_PREF_DIR="<pref-srv>"; $env:BV2_MASTER_SERVERS="127.0.0.1:10207"; .\bv2dedicated.exe` |

Use the master host's LAN IP instead of `127.0.0.1` when processes run on different machines. Ports: master TCP 10207, game server TCP 3333 (`sv_port`).

## 1. Master and server

1. Start `./bv2master`.
2. Start `bv2dedicated` with `BV2_DATA_DIR=<data>`, `BV2_PREF_DIR=<pref-srv>`, `BV2_MASTER_SERVERS=<master>:10207`.
3. In the server's stdin console:
   ```
   maplistall
   set sv_gameName "Phase A test"
   set sv_gamePublic true
   set sv_scoreLimit 3
   set sv_gameTimeLimit 300
   set sv_gameType 0
   dedicate <dm-map>
   addmap <dm-map>
   addmap <dl-map>
   ```
   Pick map names from `maplistall`. `sv_gameType`: 0 DM, 1 TDM, 2 CTF (`game/src/Game.h:39`).
4. The server log shows no "map not found" warning.

## 2. Client and master listing

1. Start client 1 with `BV2_DATA_DIR=<data-client>`, `BV2_PREF_DIR=<pref-cl1>`, `BV2_MASTER_SERVERS=<master>:10207`.
2. Menu: music and click sounds play; mouse and keyboard drive the menu.
3. Open the server browser. "Phase A test" is listed within 30 s (server heartbeat every 20 s, `game/src/Server.cpp:508`) with the right map and player count.
4. Join from the browser. Start client 2 the same way with `<pref-cl2>` and `<data>`, and join with the console command `connect <server-ip> 3333`.
5. Both players appear on both scoreboards (Tab), and the browser shows 2 players.

## 3. DM round

With both clients in game:

- [ ] Movement: W/A/S/D; aim with the mouse; shoot (left button), grenade (right), molotov (middle), melee (Space), pick up (F).
- [ ] Sound: own and other player's shots, hits, explosions, death; positional (left/right) when the other player is off-centre.
- [ ] Chat: T (all), messages appear on both clients.
- [ ] Damage and kills register on both clients; scores match on the scoreboard (Tab).
- [ ] Respawn after death.
- [ ] Round ends at `sv_scoreLimit 3`; the next map loads.
- [ ] Esc opens the in-game menu; Esc again returns to play.
- [ ] Window: toggle fullscreen/windowed and resize without losing input or rendering.

## 4. Map download

1. Server console: `changemap <dl-map>`.
2. Client 1 (whose data lacks `<dl-map>`) shows download progress and then loads the map.
3. `<pref-cl1>/maps/<dl-map>.bvm` exists and the map plays normally.
4. Disconnect and reconnect client 1: the map loads from the pref dir without downloading again.

## 5. TDM round

Server console: `set sv_gameType 1`, then `changemap <tdm-map>`.

- [ ] Both clients reload into TDM; team selection works; the two players end up on opposite teams.
- [ ] Friendly fire follows `sv_friendlyFire`; enemy kills score for the team.
- [ ] Team chat (Y) reaches only teammates.
- [ ] Round ends at the score limit and the next map loads.

## 6. CTF round

Server console: `set sv_gameType 2`, then `changemap <ctf-map>`. Alternatively restart the server with `./bv2dedicated CTF` (runs `main/LaunchScript/CTF.cfg` from the data dir).

- [ ] Picking up the enemy flag, carrying it home and capturing scores for the team on both clients.
- [ ] Killing the carrier drops the flag; touching your own dropped flag returns it.
- [ ] Capture and flag sounds play.
- [ ] Round ends at the score limit.

## 7. Shutdown

- [ ] Client "Quit" exits cleanly (no crash dialog, exit code 0).
- [ ] `quit` in the server console stops `bv2dedicated`; the master drops the server from the browser within 60 s (`MASTER_SERVER_TIMEOUT`, [../../../masterserver/README.md](../../../masterserver/README.md)).
- [ ] `<pref-*>/console.log` shows no errors or warnings that weren't expected.

## Results

One row per OS. All three hosts run server, master and both clients locally; note any cross-OS mix tested in Notes.

| OS (version) | Commit | Date | Tester | 1 | 2 | 3 DM | 4 Download | 5 TDM | 6 CTF | 7 | Notes |
|---|---|---|---|---|---|---|---|---|---|---|---|
| Linux (distro, X11/Wayland) | | | | | | | | | | | |
| macOS | | | | | | | | | | | |
| Windows (10 22H2 or 11; note which) | | | | | | | | | | | |

File each failure as an issue and link it in Notes. Phase A is done when every cell is a pass.
