# Phase B: work after Phase A

**Depends on:** Phase A complete ([README.md](README.md)). Each section becomes its own scope files (copies of [_template-step.md](_template-step.md)) before work starts.

Phase A leaves a modern, portable build with the gameplay unchanged. Everything below changes behaviour, security or scope.

## Ground rules (every section)

1. **BV2 asset compatibility** ([ADR 0010](../decisions/0010-bv2-asset-compat-and-gui-freeze.md)). Until §H.1 ships a full replacement set, the game must run on original BV2 data (`BV2_DATA_DIR`) unchanged: same file formats (`.DKO`, `.bvm`, textures, sounds, fonts, `.lang`), names and paths under `main/`. New formats are added beside old ones, never instead. A PR touching a loader is tested locally with original data; CI keeps using placeholders.
2. **GUI freeze** (same ADR). Menus, HUD, layout, fonts and visible text stay as in BV2. Items marked **[GUI]** change what the player sees: the owner must explicitly approve each one before its scope file is written. If an unmarked item turns out to change the interface, stop and ask.
3. **Internal renaming** ([ADR 0009](../decisions/0009-rename-internal-identifiers-continuously.md)). Replace `bv2`/`babo` with `roboviolence2` in code symbols, namespaces, source file and class names, internal CMake targets and variables, and comments, in files already being touched. Binaries (`bv2`, `bv2dedicated`, `bv2master`), `BV2_*` env vars and CMake options, cvars, config and pref paths, `bv2.db`, file formats, protocol and master-server IDs, and visible strings wait for §H.7. Renames are separate mechanical commits; tests stay green.

## Order

0. **§0** supply-chain security (Phase B step 0)
1. **§A** security and crash fixes
2. **§C.1–C.4** enablers, then the first anti-cheat checks **§C.5–C.7**
3. **§B**, **§D** and **§I** (localisation) alongside
4. **§C.8–C.9** telemetry and replays
5. **§E** (UDP), then server-authoritative movement **§C.10**
6. **§F**, **§G** and internal renaming continuously
7. **§H** (assets and rebrand) in parallel from the start. Nothing ships publicly before H.1 provides a playable asset set

---

## 0. Supply-chain security (Phase B step 0)

Repo settings change as `Hub-Tansky` (Settings → Advanced Security).

1. **Dependency graph** on. GitHub can't parse `vcpkg.json`, so add a workflow that submits the resolved vcpkg dependencies through the dependency submission API (vcpkg's `dependencygraph` feature flag with `GITHUB_TOKEN`, `contents: write`) on pushes to `main`. Done when Insights → Dependency graph lists the vcpkg ports.
2. **Dependabot alerts** and security updates on, plus `.github/dependabot.yml` for the `github-actions` ecosystem (weekly). This covers GitHub Actions only: the GitHub Advisory Database has no C/C++ ecosystem, so vcpkg ports in the graph get no alerts. vcpkg ports are covered by:
   - **OSV-Scanner** (`google/osv-scanner-action` reusable workflow) on PRs and weekly, results uploaded to code scanning. Its C/C++ coverage is partial; record which ports it resolves when adding it.
   - a manual **vcpkg baseline bump** monthly, or sooner when a port has a published CVE ([step3](step3-dependency-upgrades.md) process).
3. **CodeQL** advanced setup: `.github/workflows/codeql.yml`, languages `c-cpp` (manual build of `linux-x64`: `bv2`, `bv2dedicated`, `bv2master`) and `actions`; on push, PR and weekly. Not a required check until the first alerts are triaged; findings go into §A.

Update `ARCHITECTURE.md` and `AGENTS.md` (CI list).

## A. Security and crash fixes (highest priority)

Source: [../analysis/KEY_QUESTIONS.md](../analysis/KEY_QUESTIONS.md) Parts C, D and E. Every item has `[VERIFY: file:line]` evidence; paths there predate the step 2 move (`BaboViolent2/Code` → `game/src`).

1. **Packet hygiene layer** in `Server::recvPacket` (`game/src/ServerRecv.cpp`):
   - resolve the player slot from `bbnetID` once and ignore the packet's `playerID` (Q-S1);
   - range-check `playerID` and `weaponID` (Q-S2);
   - enforce message sizes;
   - NUL-terminate and sanitise `mapName` (Q-S6);
   - fix the `UPDATE_SKIN` N² broadcast and validate `PLAY_SOUND` (Q-S7).

   This is the base of the anti-cheat validator (C.5).
