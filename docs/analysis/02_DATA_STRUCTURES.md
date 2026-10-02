# 02 — Key Data Structures

> Every field list below was read from the header, not inferred. Tags are `[VERIFY: path:line]`.

---

## 1. Ownership graph

```
Scene
 ├── Server* server ──► Game* game (isServerGame = true)
 └── Client* client ──► Game* game (isServerGame = false)        (client builds only)

Game
 ├── Player** players            [MAX_PLAYER = 32 slots, index == playerID]
 │     ├── Weapon* weapon         (primary; per-player copy of a gameVar.weapons[] template)
 │     ├── Weapon* meleeWeapon    (secondary)
 │     ├── CoordFrame currentCF / lastCF / netCF0 / netCF1
 │     └── CMiniBot* minibot      (_PRO_ only)
 ├── Map* map
 │     ├── map_cell* cells        [size.x * size.y]
 │     ├── dm_spawns / blue_spawns / red_spawns (std::vector<CVector3f>)
 │     └── flagPodPos[2], flagPos[2], flagState[2], objective[2]
 ├── std::vector<Projectile*> projectiles
 └── SVoting voting
```

- `Scene` members: [VERIFY: BaboViolent2/Code/Scene.h:52-60]
- `Game::players` is `Player**` sized `MAX_PLAYER`: [VERIFY: BaboViolent2/Code/Game.h:443] [VERIFY: BaboViolent2/Code/Game.cpp:52-53]
- `MAX_PLAYER` is 32: [VERIFY: BaboViolent2/Code/Game.h:34]
- `Game::map`, `projectiles`: [VERIFY: BaboViolent2/Code/Game.h:465] [VERIFY: BaboViolent2/Code/Game.h:474]

---

## 2. `CoordFrame` — the unit of motion

[VERIFY: BaboViolent2/Code/Player.h:47-138]

```
struct CoordFrame {
    CVector3f position;       // world units: 1 unit == 1 map cell
    CVector3f vel;            // units / second
    long      frameID;        // sender's 30 Hz frame counter
    float     angle;          // yaw in degrees (derived from mouse)
    CVector3f mousePosOnMap;  // aim point in world space
    float     camPosZ;        // _PRO_ only: camera height (anti-zoom-hack)
    void interpolate(long& progress, CoordFrame& from, CoordFrame& to, float delay);
};
```

- `operator=` copies position, vel, angle, frameID, mousePosOnMap but **not** `camPosZ`: [VERIFY: BaboViolent2/Code/Player.h:69-76]
- `interpolate` is the remote-entity smoothing algorithm (see `ALGORITHM_02`): [VERIFY: BaboViolent2/Code/Player.h:78-137]

Every networked entity keeps **four** coord frames:

| Field | Meaning | Evidence |
|---|---|---|
| `currentCF` | What is simulated / rendered now | [VERIFY: BaboViolent2/Code/Player.h:312] |
| `lastCF` | Copy of `currentCF` at start of this tick (used as the "from" point for swept collision) | [VERIFY: BaboViolent2/Code/Player.h:313] [VERIFY: BaboViolent2/Code/PlayerUpdate.cpp:93] |
| `netCF0` | Start key of the interpolation curve | [VERIFY: BaboViolent2/Code/Player.h:314] |
| `netCF1` | Latest key received from the network | [VERIFY: BaboViolent2/Code/Player.h:315] |

---

## 3. `Player`

[VERIFY: BaboViolent2/Code/Player.h:209-494]

Grouped by concern (not all ~100 fields are listed):

