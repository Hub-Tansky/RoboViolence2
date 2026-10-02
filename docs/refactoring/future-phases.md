# Future phases (after Phase A)

**Depends on:** Phase A complete ([README.md](README.md)). Each section becomes its own scope files (copies of [_template-step.md](_template-step.md)) before work starts.

Phase A leaves a modern, portable build with the gameplay unchanged. Everything below changes behaviour, security or scope.

Recommended order:

1. **A** (security)
2. **C.1–C.3** (sim core, commands, controllers)
3. **B** and **D** alongside
4. bots (**C.4–C.6**) and anti-cheat basics (**C.8–C.10**)
5. **E** (UDP), then server-authoritative movement (**C.7**)
6. **F** and **G** continuously
7. **H** (assets and rename) in parallel from the start; nothing ships publicly before H.1 provides a playable asset set

---

## A. Security and crash fixes (highest priority)

Source: [../analysis/KEY_QUESTIONS.md](../analysis/KEY_QUESTIONS.md) Parts C, D and E. Every item has `[VERIFY: file:line]` evidence; paths there predate the step 2 move (`BaboViolent2/Code` → `game/src`).

1. **Packet hygiene layer** in `Server::recvPacket` (`game/src/ServerRecv.cpp`):
   - resolve the player slot from `bbnetID` once and ignore the packet's `playerID` (Q-S1);
   - range-check `playerID` and `weaponID` (Q-S2);
   - enforce message sizes;
   - NUL-terminate and sanitise `mapName` (Q-S6);
   - fix the `UPDATE_SKIN` N² broadcast and validate `PLAY_SOUND` (Q-S7).

   This is also the base of the anti-cheat validation layer (C.8).
2. **Crash fixes R1–R5**. R3 (the `sv_serverType = 1` assignment) is load-bearing: decide the ruleset first.
3. **SQL**: `sqlite3_prepare_v2` with bound parameters everywhere (Q-S3), in the game server and `masterserver/src/cMasterServer.cpp`.
4. **Client `SV_CHANGE` filter**: accept only `sv_*` variables (Q-S4).
5. **Medium and low defects**: R6–R15 (R11 and R14 are already fixed in step 3) and babonet **R7**.

Build a **packet fuzz/replay harness** first (on the step 5 test scaffolding). It verifies these fixes and is reused by C.10.

## B. Credentials and accounts

- Server and admin passwords: challenge-response (HMAC with a server nonce) or TLS, instead of cleartext or unsalted MD5 (Q-S5).
- Account auth: game servers must never see reusable account secrets. Use short-lived signed session tokens issued by the account service and verified by the game server.
- Replace MD5 with a modern KDF (Argon2id / scrypt) on the account side.
- Stop using client-reported MAC addresses for identity or bans (spoofable, and the Q-S3 injection source). Identity comes from the account token (C.12).
- Secrets remain outside the repo (step 1 §1.2, step 4 §4.5).

## C. Foundations for bots and anti-cheat

Today there are **no AI players**. `Minibot` is a deployable drone weapon, not a bot. Anti-cheat is a few ad-hoc checks inside `ServerRecv.cpp`:

- a weapon-hack kick at old lines 689–703;
- a speed-hack counter over one velocity sample per 3 s window at old lines 801–826; see [../analysis/ALGORITHM_02-Movement-Collision-Interpolation.md](../analysis/ALGORITHM_02-Movement-Collision-Interpolation.md) §5.

Movement is client-authoritative ([../analysis/KEY_QUESTIONS.md](../analysis/KEY_QUESTIONS.md) Q2).

The items below are **enablers**: architecture that makes bots and anti-cheat possible without rewriting the game twice.

### Shared enablers

**C.1 Headless simulation core (`bv2_sim` library).**
- Extract the game rules into a library with no rendering, audio, UI or networking: `Game`, `Player` physics, `Map` collision and ray march, `Weapon`, `Projectile`, game modes, spawning.
- The `CONSOLE` build proves this code already runs headless. The work is replacing the scattered `#ifndef CONSOLE` blocks with a thin presentation interface (render and sound hooks) that the client implements and the server leaves empty.
- Client, dedicated server, tests, bots and replay tools all link `bv2_sim`.