2. **Crash fixes R1–R5**. R3 (the `sv_serverType = 1` assignment) is load-bearing: decide the ruleset first.
3. **SQL**: `sqlite3_prepare_v2` with bound parameters everywhere (Q-S3), in the game server and `masterserver/src/cMasterServer.cpp`.
4. **Client `SV_CHANGE` filter**: accept only `sv_*` variables (Q-S4).
5. **Medium and low defects**: R6–R15 (R11 and R14 are already fixed in step 3) and babonet **R7**.

Build a **packet fuzz/replay harness** first (on the step 5 test scaffolding). It verifies these fixes and is reused by C.9.

## B. Credentials and accounts

- Server and admin passwords: challenge-response (HMAC with a server nonce) or TLS, instead of cleartext or unsalted MD5 (Q-S5).
- Account auth: game servers must never see reusable account secrets. Use short-lived signed session tokens issued by the account service and verified by the game server.
- Replace MD5 with a modern KDF (Argon2id / scrypt) on the account side.
- Stop using client-reported MAC addresses for identity or bans (spoofable, and the Q-S3 injection source). Identity comes from the account token (C.12).
- Keep the existing login and registration screens; any new dialog or field is **[GUI]**.
- Secrets remain outside the repo (step 1 §1.2, step 4 §4.5).

## C. Anti-cheat foundations

AI players are out of scope (owner decision, 2026-10-06); the former bot items (world query API, bot framework, match runner) are dropped. `Minibot` stays a deployable drone weapon.

Today's anti-cheat is ad-hoc checks in `game/src/ServerRecv.cpp`:

- a spawn weapon whitelist kick at `ServerRecv.cpp:645`;
- a speed-hack counter at `ServerRecv.cpp:795–821` (`Player::speedHackCount`), one velocity sample per 3 s window; see [../analysis/ALGORITHM_02-Movement-Collision-Interpolation.md](../analysis/ALGORITHM_02-Movement-Collision-Interpolation.md) §5.

Movement is client-authoritative ([../analysis/KEY_QUESTIONS.md](../analysis/KEY_QUESTIONS.md) Q2).

Stance: the client is open source, so the **client can't be trusted**. Defences are server-side: authority, validation, information control and detection. No kernel or client-integrity anti-cheat; the removed binary checksum isn't coming back. None of this changes the client's visible interface; kicks reuse the existing disconnect messages.

### Enablers

**C.1 Headless simulation core (`roboviolence2_sim` library).**
- Extract the game rules with no rendering, audio, UI or networking: `Game`, `Player` physics, `Map` collision and ray march, `Weapon`, `Projectile`, game modes, spawning.
- The `CONSOLE` build proves this already runs headless. Replace the scattered `#ifndef CONSOLE` blocks with a thin presentation interface (render and sound hooks) that the client implements and the server leaves empty. Client output must stay identical.
- Client, dedicated server, tests and replay tools link it. The server uses it to validate movement (C.7) and later to simulate it (C.10).

**C.2 Per-tick player commands.**
- A `PlayerCommand` struct: tick number, move vector, aim direction, fire, secondary fire, melee, weapon switch, use.
- The **only** way anything drives a player: local input, network or replay.
- The client still sends position and velocity at first; commands feed the existing coord-frame path, so behaviour is unchanged.

**C.3 Slot ownership.**
- One server-side map from player slot to its `bbnetID` (reuses the A.1 binding) and an `IPlayerController` interface: `RemoteClientController` now, `ReplayController` for C.9.

**C.4 Deterministic, seeded simulation.**
- Replace global `rand()` with a per-`Game` seeded RNG. Spread is already rolled server-side in the Pro rules ([../analysis/ALGORITHM_03-Weapons-Hitscan-Projectiles.md](../analysis/ALGORITHM_03-Weapons-Hitscan-Projectiles.md)).
- With the fixed 30 Hz step, the same seed and commands reproduce a match: needed for replays and for investigating cheat reports.

### First anti-cheat features