| Group | Fields | Evidence |
|---|---|---|
| Identity | `name`, `playerIP[16]`, `babonetID` (network connection id), `playerID` (slot 0..31), `teamID`, `status`, `userID` (account) | [VERIFY: BaboViolent2/Code/Player.h:256-273] |
| Health | `life` (float, **1.0 = full health**), `protection` (shield timer), `immuneTime` | [VERIFY: BaboViolent2/Code/Player.h:275] [VERIFY: BaboViolent2/Code/Player.h:292] [VERIFY: BaboViolent2/Code/Player.h:344] |
| Stats | `dmg`, `kills`, `deaths`, `score`, `returns`, `damage`, `flagAttempts`, `timePlayedCurGame` | [VERIFY: BaboViolent2/Code/Player.h:295-306] |
| Ping | `ping`, `pingSum`, `avgPing`, `currentPingFrame`, `pingLog[60]`, `waitForPong` | [VERIFY: BaboViolent2/Code/Player.h:278-287] [VERIFY: BaboViolent2/Code/Player.h:324] |
| Anti-cheat | `frameSinceLast`, `lastFrame`, `currentFrame`, `speedHackCount`, `shotsPerSecond`, `shotCount`, `mfElapsedSinceLastShot` | [VERIFY: BaboViolent2/Code/Player.h:214-231] |
| Motion | the four `CoordFrame`s, `cFProgression`, `matrix` (visual roll) | [VERIFY: BaboViolent2/Code/Player.h:312-321] |
| Weapons | `weapon`, `meleeWeapon`, `nextSpawnWeapon`, `nextMeleeWeapon`, `nbGrenadeLeft`, `nbMolotovLeft`, `grenadeDelay`, `meleeDelay` | [VERIFY: BaboViolent2/Code/Player.h:350-357] [VERIFY: BaboViolent2/Code/Player.h:392-395] |
| Photon rifle trace | `incShot`, `p1`, `p2` | [VERIFY: BaboViolent2/Code/Player.h:426-428] |
| Cosmetics | `skin`, `redDecal`, `greenDecal`, `blueDecal` | [VERIFY: BaboViolent2/Code/Player.h:414-420] |

Enumerations:

```
status : PLAYER_STATUS_ALIVE 0 | DEAD 1 | LOADING 2
teamID : PLAYER_TEAM_SPECTATOR -1 | BLUE 0 | RED 1 | AUTO_ASSIGN 2
```
[VERIFY: BaboViolent2/Code/Player.h:33-40]

> **Two IDs, don't mix them up.** `playerID` is the array slot (0..31) and is what every game packet carries. `babonetID` is the connection handle that `bb_serverSend` needs. The server maps a disconnecting `babonetID` back to a slot by linear search.
> [VERIFY: BaboViolent2/Code/Server.cpp:487-491]

`PlayerStats` is a snapshot kept for disconnected players (for end-of-round reports): [VERIFY: BaboViolent2/Code/Player.h:141-157]

---

## 4. `Map` and `map_cell`

### 4.1 Cell grid

```
struct map_cell {
    bool  passable;      // false = wall
    float splater[4];    // per-corner "dirt" blend weights (render + ice/lava detection)
    int   height;        // wall height, default 1
    unsigned dl;         // GL display list (client only)
};
```
[VERIFY: BaboViolent2/Code/Map.h:117-153]

- Row-major: cell `(x, y)` is `cells[y * size[0] + x]`: [VERIFY: BaboViolent2/Code/Map.h:349]
- 1 world unit = 1 cell; babo radius is 0.25 (see collision calls with `.25f`): [VERIFY: BaboViolent2/Code/Game.cpp:630]
- `splater[0] > .5` under a player is how the game detects "ice" (snow theme) or "lava" tiles: [VERIFY: BaboViolent2/Code/PlayerUpdate.cpp:258-262] [VERIFY: BaboViolent2/Code/Game.cpp:713-722]

### 4.2 Map object

[VERIFY: BaboViolent2/Code/Map.h:172-530]