**C.2 Per-tick player commands.**
- Introduce a `PlayerCommand` struct: tick number, move vector, aim direction, fire, secondary fire, melee, weapon switch, use.
- It becomes the **only** way anything drives a player: local input, network, bot or replay.
- Today the client writes position and velocity directly. It will produce commands, and the existing coord-frame path consumes them, so behaviour is unchanged at first.

**C.3 Player controllers and slot ownership.**
- A server-side `IPlayerController` interface with implementations:
  - `RemoteClientController`, bound to a `bbnetID` (reuses the A.1 slot binding);
  - `BotController`, with no connection;
  - later, `ReplayController`.
- Everything that assumes "every player has a babonet connection" must handle bot slots: broadcasts, pings, kicks, master-server reports, votes, auto-balance.

**C.4 Deterministic, seeded simulation.**
- Replace global `rand()` with a per-`Game` seeded RNG. Server-side spread is already rolled on the server in the Pro rules, see [../analysis/ALGORITHM_03-Weapons-Hitscan-Projectiles.md](../analysis/ALGORITHM_03-Weapons-Hitscan-Projectiles.md).
- Given the fixed 30 Hz step, the same seed + the same commands then reproduce a match. That's needed for replays (C.10), bot regression tests and investigating cheat reports.

### Bots

**C.5 World query API for AI**, a read-only facade over `bv2_sim`:
- line of sight and ray casts (reuse the map ray march and `segmentToSphere`);
- a navigation grid derived from the `.bvm` cell grid, with A* or flow fields, cached per map;
- spawn points, flag and objective positions, projectile threat queries.

**C.6 Bot framework.**
- `BotController` produces `PlayerCommand`s each tick from a simple utility or behaviour-tree brain (roam, engage, retreat, CTF carry/return).
- Difficulty comes from reaction delay, aim error and awareness radius, **not** from extra information the bot shouldn't have.
- New server vars: `sv_botCount`, `sv_botFillTo`, `sv_botDifficulty`.
- Bots are flagged in the player info packet (a protocol bump is fine).
- A headless **match runner** (`bv2sim --bots 8 --map X --ticks N --seed S`) supports regression, balance and performance tests in CI.

### Anti-cheat basics

Stance: the client is open source, so the **client can't be trusted**. Defences are server-side, based on authority, validation, information control and detection. No kernel or client-integrity anti-cheat; the removed binary checksum isn't coming back.

**C.7 Server-authoritative movement** (after E moves coord frames to UDP):
- clients send C.2 commands;
- the server simulates with `bv2_sim`;
- the client predicts and reconciles.

This removes speed, teleport and noclip cheats by construction. Until then, add interim checks to coord frames: per-tick max displacement against the weapon/speed rules, a server-side wall collision check with `Map`, and teleport detection.

**C.8 Central validation layer** (`ServerValidator`, built on A.1):
- sender binding and range checks;
- rate limits;
- fire rate against the weapon's `fireDelay`;
- ammo, reload and weapon-switch timing;
- spawn and team rules.

Every violation feeds a per-player **violation score** with configurable thresholds and actions (log, warn, kick, temp-ban). This replaces the ad-hoc kicks and `speedHackCount`.

**C.9 Match telemetry and event log.**
- A structured per-match event stream (JSON lines or SQLite): shots, hits, damage, accuracy, time-to-target and aim-snap angles, positions sampled at a low rate, violations.
- It feeds heuristic detection (aimbot: snap angle and accuracy outliers; wallhack: aim tracking through walls) and gives data for bot tuning.
- Privacy: key events on the player account ID, never the IP; set a retention period; nothing personal in the repo.

**C.10 Demo and replay recording.**
- Record the inbound commands and periodic snapshots per tick (C.2, C.4).
- Uses: reviewing player reports, reproducing bugs, fuzz corpora (A), and bot tests.

**C.11 Information hiding (anti-ESP/wallhack).**
- The server sends positions only for players the recipient could perceive: a visibility check against the map grid plus a margin.
- This needs C.1 queries and a per-recipient snapshot builder.

**C.12 Identity, reports and sanctions.**
- Stable player identity from B (account token).
- An in-game `/report` command attaches the C.10 replay window.
- Bans by account, with expiry, stored in the master DB via prepared statements.
- An admin review tool reads the C.9 logs.

## D. Relaunch infrastructure and scope decisions

