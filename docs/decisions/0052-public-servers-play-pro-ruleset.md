# 0052: The shipped launch scripts play the Pro ruleset

- Status: Accepted
- Date: 2026-11-11
- Affects: `content/LaunchScript/CTF.cfg`, `content/LaunchScript/TDM.cfg` (prod and staging servers); amends a consequence of [ADR 0013](0013-r3-restore-normal-and-pro-rulesets.md); ROBO-199

## Context
- The owner reported on 2026-11-10 that the flamethrower is far stronger than in BaboViolent 2. Its range code and defaults match the original: at most `sv_ftMaxRange` 8 units, shrinking to `sv_ftMinRange` 1 after about 1.3 s of fire (`game/src/Game.cpp:1377`).
- Both launch scripts set `sv_serverType 0` (Normal), as the original CTF script did. On Normal every flame hit does the weapon's flat 8 damage up to 8 units. On Pro it does `sv_ftDamage` (0.065 in `config/bv2.example.cfg:150`, so 6.5) falling linearly to 0 at `sv_ftMaxRange` (`game/src/Player.cpp:1130`).
- In BV2 the assignment ADR 0013 fixed switched every server to Pro on the first rocket, so BV2 servers played Pro whatever they set. Since ADR 0013, our servers play real Normal rules, which no BV2 player knew.

## Decision
The CTF and TDM launch scripts set `sv_serverType 1` (owner, 2026-11-11). The engine default (0) and the Normal branches stay.

## Consequences
- Prod and staging play what BV2 servers played: flame and Photon damage fall with distance, rockets do `sv_zookaDamage` (85 instead of 75) in `sv_zookaRadius` (2 instead of 3) and can be detonated in the air (`sv_zookaRemoteDet`), the Pro shotgun spread, the chain gun's spread divided by 2.7 when nearly still (`game/src/Game.cpp:1255`), bullet weapons (SMG, sniper, shotgun, DMG, chain gun) take their damage from the `sv_*Damage` variables (`game/src/Player.cpp:1078`), shield cooldown 2.5 s, chain gun recoil halved.
- Takes effect when a server restarts on a build with these scripts; prod needs a redeploy.
- A host who wants Normal sets `sv_serverType 0` in their own launch script. A `config/local/*.cfg` value does not work: the config layers load first (`game/src/Paths.cpp:417`) and the launch script's `set` overrides them.
- Rejected: change the engine default to 1. `main/bv2.example.cfg` already sets 1 (`config/bv2.example.cfg:26`) and loads first, so the shipped servers would still play Normal through their scripts' `set sv_serverType 0`.
- Rejected: tune the flamethrower alone (`sv_ftMaxRange` or damage). It leaves the other weapons on rules BV2 players never played.
