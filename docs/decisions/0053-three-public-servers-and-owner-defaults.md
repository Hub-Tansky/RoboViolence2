# 0053: Three public servers, the owner's defaults, disabled secondaries hidden

- Status: Accepted
- Date: 2026-11-14
- Affects: `content/LaunchScript/*.cfg`, `config/bv2.example.cfg`, `game/src/Game.cpp` (weapon menu), `packaging/docker/`, `tools/staging/deploy.sh`; extends [ADR 0027](0027-several-game-servers-per-host.md) and [ADR 0052](0052-public-servers-play-pro-ruleset.md); ROBO-212

## Context
- The owner asked on 2026-11-14 for three public servers, in this order: "Deathmatch ring" (free for all, 12 players), "Get that flag." (CTF, 10), "Team Deathmatch" (TDM, 10).
- And, for every server and the Host tab: Pro ruleset, Normal spawn, win limit 7, friendly fire on, reflect off, force respawn off, `sv_ftMaxRange 5`, Nuke Bot and Mini Bot off and not shown in the weapon menu.
- `sv_enableNuclear` and `sv_enableMinibot` exist; the client greyed a disabled secondary but still listed it.

## Decision
- New `content/LaunchScript/FFA.cfg` (`sv_gameType 0`, port 3335, compose service `ffa`, admin API at `/ffa/`). CTF keeps 3333 and TDM 3334.
- The three scripts and `config/bv2.example.cfg` (the first config layer, so the Host tab's defaults) set the owner's values. Engine defaults in `GameVar.cpp` stay BV2's, as in ADR 0052.
- The weapon menu hides a secondary the server disables and closes up the rest (`game/src/SecondaryMenu.h`). A saved pick of a hidden one falls back to the first enabled one. `sv_enableSecondary false` still greys all four.

## Consequences
- Prod needs TCP and UDP 3335 open, the deploy agent refreshed (its `GAME_SERVERS` lists `ffa`), and an uptime monitor for 3335.
- A player's saved `bv2.cfg` keeps its old Host tab values: the new defaults reach new players, or those who delete `bv2.cfg`.
- The server browser sorts by ping, then player count (`game/src/CBrowser.cpp:46`), so the three servers on one host show in no fixed order. "First" is the order of scripts and services.
- Rejected: FFA on 3333 and CTF, TDM moved up. It changes every port players, monitors and firewalls know, for an order the browser doesn't keep.
- Rejected: change the engine defaults. The example config loads first and every script sets the values, so it would change nothing shipped, only tests and hosts without the example config.