| Field | Meaning | Evidence |
|---|---|---|
| `size` | grid dimensions | [VERIFY: BaboViolent2/Code/Map.h:176] |
| `cells` | grid storage | [VERIFY: BaboViolent2/Code/Map.h:182] |
| `flagPodPos[2]` | CTF bases (index 0 = blue, 1 = red) | [VERIFY: BaboViolent2/Code/Map.h:254] [VERIFY: BaboViolent2/Code/Map.h:103-104] |
| `flagPos[2]`, `flagState[2]` | current flag position; `-2` on pod, `-1` on ground, `>=0` = carried by that playerID | [VERIFY: BaboViolent2/Code/Map.h:268-272] |
| `objective[2]` | S&D bomb sites | [VERIFY: BaboViolent2/Code/Map.h:275] |
| `dm_spawns`, `blue_spawns`, `red_spawns` | spawn points | [VERIFY: BaboViolent2/Code/Map.h:278-280] |
| `dko_map`, `dko_mapLM` | optional 3D model map; when present, collision/raycasts go through `dko` instead of the grid | [VERIFY: BaboViolent2/Code/Map.h:221] [VERIFY: BaboViolent2/Code/Map.h:243] [VERIFY: BaboViolent2/Code/MapRender.cpp:427-457] |

### 4.3 `.bvm` file format

Loaded in `Map::Map` through `FileIO` (server/editor read from disk; a joining client reads from the downloaded `mapBuffer`):
[VERIFY: BaboViolent2/Code/Map.cpp:149-163]

```
u32  version                       ─┐ 10010 | 10011 | 20201 | 20202 (MAP_VERSION)
[20202 only] char author[25]         │
[20201+]     i32 theme, i32 weather  │
i32  width, i32 height               │
repeat width*height (row-major):     │
   u8 cellByte  → passable = bit7, height = bits0-6
   u8 dirt      → setTileDirt(x, y, dirt/255)
[10011, 20201] vec3 flagPod[2], vec3 objective[2],
               i32 n + vec3[n] dm, i32 n + vec3[n] blue, i32 n + vec3[n] red
[20202]        i32 n + vec3[n] dm, then GAME_TYPE_COUNT sections:
               i32 id; CTF → vec3 flagPod[2]; SND → vec3 obj[2] + blue/red spawn lists
```

- Version switch: [VERIFY: BaboViolent2/Code/Map.cpp:214-216]
- Cell byte decoding: [VERIFY: BaboViolent2/Code/Map.cpp:376-380]
- v20202 header (author, theme, weather): [VERIFY: BaboViolent2/Code/Map.cpp:348-361]
- v20202 per-gametype sections: [VERIFY: BaboViolent2/Code/Map.cpp:392-429]
- `MAP_VERSION 20202`: [VERIFY: BaboViolent2/Code/Map.h:101]
- Names are truncated to 15 characters: [VERIFY: BaboViolent2/Code/Map.cpp:102]
- Optional 3D map: `main/modelmaps_______/<name>/<name>.DKO` plus `LM.DKO` for collision: [VERIFY: BaboViolent2/Code/Map.cpp:437-458]

`IsMapValid` decides whether a map can host a game type (DM/TDM need ≥1 dm spawn; CTF also needs both flag pods non-zero): [VERIFY: BaboViolent2/Code/Map.cpp:1737-1769]

---

## 5. `Weapon`

[VERIFY: BaboViolent2/Code/Weapon.h:82-184]

| Field | Meaning | Evidence |
|---|---|---|
| `fireDelay` / `currentFireDelay` | cooldown between shots | [VERIFY: BaboViolent2/Code/Weapon.h:110-111] |
| `damage` | fraction of full life (life is 1.0) | [VERIFY: BaboViolent2/Code/Weapon.h:129] |
| `impressision`, `startImp`, `currentImp` | spread in degrees; grows +3 per shot up to `impressision` | [VERIFY: BaboViolent2/Code/Weapon.h:132-134] [VERIFY: BaboViolent2/Code/Game.cpp:1281-1283] |
| `nbShot` | rays per trigger pull | [VERIFY: BaboViolent2/Code/Weapon.h:137] |
| `reculVel` | recoil impulse applied to the shooter | [VERIFY: BaboViolent2/Code/Weapon.h:140] [VERIFY: BaboViolent2/Code/Weapon.cpp:381] |
| `projectileType` | `PROJECTILE_DIRECT` (hitscan) or a projectile kind | [VERIFY: BaboViolent2/Code/Weapon.h:149] |

