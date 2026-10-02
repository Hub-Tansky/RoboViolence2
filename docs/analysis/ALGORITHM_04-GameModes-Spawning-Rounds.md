# ALGORITHM 04 — Game Modes, Spawning, Rounds, Voting and Rotation

---

## 1. Round state machine (server)

```
           ┌────────────────────────── win condition met ──────────────────────────┐
           │                                                                       ▼
   ┌───────────────┐                                                ┌────────────────────────────┐
   │ GAME_PLAYING  │                                                │ BLUE_WIN / RED_WIN / DRAW  │
   │     (-1)      │◄──── changeMapDelay hits 0: createMap,          │ or DONT_SHOW (time limit)  │
   └───────────────┘      MAP_CHANGE + GAME_STATE(reInit=true) ──────┤ changeMapDelay = 10 s      │
           ▲                                                         │ nextMap = queryNextMap()   │
           │  admin "changemap": roundState = GAME_MAP_CHANGE (4),   └────────────────────────────┘
           └─ changeMapDelay = 10 ───────────────────────────────────────────────────────────────
```

- Win checks run only while `roundState == GAME_PLAYING`: [VERIFY: BaboViolent2/Code/Server.cpp:823]
- Round end → report upload, stats cache clear, 10 s delay, next map, broadcast: [VERIFY: BaboViolent2/Code/Server.cpp:963-994]
- Delay expiry → `resetGameType`, load map (skipping invalid maps), broadcast: [VERIFY: BaboViolent2/Code/Server.cpp:753-798]
- Admin map change path: [VERIFY: BaboViolent2/Code/Server.cpp:200-234]
- While not `GAME_PLAYING`, players aren't updated and spawn requests are refused: [VERIFY: BaboViolent2/Code/Game.cpp:388] [VERIFY: BaboViolent2/Code/ServerRecv.cpp:670]

### Win conditions per mode

| Mode | Condition → state | Evidence |
|---|---|---|
| DM | time up (if limited) or any player `score ≥ sv_scoreLimit` → `DONT_SHOW` | [VERIFY: BaboViolent2/Code/Server.cpp:828-849] |
| TDM | tie at/over limit → DRAW; blue/red ≥ `sv_scoreLimit` → WIN; time up → `DONT_SHOW` | [VERIFY: BaboViolent2/Code/Server.cpp:852-876] |
| CTF | `blueWin/redWin ≥ sv_winLimit`; on time-up, compare captures | [VERIFY: BaboViolent2/Code/Server.cpp:879-917] |
| SND | same as CTF on `blueWin/redWin`; bomb logic is **empty** (`updateSnD` has no body) | [VERIFY: BaboViolent2/Code/Server.cpp:920-961] [VERIFY: BaboViolent2/Code/ServerSnD.cpp:24-26] |

Note the TDM time-up branch runs after the score branch and overwrites a WIN set in the same tick with `DONT_SHOW`: [VERIFY: BaboViolent2/Code/Server.cpp:871-875]

In `_PRO_` builds, game type 3 is repurposed ("Champion") instead of Search & Destroy: every `sv_roundTimeLimit` all players are force-killed (hit with −100) and spawn slots are reshuffled.
[VERIFY: BaboViolent2/Code/Server.cpp:1294-1331] [VERIFY: BaboViolent2/Code/GameVar.cpp:375]

---

## 2. Spawn selection (`Game::spawnPlayer`)

[VERIFY: BaboViolent2/Code/GameSpawn.cpp:144-324]

Only team players (blue/red) spawn. The algorithm is **max-min distance**:

$$s^\* = \arg\max_{s \in S}\ \min_{j \in E} \lVert s - p_j \rVert^2$$

where `S` = `dm_spawns`, and `E` = all other alive players (DM) or alive **enemies** (TDM/CTF).

```
best = 0; bestScore = 0
for each spawn s:
    nearest = min over E of distanceSquared(s, p_j)    (100000 if none)
    if nearest > bestScore: bestScore = nearest; best = s
    if |E| == 0: best = random spawn; break
spawn at (s.x, s.y, 0.25)
```
- DM: [VERIFY: BaboViolent2/Code/GameSpawn.cpp:208-247]
- TDM / CTF (enemies only): [VERIFY: BaboViolent2/Code/GameSpawn.cpp:251-303]
- Pro CTF "ladder" spawn: first 10 s of the game, spawn on your own flag pod: [VERIFY: BaboViolent2/Code/GameSpawn.cpp:287-296]

