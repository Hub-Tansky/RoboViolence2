# BaboViolent 2 — Codebase Analysis

Generated with the `codebase-analysis` skill (v2.1 methodology) on 2026-09-30.
Every claim carries a `[VERIFY: path:line]` tag relative to the repository root; all tags were checked automatically for file existence and line range, and a sample was checked by hand against the source.

## Reading order

| # | Document | What you get |
|---|---|---|
| 1 | [01_SYSTEM_OVERVIEW.md](01_SYSTEM_OVERVIEW.md) | Deliverables, build variants (`CONSOLE`, `_PRO_`), module map, lifecycle, 30 Hz fixed step |
| 2 | [02_DATA_STRUCTURES.md](02_DATA_STRUCTURES.md) | `CoordFrame`, `Player`, `Map` + `.bvm` format, `Weapon`, `Projectile`, packet quantisation |
| 3 | [03_DATA_FLOW.md](03_DATA_FLOW.md) | Join handshake, per-tick server/client pipelines, full message catalogue, external services |
| 4 | [ALGORITHM_01-Networking.md](ALGORITHM_01-Networking.md) | babonet batch framing, "RND1" hash, receive state machine, ping in frames |
| 5 | [ALGORITHM_02-Movement-Collision-Interpolation.md](ALGORITHM_02-Movement-Collision-Interpolation.md) | Client physics, grid collision, ray march, Bézier interpolation (`/90` derivation), speed-hack check |
| 6 | [ALGORITHM_03-Weapons-Hitscan-Projectiles.md](ALGORITHM_03-Weapons-Hitscan-Projectiles.md) | Weapon table, server-side hitscan, projectile physics, radial damage, `hitSV` |
| 7 | [ALGORITHM_04-GameModes-Spawning-Rounds.md](ALGORITHM_04-GameModes-Spawning-Rounds.md) | Round state machine, max-min spawn selection, CTF, auto-balance, map rotation, voting |
| 8 | [KEY_QUESTIONS.md](KEY_QUESTIONS.md) | Design Q&A, security findings, ranked defect register, relaunch fix order |
| 9 | [FORK_jmainguy-modern.md](FORK_jmainguy-modern.md) | Review of the Jmainguy `modern` fork: protocol impact, merge verdict, harvest list (implemented) |

`[VERIFY:]` line numbers describe the baseline commit `5eb7aad`. The harvest commits after it change the behaviour described in ALGORITHM_01 §6, ALGORITHM_02 §5 and 03_DATA_FLOW §1, which are updated. They also shift line numbers in the files they touch (see `git log 5eb7aad..`).

## Headline findings

1. **All traffic is TCP.** The server is created with `UDPenabled=false`, so packets tagged `NET_UDP` go over the reliable stream too (`03_DATA_FLOW` §5).
2. **Movement is client-authoritative**; the server checks only one reported velocity per 3 s window (`ALGORITHM_02` §5).
3. **Packets aren't bound to their sender.** Most handlers trust the `playerID` field, which is also never range-checked: impersonation and remote crash (`KEY_QUESTIONS` Q-S1, Q-S2).
4. **`if (gameVar.sv_serverType = 1)`** in projectile update silently forces Pro rules (`ALGORITHM_03` §5, W1).
5. **SQL injection** on the master server through the client-reported MAC address (`KEY_QUESTIONS` Q-S3).

## Analysis plan & coverage (Phase 1.5)

| Area | Covered in | Depth |
|---|---|---|
| Entry points, Scene, timer | 01 | full |
| Game state structs, map format | 02 | full |
| Protocol & handshake | 03 | full |
| babonet TCP path | ALGORITHM_01 | full (send/receive/hash) |
| babonet UDP / P2P (`cPeer2Peer`) | — | **not covered** |
| Physics, collision, interpolation | ALGORITHM_02 | full |
| Weapons, projectiles, damage | ALGORITHM_03 | full |
| Game modes, spawns, rotation, votes | ALGORITHM_04 | full (S&D is an empty stub in the code) |
| Rendering (`*Render.cpp`, `CMeshBuilder`, `dkgl`) | — | **not covered** |
| Menus / UI (`CControl`, `CMainTab`, …) | — | **not covered** |
| Map editor (`Editor*.cpp`) | — | **not covered** |
| Master server | 03, KEY_QUESTIONS | partial (ban path + SQL) |
| Launcher / RemoteAdmin / UpdateServer | — | **not covered** |

The skill targets 1,500–3,000 lines per algorithm document. These documents are deliberately shorter: they keep only verified, code-backed content and don't pad it out.