Projectile type IDs: [VERIFY: BaboViolent2/Code/Weapon.h:34-44]

```
DIRECT 1  ROCKET 2  GRENADE 3  LIFE_PACK 4  DROPED_WEAPON 5  DROPED_GRENADE 6
COCKTAIL_MOLOTOV 7  FLAME 8  GIB 9  NONE 10  PHOTON 11
```

---

## 6. `Projectile`

[VERIFY: BaboViolent2/Code/Game.h:258-328]

| Field | Meaning | Evidence |
|---|---|---|
| `projectileType` | see table above | [VERIFY: BaboViolent2/Code/Game.h:261] |
| `currentCF`, `lastCF`, `netCF0`, `netCF1` | same scheme as players | [VERIFY: BaboViolent2/Code/Game.h:264-267] |
| `remoteEntity` | `true` on clients (mirror), `false` on the server (simulated + authoritative) | [VERIFY: BaboViolent2/Code/Game.h:273] [VERIFY: BaboViolent2/Code/GameSpawn.cpp:472] [VERIFY: BaboViolent2/Code/GameSpawn.cpp:521] |
| `fromID` | owner playerID — **overloaded**: for `DROPED_WEAPON` it stores the *weaponID* | [VERIFY: BaboViolent2/Code/Game.h:293] [VERIFY: BaboViolent2/Code/GameSpawn.cpp:460] [VERIFY: BaboViolent2/Code/ServerRecv.cpp:1141] |
| `duration` | lifetime in seconds | [VERIFY: BaboViolent2/Code/Game.h:290] |
| `projectileID` | current index in `Game::projectiles` (rewritten every tick) | [VERIFY: BaboViolent2/Code/Game.h:310] [VERIFY: BaboViolent2/Code/Game.cpp:823] |
| `uniqueID` | monotonically increasing id assigned by the server (static counter) | [VERIFY: BaboViolent2/Code/Game.h:313-314] [VERIFY: BaboViolent2/Code/GameSpawn.cpp:437-438] |
| `needToBeDeleted` / `reallyNeedToBeDeleted` | two-phase delete (one extra tick "to prevent invisi flame bug") | [VERIFY: BaboViolent2/Code/Game.h:276-279] [VERIFY: BaboViolent2/Code/Game.cpp:824-837] |

---

## 7. `Game` round / mode state

[VERIFY: BaboViolent2/Code/Game.h:500-536]

- Game types `DM 0, TDM 1, CTF 2, SND 3`: [VERIFY: BaboViolent2/Code/Game.h:38-42]
- `roundState` values `GAME_PLAYING -1, BLUE_WIN 0, RED_WIN 1, DRAW 2, DONT_SHOW 3, MAP_CHANGE 4`: [VERIFY: BaboViolent2/Code/Game.h:50-55]
- Scores: `blueScore/redScore` (TDM kills) and `blueWin/redWin` (CTF captures); in CTF the capture code keeps both equal: [VERIFY: BaboViolent2/Code/ServerCTF.cpp:80-81]
- Timers: `gameTimeLeft`, `roundTimeLeft` (seconds): [VERIFY: BaboViolent2/Code/Game.h:510-511]

### `SVoting`

[VERIFY: BaboViolent2/Code/Game.h:542-616]

- `castVote` snapshots the eligible voters (players on a team) into `activePlayersID` (as `babonetID`s) and gives 30 s: [VERIFY: BaboViolent2/Code/Game.h:560-592]
- `update` ends the vote on timeout, on a strict majority either way, or when everyone voted: [VERIFY: BaboViolent2/Code/Game.h:593-615]
- Pass rule on the server: `yes > floor(voters / 2)`: [VERIFY: BaboViolent2/Code/Game.cpp:373]

---

## 8. `Server` bookkeeping

[VERIFY: BaboViolent2/Code/Server.h:58-231]