Each new check ships behind `sv_antiCheat` (0 off, 1 log only, 2 enforce; default 1) so thresholds are tuned on real matches before anyone is kicked.

**C.5 `ServerValidator` (refactor, no behaviour change).**
- Move the spawn weapon kick and `speedHackCount` out of `ServerRecv.cpp` into `ServerValidator`, built on A.1 (sender binding, range checks).
- Every violation feeds a per-player **violation score** with configurable thresholds and actions (log, warn, kick, temp-ban). The current kicks become rules with today's thresholds; unit tests pin the old behaviour first.
- Add rate limits per message type.

**C.6 Weapon checks.**
- Shots faster than the weapon's `fireDelay`, shots with no ammo or during reload, weapon-switch timing, spawn and team rules.

**C.7 Movement sanity checks** (interim until C.10).
- Per-tick maximum displacement against the speed rules, a wall-collision check against `Map`, and teleport detection, with spawns and deaths resetting state.
- Replaces the single-sample speed check.

### Later

**C.8 Match telemetry and event log.**
- A per-match event stream (JSON lines or SQLite): shots, hits, damage, accuracy, time-to-target and aim-snap angles, low-rate positions, violations.
- Feeds heuristic detection (aimbot: snap angle and accuracy outliers; wallhack: aim tracking through walls) and threshold tuning for C.5–C.7.
- Privacy: key events on the account ID, never the IP; set a retention period; nothing personal in the repo.

**C.9 Demo and replay recording.**
- Record inbound commands and periodic snapshots per tick (C.2, C.4). Uses: reviewing reports, reproducing bugs, fuzz corpora (§A). Playback UI is **[GUI]**.

**C.10 Server-authoritative movement** (after §E moves coord frames to UDP).
- Clients send C.2 commands, the server simulates with C.1, the client predicts and reconciles. Removes speed, teleport and noclip cheats by construction.

**C.11 Information hiding (anti-ESP/wallhack).**
- The server sends positions only for players the recipient could perceive: a visibility check against the map grid plus a margin, built per recipient on C.1.

**C.12 Identity, reports and sanctions.**
- Stable identity from §B (account token). Bans by account, with expiry, in the master DB via prepared statements. An admin review tool (outside the game client) reads C.8 logs.
- An in-game `/report` command that attaches the C.9 replay window is **[GUI]**.

## D. Relaunch infrastructure and scope decisions

- **Endpoints** (Q12): host your own account, master and report services over HTTPS. Their addresses are supplied only through local config or env (step 4 §4.5), **never committed**.
- **Account and ladder system** (`CUserLogin`, `CRegisterClan`): keep and host it, replace it, or remove it. This is a product decision; removing its menus is **[GUI]**. Any HTTP service turns `BV2_WITH_HTTP` back on or replaces `CCurl` ([ADR 0002](../decisions/0002-compile-out-libcurl.md)).
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
- Tick-stamped snapshots and delta compression, as the base for C.10 and C.11.
- Map download keeps sending the `.bvm` format unchanged (ground rule 1).

## F. Rendering and UI

Immediate-mode OpenGL 2.1 plus GLU runs on macOS only through Apple's deprecated GL, which blocks macOS releases if Apple removes it. Plan ([ADR 0003](../decisions/0003-keep-opengl-2.1-then-sdl-gpu.md)):

1. **Isolate** (after the first playable match): gather the 144 `glBegin` blocks, display lists and GLU calls into one batched sprite/mesh renderer, still on GL 2.1. Replace GLU with small helpers.
2. **Replace** (when Apple announces OpenGL removal, or once regular players exist): move that renderer to SDL_GPU (Metal, Vulkan, D3D12), with shaders built by SDL_shadercross.

Both must render BV2 data the same as before: compare screenshots of the same scenes with original data before merging. GL 3.3 core is not an option: on macOS it still runs on Apple's deprecated OpenGL.

- **[GUI]** HiDPI and ultrawide UI scaling, gamepad menus.
- Analyse rendering, menus, the map editor and `CMeshBuilder` with the `codebase-analysis` skill first; [../analysis/README.md](../analysis/README.md) lists them as not covered.

## G. Code modernisation (continuous)