- **Endpoints** (Q12): host your own account, master and report services over HTTPS. Their addresses are supplied only through local config or env (step 4 §4.5), **never committed**.
- **Account and ladder system** (`CUserLogin`, `CRegisterClan`): keep and host it, replace it, or remove it. This is a product decision. Any HTTP service turns `BV2_WITH_HTTP` back on or replaces `CCurl` ([ADR 0002](../decisions/0002-compile-out-libcurl.md)).
- **Remote admin** (Bv2RemoteAdmin was deleted in step 1): an authenticated admin channel, either RCON over TLS or an HTTP API in `bv2dedicated`, with rate limiting and audit logs. No plaintext UDP authorised by IP:port.
- **Distribution:**
  - Windows 11: signed installer or zip;
  - macOS: codesign + notarisation, DMG;
  - Linux: AppImage and/or Flatpak (+ `.deb`);
  - optionally Steam or itch.io.

  This replaces the deleted Launcher and UpdateServer.
- **Hosting:** container images for `bv2dedicated` and `bv2master` (Linux). Secrets are injected from the platform's secret store; an ops runbook lives in `docs/` without real hosts.

## E. Networking

- Move coord frames and, later, C.2 commands to UDP (`bb_serverCreate(true, …)`) only **after** auditing babonet's UDP / `cPeer2Peer` path, which the analysis didn't cover.
- Replace raw `memcpy` structs with explicit, versioned serialisation (bounds-checked reads, which also supports A.1).
- Tick-stamped snapshots and delta compression, as the base for C.7 and C.11.

## F. Rendering and UI

Immediate-mode OpenGL 2.1 plus GLU runs on macOS only through Apple's deprecated GL, which blocks macOS releases if Apple removes it. Plan ([ADR 0003](../decisions/0003-keep-opengl-2.1-then-sdl-gpu.md)):

1. **Isolate** (after the first playable match): gather the 144 `glBegin` blocks, display lists and GLU calls into one batched sprite/mesh renderer, still on GL 2.1. Replace GLU with small helpers.
2. **Replace** (when Apple announces OpenGL removal, or once regular players exist): move that renderer to SDL_GPU (Metal, Vulkan, D3D12), with shaders built by SDL_shadercross.

GL 3.3 core is not an option: on macOS it still runs on Apple's deprecated OpenGL.

- HiDPI and ultrawide UI scaling, gamepad menus.
- A bot/observer spectator view that uses C.5 debug overlays (navgrid, LOS).
- Analyse rendering, menus, the map editor and `CMeshBuilder` with the `codebase-analysis` skill first; [../analysis/README.md](../analysis/README.md) lists them as not covered.

## G. Code modernisation (continuous)

- Replace raw `sprintf`/`strcpy`/`strcat` (90 call sites) with `snprintf` or `std::string` as files are touched.
- Ownership: raw `new`/`delete` of `Player`, `Projectile` and `CControl` → `std::unique_ptr` where lifetimes are clear.
- Add unit tests alongside each fix in A–C. Keep `ARCHITECTURE.md` current in every change.

## H. Replacement assets and rebranding

Source of truth: `docs/assets/ASSET-INVENTORY.md` (step 0 §0.3), with one row per removed file and its purpose, constraints and `Recreate` class.

1. **Minimum playable set:** every `required` row. Textures, skins, fonts, models (`.DKO` via an exporter for a free tool, see 4), effects and one map per game mode. Use CC0/CC-BY or original work only.
2. **Record every replacement** in the inventory's `Replaced by` column and in the `ASSETS-LICENSE.md` replacement register: author, licence, source URL. The hash blocklist (`tools/check-original-assets.py`) stays on permanently.
3. **Maps:** recreate the `content` rows with the in-game editor. New maps get new names, not copies of the originals.
4. **Toolchain:** `.DKO` models came from 3ds Max via `dkoExporter.dle` (removed). Write a Blender exporter, or convert from glTF in `tools/`, and record the choice as an ADR.
5. **Audio:** new effects, and new music to replace the third-party tracks.
6. **Translations:** new language files written from `en.lang` by project contributors, under the project licence.
7. **Rebrand:** window title, menus, credits, binary names (`bv2*`), config/pref-path names, master-server game ID. Keep a "based on BaboViolent 2 by bitHeads / RndLabs" credit. This breaks config paths, so migrate the old pref dir once.
8. **Grants:** if the original authors grant rights ([../legal/permission-request.md](../legal/permission-request.md)), record them in `ASSETS-LICENSE.md`. Re-adding any granted original needs an ADR and removing its hash from the blocklist.
