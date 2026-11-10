# 0049: Server-side bots fill empty servers to a target population

- Status: Accepted
- Date: 2026-11-10
- Affects: ROBO-123, `game/src/Bot.*`, `game/src/BotRoster.*`, `game/src/Server.cpp`, `game/src/Game.cpp`, `game/src/CMaster.cpp`, `game/src/AdminApi.cpp`, `tools/staging/deploy.sh`, `packaging/docker/compose.example.yml`

## Context
- New players find the public CTF and TDM servers empty and leave. BV2 has no bots.
- BaboViolent/babo-dedicated-server (Rust, GPL-3) has bots in `server/src/game/bots.rs` (commit 7c7d05c): utility-scored states, six CTF roles, tile navigation with stuck recovery, human-like aim, tuned against a headless simulation.
- Clients must not change: v0.0.2+ clients are in players' hands, and a protocol bump is a hard cut (release channel decision 2026-10-20).

## Decision
- **Bots run in the game server** (C++, a port of `bots.rs`). A bot is a player slot with `Player::isBot` and no connection (babonetID `Server::BOT_BABONET_ID + slot`). Each frame its brain decides, and it sends the same messages a client sends (team, spawn, coord frame, shoot, projectile, melee) through `Server::recvPacket`, so `checkPacket` and the validator apply to bots as to humans. Nothing is sent to a bot's babonetID.
- **`sv_bots` is a target population**: humans on blue or red plus bots = `sv_bots`, so each human who joins a team replaces one bot (never a flag carrier). A connecting or spectating human replaces none but takes a slot, and one slot below `sv_maxPlayer` stays free. Bots fill the smaller team first. `sv_bots` changes live (console, admin API); bots stay through map changes. Rules: `bots::wanted` (`BotRoster.cpp`).
- **Bots count as players** in the browser row and LAN broadcast (`nbPlayer`), so `sv_bots 8` shows 8/10 with or without humans. `Game::countPlayers()` returns humans and bots separately, so a later UI can show them apart. Names end in ` [bot]` and come from `content/bots/names.txt`; ping shows 0.
- Bots have user id 0, so nothing goes to stats or accounts; votes skip them.
- Difficulty is fixed constants in `bots::Tuning` plus per-slot variation, as in `bots.rs`. No skill cvar yet.
- Production enables them through the environment: `BV2_SV_BOTS` (compose default 8), not the launch scripts, which tests, the e2e run and client-hosted games also execute.
- `bot_sim` (tests) plays a seeded bots-only match and fails CI when CTF has no capture, TDM no kill, or a bot stands still away from its goal over 10 s.

## Consequences
- The staging deploy waits for 0 humans (`/status` line `humans N`), not 0 players, or bots would block every deploy.
- A bot costs one player slot and a little CPU per frame (perception every 0.08 s, a BFS on the tile grid when the straight line is blocked).
- Deviation from `bots.rs`: a retreating bot whose escape point resolves to where it stands fights back instead of holding still. The walled placeholder arenas corner bots; `bots.rs`'s maps rarely did.
- Bots see positions the server sees, not what a client would render, and never miss a sound. Tuning stays in one struct to adjust after playtests.
- Rejected: an external bot process connecting as a client. It needs the join handshake, a password, the network path and a slot on the master's view of the server, for no gain.
- Rejected: bots excluded from `nbPlayer`. The owner wants the browser to show a full-looking server (ROBO-123, Q8).