Complexity: `O(|S| · P)`.

Observations from the code:
- CTF/TDM use the shared `dm_spawns` list, not `blue_spawns/red_spawns` (those are only used by S&D validation): [VERIFY: BaboViolent2/Code/GameSpawn.cpp:255-257] [VERIFY: BaboViolent2/Code/Map.cpp:1762-1764]
- In non-Pro builds there is **no SND branch**, so `spawnPlayer` returns false for game type 3: [VERIFY: BaboViolent2/Code/GameSpawn.cpp:152-205]
- Pro SND slot formula `bestFound*5 + rand(0,5)` is bounds-checked with `>` instead of `>=`, allowing `spawnLocation == size()` (one past the end): [VERIFY: BaboViolent2/Code/GameSpawn.cpp:193-198]

### Spawn request flow

1. Client: dead, team player, `timeToSpawn` elapsed, and (shoot pressed or `sv_forceRespawn`) → `SPAWN_REQUEST`: [VERIFY: BaboViolent2/Code/PlayerUpdate.cpp:388-445]
2. Server: optional weapon whitelist (`sv_validateWeapons`, kick on violation), minibot gate, `spawnPlayer`, broadcast `PLAYER_SPAWN`: [VERIFY: BaboViolent2/Code/ServerRecv.cpp:667-776]

---

## 3. CTF (`Server::updateCTF`)

[VERIFY: BaboViolent2/Code/ServerCTF.cpp:24-262]

Flag state encoding: `-2` on pod, `-1` dropped, `≥0` carrier's playerID ([VERIFY: BaboViolent2/Code/Map.h:272]). Index 0 = blue flag, 1 = red flag.

| Event | Condition | Radius | Evidence |
|---|---|---|---|
| Take enemy flag from pod | enemy within r of pod | 0.25 | [VERIFY: BaboViolent2/Code/ServerCTF.cpp:34-55] |
| Capture | carrier of enemy flag reaches own pod **while own flag is on its pod** | 0.25 | [VERIFY: BaboViolent2/Code/ServerCTF.cpp:57-84] |
| Pick up dropped enemy flag | enemy within r | 0.5 (blue flag) / **0.25 (red flag)** | [VERIFY: BaboViolent2/Code/ServerCTF.cpp:97-99] [VERIFY: BaboViolent2/Code/ServerCTF.cpp:215-217] |
| Return own dropped flag | teammate within r | 0.5 (blue) / 0.25 (red) | [VERIFY: BaboViolent2/Code/ServerCTF.cpp:120-122] [VERIFY: BaboViolent2/Code/ServerCTF.cpp:238-240] |

Capture scoring: `score++`, team `Win++`, `Score = Win`: [VERIFY: BaboViolent2/Code/ServerCTF.cpp:79-81]

Defects:
- **Asymmetric radii**: the dropped blue flag uses 0.5, the dropped red flag 0.25, so the red flag is harder to pick up / return (probably a copy-paste slip).
- Log message for returning the red flag says "returned the blue flag": [VERIFY: BaboViolent2/Code/ServerCTF.cpp:253]
- Evaluation order within one tick favours lower player IDs (`break` after the first event per flag state).

---

## 4. Team assignment and auto-balance

### `Game::assignPlayerTeam`

[VERIFY: BaboViolent2/Code/Game.cpp:880-983]

- AUTO_ASSIGN: smaller team, then losing team, then coin flip: [VERIFY: BaboViolent2/Code/Game.cpp:888-935]
- Approval gate (`isApproved`): unapproved → spectator: [VERIFY: BaboViolent2/Code/Game.cpp:938-939]
- Actual change kills the player and resets the spawn timer: [VERIFY: BaboViolent2/Code/Game.cpp:941-948]

### `Server::autoBalance`

Triggered for TDM/CTF when `|reds − blues| ≥ 2`: first a warning broadcast, then after `sv_autoBalanceTime` seconds the balance runs.
[VERIFY: BaboViolent2/Code/Server.cpp:1231-1275]

Intended algorithm: move `nbToSwitch` players with the least `timePlayedCurGame` from the big team, never the flag carrier.
[VERIFY: BaboViolent2/Code/Server.cpp:1416-1495]

