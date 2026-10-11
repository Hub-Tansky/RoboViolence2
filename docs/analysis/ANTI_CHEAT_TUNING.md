# Anti-cheat tuning (Phase B step 9)

Rules live in `ServerValidator` (`game/src/ServerValidator.cpp`); the handlers in `game/src/ServerRecv.cpp` report breaks. Movement stays client-authoritative, so every limit trades false positives for detection.

## Levels and score

| `sv_antiCheat` | Effect |
|---|---|
| `0` | Step 9 rules off; step 8 speed/frame kick off. Drops and the spawn weapon kick that existed before step 9 still apply. |
| `1` (default) | Breaks score and log; messages are applied unchanged. The step 8 speed/frame kick applies. |
| `2` | As `1`, plus a weapon or flood break drops its message; a private warning at `sv_antiCheatWarn` (5), a kick at `sv_antiCheatKick` (10). Movement breaks never drop `COORD_FRAME`s. |

- A rule scores only past its tolerance: breaks per 3 s window that don't count (`ServerValidator.cpp:14`). Lag bunches packets, and a pickup races shots in flight.
- The score decays by 1 per 10 s (`ServerValidator.cpp:185`) and restarts for a new player in the slot.
- Log line: `> anti-cheat: <name> (<IP>) <rule>, score <n>`, once per rule and window.

## Rules and limits

| Rule | Break | Tolerance | Where |
|---|---|---|---|
| fire rate | A shot or projectile dropped by the existing fire-delay check | 5 | `ServerRecv.cpp` shoot and projectile handlers |
| shot with another weapon | `weaponID` is not the held weapon | 2 | shoot handler |
| shot after death | A shot or projectile more than 1 s after death | 2 | `ServerRecv.cpp:965`, `:1055` |
| grenade without ammo | `spawnProjectile` refuses a grenade or molotov | 1 | projectile handler |
| rocket without bazooka | A rocket from a player without a bazooka | 1 | projectile handler |
| early spawn | A spawn request while alive, or with more than 1 s of the server's spawn timer left | 1 | `ServerRecv.cpp:681` |
| teleport | A jump longer than `3.75 * dt + 1` units since the last frame (no hit in the last 2 s) | 0 | `ServerValidator.cpp:72` |
| wall | A frame-to-frame path through the inner half of a wall cell | 1 | `ServerValidator.cpp:47` |
| path speed | A 3 s window's path longer than `3.75 * t + 1.5` units (no hit in the window) | 0 | `ServerValidator.cpp:108` |
| chat, vote, name, skin, pong, team flood | A message past its budget | 0 | budgets `ServerValidator.cpp:16-17` |

- Speed limit: the client clamp 3.25 plus 0.5 (`ServerValidator.cpp:44`). The teleport and path slack (1 and 1.5 units) absorbs lag bursts and position quantisation, so a speed up to about 4.25 units/s passes the path test.
- Budgets (burst, per second): chat 5, 1; votes and vote requests 3, 0.2; name 3, 0.1; skin 3, 0.1; pong 3, 2; team 3, 0.2.

## Tuning from a play-test

1. Run at `sv_antiCheat 1` and collect the server log.
2. A rule that logs honest players: raise its tolerance, slack or budget, smallest step first.
3. Re-run `ctest -R validator`: the negative tests (lagged walking at the clamp, jittered fire, a chat line a second) must stay clean.