| Field | Purpose | Evidence |
|---|---|---|
| `mapList`, `mapInfoList` | rotation list + per-map area and "last played" counter | [VERIFY: BaboViolent2/Code/Server.h:87-90] |
| `banList` | `(name, ip)` pairs, persisted to `main/banlist` as 32 + 16 byte records | [VERIFY: BaboViolent2/Code/Server.h:93] [VERIFY: BaboViolent2/Code/Server.cpp:64-79] |
| `CachedPlayers[50]` | ring buffer of recent (nick, IP, MAC) | [VERIFY: BaboViolent2/Code/Server.h:96-97] |
| `mapTransfers` | in-flight map downloads (client id, map, chunk index) | [VERIFY: BaboViolent2/Code/Server.h:100-106] |
| `authRequests` | pending HTTP account-auth requests (`CCurl`) | [VERIFY: BaboViolent2/Code/Server.h:112] |
| `delayedKicks` | `_PRO_` delayed kicks, keyed by babonetID | [VERIFY: BaboViolent2/Code/Server.h:211-228] |

---

## 9. Network packet structs

All game messages are raw C structs `memcpy`'d on and off the wire, declared in `netPacket.h`. They depend on the compiler's struct layout and on host byte order (little-endian). Nothing is serialised field by field.
[VERIFY: BaboViolent2/Code/ServerRecv.cpp:47-48]

Quantisation conventions (used consistently by sender and receiver):

| Quantity | Wire type | Scale | Evidence |
|---|---|---|---|
| position | `short[3]` | ×100 (1 cm) | [VERIFY: BaboViolent2/Code/PlayerUpdate.cpp:299-301] [VERIFY: BaboViolent2/Code/Player.cpp:1530-1532] |
| velocity | `char[3]` | ×10, so max ≈ ±12.7 u/s | [VERIFY: BaboViolent2/Code/PlayerUpdate.cpp:302-304] |
| aim point | `short[3]` | ×100 | [VERIFY: BaboViolent2/Code/PlayerUpdate.cpp:305-307] |
| hit normal | `char[3]` | ×120 | [VERIFY: BaboViolent2/Code/Game.cpp:1526-1528] |
| spawn position | `short[3]` | ×**10** (note: not 100) | [VERIFY: BaboViolent2/Code/ServerRecv.cpp:732-734] |

Range limit: `short` × 100 caps coordinates at 327.67, so maps larger than ~327 cells would overflow positions on the wire.

Message ID ranges: `1-8` client→server, `101-135` server→client, `201-212` both directions, `301-302` LAN broadcast, `404-405` and `1001-1002` Pro-only.
[VERIFY: BaboViolent2/Code/netPacket.h:28] [VERIFY: BaboViolent2/Code/netPacket.h:107] [VERIFY: BaboViolent2/Code/netPacket.h:434] [VERIFY: BaboViolent2/Code/netPacket.h:581-582] [VERIFY: BaboViolent2/Code/netPacket.h:600] [VERIFY: BaboViolent2/Code/netPacket.h:480]

The full catalogue lives in `03_DATA_FLOW.md` §3.

---

## 10. babonet internals

| Struct | Purpose | Evidence |
|---|---|---|
| `stHeader {u16 Size; u16 typeID}` | per-message header inside a TCP batch | [VERIFY: Engine/babonet/Code/cPacket.h:43-47] |
| `cPacket` | queued message (doubly linked list) | [VERIFY: Engine/babonet/Code/cPacket.h:51-92] |
| `cClient` | one connection: send queues, receive state machine, packet-ID counters | [VERIFY: Engine/babonet/Code/cClient.h:40-160] |
| `cServer` | listener + client list; `UDPenabled` flag | [VERIFY: Engine/babonet/Code/cServer.h:35-112] |
| `KEY_SIZE 9`, `RND_KEY "RND1"` | batch prefix: 4-byte key + 4-byte hashed sequence + 1-byte count | [VERIFY: Engine/babonet/Code/cClient.h:36-37] |

See `ALGORITHM_01-Networking.md`.