- Replace raw `sprintf`/`strcpy`/`strcat` (90 call sites) with `snprintf` or `std::string` as files are touched.
- Ownership: raw `new`/`delete` of `Player`, `Projectile` and `CControl` → `std::unique_ptr` where lifetimes are clear.
- Internal `bv2` → `roboviolence2` renames under ground rule 3, as files are touched.
- Add unit tests alongside each fix in §A–C. Keep `ARCHITECTURE.md` current in every change.

## H. Replacement assets and rebranding

Source of truth: the asset inventory (kept outside this repository) (step 0 §0.3), with one row per removed file and its purpose, constraints and `Recreate` class.

1. **Minimum playable set:** every `required` row. Textures, skins, fonts, models (`.DKO` via an exporter for a free tool, see 4), effects and one map per game mode. Use CC0/CC-BY or original work only. Replacements use the BV2 formats and names so either set loads (ground rule 1); dropping BV2 compatibility afterwards needs an ADR.
2. **Record every replacement** in the inventory's `Replaced by` column and in the replacement register (kept outside this repository): author, licence, source URL. The hash blocklist (`tools/check-original-assets.py`) stays on permanently.
3. **Maps:** recreate the `content` rows with the in-game editor. New maps get new names, not copies of the originals.
4. **Toolchain:** `.DKO` models came from 3ds Max via `dkoExporter.dle` (removed). Write a Blender exporter, or convert from glTF in `tools/`, and record the choice as an ADR.
5. **Audio:** new effects, and new music to replace the third-party tracks.
6. **Fonts:** the replacement font covers Latin Extended at least, so languages with diacritics (Polish, Czech, German) render (§I.1).
7. **Rebrand** **[GUI]**: window title, menus, credits, binary names (`bv2*`), `BV2_*` env vars, config/pref-path names, master-server game ID. Keep a "based on BaboViolent 2 by bitHeads / RndLabs" credit. This breaks config paths, so migrate the old pref dir once.
8. **Grants:** if the original authors grant rights (the permission request (kept outside this repository)), record them in the licence notes. Re-adding any granted original needs an ADR and removing its hash from the blocklist.

## I. Localisation

Today the client loads one file, `languageFile` (default `main/languages/en.lang`, `game/src/GameVar.cpp:271`), set only through config. There is no in-game language picker (`game/src/OptionMenu.h` is a leftover header). Format and loading: [ADR 0011](../decisions/0011-gettext-po-translations.md).

1. **OS language by default.** When `languageFile` is empty (the new default), pick the language from `SDL_GetPreferredLocales()` (Windows, macOS, Linux): exact `ll_CC`, then `ll`, else English. A saved `languageFile` always wins. Auto-detection skips a language whose strings need glyphs the loaded font lacks (non-ASCII waits for §H.6), so a Polish system shows English until then; setting `languageFile` by hand still loads it.
2. **gettext PO files.** `content/languages/en.pot` is the template, generated from `en.lang` by a `tools/` script; translations are `<ll>[_<CC>].po` (`pl.po`, `pt_BR.po`), UTF-8. `msgctxt` is the existing key (`lang_mainMenu`), `msgid` the English text, `msgstr` the translation. The loader prefers `<locale>.po`, then `<locale>.lang`, and falls back to English per missing key. Original BV2 `.lang` files keep loading unchanged.
3. **Checks.** A `tools/` script, run by the `hygiene` CI job: valid PO syntax, no unknown keys, `en.pot` in sync with `en.lang`, missing keys reported per language.
4. **Translator guide** in `content/languages/README.md`: copy `en.pot` to `<ll>.po`, edit in Poedit or a hosted tool (Weblate), naming, glyph limits until §H.6.
5. **[GUI]** An in-game language picker, and moving hard-coded English strings (e.g. `game/src/GameShowStats.cpp:298`) into the language file where that changes the text shown.

## Deprecated, to be removed

Kept only for compatibility; remove once the trigger holds, each removal with an ADR.

| Item | Kept for | Remove when |
|---|---|---|
| `.lang` language files and their loader (`game/src/GameVar.cpp` `loadLanguage`) | Original BV2 data (ADR 0010, ADR 0011) | BV2 assets are no longer supported (after §H.1); `.po` becomes the only format and `en.lang` is replaced by `en.po` as the source for `en.pot` |
