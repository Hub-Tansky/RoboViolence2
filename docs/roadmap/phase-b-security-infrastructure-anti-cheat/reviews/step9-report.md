SKILL: Launching skill: thermo-nuclear-code-quality-review

Review of `git diff 5532f85...3f23b5d` (PR #39, Phase B step 9). Build was up to date, all 13 ctests pass, and `tools/review.sh` passes. Verdict: **not approvable**. There is one remotely triggerable server crash (confirmed), several lag false positives that break the "honest players log nothing" acceptance check, and a structural flaw behind the crash.

## Findings

**1. CRITICAL: a movement-rule kick at `sv_antiCheat 2` crashes the server (null dereference). Confirmed.**
- Where: `game/src/ServerValidator.cpp:72-74,90,110` with `game/src/ServerRecv.cpp:782-790`.
- What happens: on a TELEPORT, WALL or PATH_SPEED break, `violation()` calls `scene->kick(slot)`. That deletes the Player and nulls the slot (`SceneNet.cpp:243`). `onCoordFrame` then returns `KICK`. The coord-frame handler reads `KICK` as "you must kick" (the step 8 meaning) and dereferences `game->players[id]->name` and `->babonetID`, which are now null.
- How I confirmed it: a scratch probe outside the repo, linked against the built libraries, ran `set sv_antiCheat 2`, spawned a player and sent alternating 5-unit jumps. The score rose to 9.97, then the server got SIGSEGV in `CString::len(this=0x70)`, which is `players[id]->name` on a null Player.
- Impact: any client can crash a level-2 server with about 12 coord frames.
- Fix (see 2): make `KICK` mean one thing.
  - Let the validator perform every kick itself through `scene->kick`, including the step 8 speed/frame kick.
  - Replace the handler's hand-rolled kick block at `ServerRecv.cpp:783-789`, which duplicates `Scene::kick`, with a plain `return`.
- Test to add: at level 2, send teleport frames until kicked; check the slot is null and the server keeps ticking. Today the only level-2 kick test calls `violation()` directly (`tests/test_validator.cpp:214-216`), never through a handler.

**2. HIGH (structure): `Verdict::KICK` means two different things.**
- Where: `ServerValidator.h:10,33-37`.
- Step 8 `KICK` means "the handler must kick". Step 9 `KICK` means "already kicked, the Player is deleted". Both come out of the same function (`onCoordFrame`), which is the root cause of finding 1.
- The simplification: the validator owns the kick, `KICK` always means "gone", and the duplicated disconnect code in `ServerRecv.cpp` goes away. One contract, fewer lines.

**3. HIGH (false positives): one lag event scores once per packet, so a single stall can push a laggy player toward a kick.**
- Where: `ServerValidator.cpp:151-153`.
- Every break past the tolerance adds 1, even though the log line is written only once per rule and window.
- Example, FIRE_RATE: an honest SMG or chain gun user firing through a 1 s stall gets ~10 reliable shots in one tick. The existing check drops 9 of them, which means 9 breaks, tolerance 5, so +4.
- Example, SHOT_DEAD: a stall longer than 1 s across a death adds +8 at once (`ServerRecv.cpp:965`).
- At level 2, two or three such spikes inside the 10 s/point decay reach `sv_antiCheatKick` 10.
- At level 1 every spike is logged, which fails the acceptance check "10-minute play-test logs no violations".
- The fire-rate negative test (`test_validator.cpp:257-262`) only covers 0.067–0.133 s jitter, never a stall.
- Fix: score at most 1 per rule per window (`s += 1` only when `breaks == tolerance + 1`, the same place the log is written).
- Code-judo option: turn FIRE_RATE into a `budget()` token bucket (refill `1/fireDelay`, burst about 1–2 s of fire). Bursts then stop scoring by construction.
- Add stall tests: 10 shots in one tick, 10 shots after death.

**4. MEDIUM (false positive): TELEPORT and WALL misfire for high-ping players.**
- Where: `ServerValidator.cpp:72-73,47-61`.
- Why: clients send coord frames every `max(avgPing, sv_minSendInterval)` frames (`PlayerUpdate.cpp:287`). At RTT ≥ 300 ms the frames are ≥ 1 unit apart.
- TELEPORT (tolerance 0): its limit is `3.75·Δserver + 1`. Jitter above ~0.3 s that brings two frames in at nearly the same tick gives `d > 1` with `Δ ≈ 0`, so it scores and logs.
- WALL: the chord between two samples taken around an L-shaped wall corner enters the wall cell's inner half once samples are ≥ ~1.4 units apart. That is RTT ≈ 430 ms; two corners in 3 s score.
- The negative test `walk(c, ..., 3.25f, 4, 3, 4)` (`test_validator.cpp:382`) only models one frame per 3 client frames (≈100 ms RTT) on a straight line with no corners.
- Fix:
  - Allow for the client's send interval (for example a slack of `maxSpeed · max(Δserver, interval)` plus 1, or compare against the previous two frames).
  - Run the chord wall check only for short segments (≤ ~1 unit), plus an endpoint-inside-wall test.
  - Add negative tests: `every=10, burst=2`, and a corner walk past a one-cell wall.

**5. MEDIUM (false positive): PATH_SPEED fires after a ~0.9 s stall at the window boundary.**
- Where: `ServerValidator.cpp:105-110`.
- Frames held back from the end of the previous window arrive in the new one. The window's path then covers `3 + L` seconds of movement against 3 s of server ticks.
- Honest straight running: `3.25·(3+L) > 12.75` once L > 0.92 s. Tolerance is 0, so it scores and logs.
- Fix: give PATH_SPEED a tolerance of 1, or time the window by the received frames' `frameID` span, capped by server time plus slack.

**6. MEDIUM (false positive): a respawn is not reset reliably.**
- Where: `ServerValidator.cpp:69`, with spec 9.3 "Spawns, deaths … reset state".
- Track resets only when `player.timeAlive < t.timeAlive`, or when a coord frame arrives while dead (`ServerRecv.cpp:850`).
- A player killed within ~RTT of spawning (spawn-kill by grenade or explosion; explosions don't set `framesSinceKnockback`), with no in-flight frame after death, keeps a small `t.timeAlive`. The first frame after respawn then counts as a teleport from the death spot (tolerance 0).
- Fix: call `validator.reset(*player)` on the successful spawn in the `NET_CLSV_SPAWN_REQUEST` handler (`ServerRecv.cpp:702`) and drop the `timeAlive` heuristic.
- Related: detecting a map change by comparing `Map*` (`:69`) is an ABA problem, since a new Map can get the freed address. Resetting on spawn covers map changes too, because `Game.cpp:236` kills everyone.

**7. MEDIUM (false positive): SHOT_WEAPON races a pickup.**
- Where: `ServerRecv.cpp:892`.
- The server swaps the weapon at pickup. The client keeps firing the old weapon for about one RTT, which at 10 shots/s is 3+ shots at RTT ≥ 300 ms. That is more than the tolerance of 2, so it scores; at level 2 the shots are also dropped.
- There is no negative test (a pickup while firing).
- Fix: remember the previous weapon and the time of the swap, and skip shots carrying the previous weapon for ~1 s after it.

**8. MEDIUM (structure): rule logic is split between the handlers and the validator, with magic thresholds.**
- Predicates live inline in `ServerRecv.cpp:681` (`ALIVE || timeToSpawn > 1.0f`), `:892`, `:965` and `:1055` (`timeDead > 1.0f`, duplicated).
- The movement predicates live in the validator. The tuning doc then has to cite handler lines that will drift.
- Fix: add `validator.onShot(sender, weaponID)`, `onDeadFire(sender)` and `onSpawnRequest(sender)` returning a Verdict, so every threshold sits in `ServerValidator.cpp` next to the tolerance table.
- `ServerRecv.cpp` was already over 1k lines (1122 → 1145). Not newly crossed, but it should not keep growing.

**9. MEDIUM (structure): parallel arrays and enum-order-dependent tables.**
- Where: `ServerValidator.cpp:10-17,169` and `ServerValidator.h:46-60`.
- `ruleName`, `tolerance`, `burst` and `refill` are positional arrays that must match the `Rule` order.
- `FLOODS = TELEPORT - FLOOD_CHAT`, `flood - FLOOD_CHAT` and `rule < TELEPORT ? DROP : ACCEPT` encode behaviour in enum order. Reordering the enum silently changes which rules drop.
- Per-slot state is spread over six arrays that `join()` and `reset()` clear by hand.
- Fix:
  - One `const struct { const char* name; int tolerance; bool drops; float burst, refill; } rules[RULE_COUNT]` table.
  - One `struct Slot { int speedScore; float score; int breaks[RULE_COUNT]; bool warned; float tokens[...]; Track track; } slots[MAX_PLAYER]`.
  - `join()` becomes `slots[i] = Slot()` plus refilling the tokens.

**10. LOW: the step 8 bad-window count keeps running at `sv_antiCheat 0`.**
- Where: `ServerValidator.cpp:112-114`.
- `s` still increments at level 0 and only the kick is gated. Switching 0 → 1 mid-game kicks a player on the first frame because of stale windows.
- Fix: gate the increment, or set `s = 0` while the level is 0.

**11. LOW: SPAWN_EARLY hits the client's stuck-in-wall respawn.**
- Where: `ServerRecv.cpp:681`.
- `Game.cpp:636-655` sends a spawn request while alive when the client is stuck in a wall. It sends only once (`spawnRequested`). If a second break falls in the same window, level 2 drops it and the player stays stuck.
- Fix: exempt the request when the server-side position is inside a non-passable cell, or document it.

**12. LOW (tests): not every rule is exercised both ways.**
- No negative test for:
  - ROCKET_WEAPON: a bazooka holder firing rockets.
  - WALL: an honest corner walk.
  - SHOT_DEAD in the 0.2–1.0 s band.
  - FIRE_RATE and SHOT_DEAD under a stall (finding 3).
- Level 2 drop and kick are tested for floods only through direct `violation()` calls; only SHOT_WEAPON and chat are tested through handlers.
- The positive tests check out:
  - The wall test's first frame only seeds Track, so the hit is really WALL, not TELEPORT.
  - The respawn negative test is real.
  - Each flood type has a positive and a negative test.

**13. LOW (docs):**
- Step file 9.4 says "Excess is dropped and scored", but excess is dropped only at level 2 (9.1). Align the wording.
- Step file 9.3 says spawns and deaths reset state, which is not literally true (finding 6).
- The line references in `ANTI_CHEAT_TUNING.md` are accurate at 3f23b5d: ServerValidator `:14`, `:16-17`, `:44`, `:47`, `:72`, `:108`, `:185`; ServerRecv `:681`, `:965`, `:1055`. They will drift once finding 8 lands. Prefer naming functions or symbols.
- The `ARCHITECTURE.md` rows are accurate, scope is inside the Allowed paths, and the commits are Hub-Tansky with no attribution trailers and no mixed mechanical changes. The acceptance play-test log is not attached yet (status IN PROGRESS, as expected).

## Not checked / unsure
- I did not run the ASan preset; I used the macOS Debug build plus one scratch probe, kept out of the repo in the session scratchpad.
- The false-positive thresholds in findings 4, 5 and 7 are derived from the code (send interval = avgPing frames, reliable delivery of shots), not measured in a real networked game.
- Whether explosion knockback (`NET_SVCL_PLAYER_HIT` vel; `framesSinceKnockback` is set only for bullet and minibot hits, `Game.cpp:1316,1538`) can exceed the teleport or path slack before the client clamp applies. I did not trace the order of the per-frame update.
- DKO-model maps: the server's wall check uses only `cells[].passable`. The `modelmaps_______` path looks dead, so I did not pursue it.
- The listen-server host was not tested separately; it goes through the same babonet path.