What the code actually does:

1. `nbToSwitch = big − 1 − small` ([VERIFY: BaboViolent2/Code/Server.cpp:1440]). The correct count is `⌊(big − small)/2⌋`. For `6 vs 2` it computes 3, which would produce `3 vs 5`.
2. Inside the loop, the moved player is never removed from `blues`/`reds`, so each iteration re-selects **the same** player; `assignPlayerTeam` is then a no-op because they're already on the target team ([VERIFY: BaboViolent2/Code/Server.cpp:1444-1461] [VERIFY: BaboViolent2/Code/Game.cpp:941]).

Net effect: exactly **one** player moves per balance cycle (bug 2 masks bug 1), and the `TEAM_REQUEST` for that player is broadcast `nbToSwitch` times. The periodic check then fires again, so large imbalances converge one player per `sv_autoBalanceTime`.

3. The broadcast always announces the requested team, even if `assignPlayerTeam` returned SPECTATOR due to approval rules → clients and server can disagree on that player's team: [VERIFY: BaboViolent2/Code/Server.cpp:1454-1460]

---

## 5. Map rotation (`Server::queryNextMap`)

[VERIFY: BaboViolent2/Code/Server.cpp:363-433]

```
for maps in mapList not yet in mapInfoList:            # lazy discovery
    load map, count passable interior cells → mapArea
    mapInfoList += {name, area, lastPlayed = 1e9}
restore the current map (reload!)
for each map m:
    m.lastPlayed = 1 if current else m.lastPlayed + 1
    if filterMapFromRotation(m): candidate
next = candidate with max lastPlayed    (least recently played that fits)
fallback: random map
```

Size filter:
[VERIFY: BaboViolent2/Code/Server.cpp:1394-1411]

$$\text{tilesPerBabo} = \frac{\text{mapArea}}{\max(2, n_{\text{team players}})},\qquad \text{sv\_minTilesPerBabo} \le \text{tilesPerBabo} \le \text{sv\_maxTilesPerBabo}$$

(`sv_maxTilesPerBabo = 0` disables the upper bound.) So rotation adapts map size to the current population.

Defects:
- Discovering new maps **reloads the current map** (`game->createMap()`), mid-game, when `addmap` was used; the code comment acknowledges it: [VERIFY: BaboViolent2/Code/Server.cpp:368] [VERIFY: BaboViolent2/Code/Server.cpp:383-387]
- Fallback `rand(0, size-1)` on an empty `mapInfoList` indexes element 0 of an empty vector: [VERIFY: BaboViolent2/Code/Server.cpp:421-427]
- Lines after the if/else are unreachable: [VERIFY: BaboViolent2/Code/Server.cpp:428-432]

---

## 6. Voting

1. `VOTE_REQUEST`: rejected if a vote is running or the command isn't in `voteList` (with `set sv_…` normalised to the variable name): [VERIFY: BaboViolent2/Code/ServerRecv.cpp:94-138] [VERIFY: BaboViolent2/Code/Server.cpp:123-143]
2. `castVote`: eligible voters = players on a team; 30 s timer: [VERIFY: BaboViolent2/Code/Game.h:560-592]
3. `VOTE`: accepted once per eligible voter (checked via `babonetID` membership): [VERIFY: BaboViolent2/Code/ServerRecv.cpp:60-93]
4. Resolution: passes iff `yes > ⌊n/2⌋`; on pass, the server executes the command string as a console command: [VERIFY: BaboViolent2/Code/Game.cpp:355-386]
5. Kick votes are cancelled if the target disconnects first: [VERIFY: BaboViolent2/Code/Server.cpp:497-521]

Note: step 3 checks eligibility via `babonetID` but takes the *voter's slot* from the packet's `playerID`, so one connection can cast votes on behalf of other eligible slots (see `KEY_QUESTIONS.md`, Q-S1).

---

## 7. Anti-idle, max-ping, join message

- Idle > `sv_autoSpectateIdleMaxTime` while on a team → moved to spectator: [VERIFY: BaboViolent2/Code/Server.cpp:1100-1109]
- Idle timer resets on every accepted coord frame from an alive player: [VERIFY: BaboViolent2/Code/ServerRecv.cpp:800]
- Join message sent once, ~1 s after connecting: [VERIFY: BaboViolent2/Code/Server.cpp:1110-1116]
