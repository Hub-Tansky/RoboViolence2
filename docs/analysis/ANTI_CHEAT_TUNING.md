# Anti-cheat tuning (Phase B step 9)

Rules live in `ServerValidator` (`game/src/ServerValidator.cpp`); the handlers in `game/src/ServerRecv.cpp` call it. Movement stays client-authoritative, so every limit trades false positives for detection.

## Levels and score

| `sv_antiCheat` | Effect |
|---|---|
| `0` | Step 9 rules and the step 8 speed/frame kick off. Drops and the spawn weapon kick that existed before step 9 still apply. |
| `1` (default) | Breaks score and log; messages are applied unchanged. The step 8 speed/frame kick applies. |
| `2` | As `1`, plus a weapon or flood break drops its message; a private warning at `sv_antiCheatWarn` (5), a kick at `sv_antiCheatKick` (10). Movement breaks never drop `COORD_FRAME`s. |

- A rule scores 1 point at most once per rule and 3 s window, once its tolerance (breaks per window) is used up. One lag spike is one point.
- The score decays by 1 per 10 s and restarts for a new player in the slot. A cheat breaking one rule all the time reaches 10 in about 45 s.
- Log line: `> anti-cheat: <name> (<IP>) <rule>, score <n>`.
- Settings per rule (name, tolerance, drops, flood budget): `rules` in `ServerValidator.cpp`.

## Rules and limits

| Rule | Break | Tolerance | Function |
|---|---|---|---|
| fire rate | A shot or projectile past a budget of 2 s of fire (at least 1.5 s), refilled at the held weapon's rate; shotgun and sniper count each pellet message | 1 | `onShot` |
| shot with another weapon | `weaponID` is not the held weapon, except the weapon a pickup replaced in the last 1 s | 2 | `onShot` |
| shot after death | A shot or projectile more than 2 s after death | 2 | `onDeadShot` |
| grenade without ammo | `spawnProjectile` refuses a grenade or molotov | 1 | projectile handler |
| rocket without bazooka | A rocket from a player without a bazooka | 1 | projectile handler |
| early spawn | A spawn request while alive (unless the server sees the player in a wall cell) or with more than 1 s of the server's spawn timer left | 1 | `onSpawnRequest` |
| teleport | A jump longer than `3.75 * max(time since the last frame, send interval) + 1` units, no hit in the last 2 s | 0 | `onMove` |
| wall | A position in the inner half of a wall cell, or a path of at most 1 unit through one | 1 | `onMove` |
| path speed | A step 8 window's path longer than `3.75 * (max(server, client frames) / 30 + send interval) + 1.5` units, no hit in the window | 0 | `onCoordFrame` |
| chat, vote, name, skin, pong, team flood | A message past its budget | 0 | `budget` |

- Speed limit: the client clamp 3.25 plus 0.5. Send interval: the client sends a coord frame every `max(ping, sv_minSendInterval)` frames.
- The 1 and 1.5-unit slack absorbs lag and position quantisation: a speed up to about 4.25 units/s passes the path test.
- Movement state starts over on a spawn (`onSpawned`; `timeAlive` starting over for server-side spawns), on death and for a new player. A map change kills everyone, so the next spawn resets it.
- Budgets (burst, per second): chat 5, 1; votes and vote requests 3, 0.2; name 3, 0.1; skin 3, 0.1; pong 3, 2; team 3, 0.2.

## Tuning from a play-test

1. Run at `sv_antiCheat 1` and collect the server log.
2. A rule that logs honest players: raise its tolerance, slack or budget, smallest step first.
3. Re-run `ctest -R validator`: the negative tests (lagged and high-ping walking at the clamp, corner rounding, a stall's burst of fire, a pickup, shots up to 2 s after death, a chat line a second) must stay clean.
