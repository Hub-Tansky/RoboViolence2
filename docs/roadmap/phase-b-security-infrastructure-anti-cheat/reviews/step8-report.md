SKILL: Launching skill: thermo-nuclear-code-quality-review

# Step 8 review: `git diff ad5052e...15eace7` (PR #38)

**Verdict: no behaviour change found, but don't approve yet.** The two moved checks match the old code line by line. The problems are test coverage for the speed/frame rule, and that the step's 8.2 requirements are only half met.

**Checks I ran at 15eace7:**
- macos-arm64 build: no work to do, so the build was already current at HEAD.
- `ctest`: 13 of 13 pass, including `validator`.
- `tools/review.sh` (fast mode): all checks pass, including scope.
- `ServerRecv.cpp` shrinks to 1138 lines, so no file crosses the 1k threshold.

**Behaviour, item by item: all identical**
- **Weapon kick message:** `"%s is trying to hack his %s weapon! Kicked him"` with `which` set to `"primary"` or `"secondary"` gives the same strings as before. The order is unchanged (primary checked first), and so is the path (`sayall` + `scene->kick`).
- **`sv_validateWeapons`:** the early `ACCEPT` keeps the old guard. It still runs only inside `if (game->players[spawnRequest.playerID])`.
- **Speed path:**
  - The velocity maths is unchanged: `char` promoted then `/10.0f` equals the old `(float)char/10.0f`. Dropping `vel[2]` is safe because it was never used.
  - The frame formula, the knockback rule (>60 frames), the 3.25+0.15 limit and the reset on a clean window are all the same.
  - The kick at `>= 3` is now `>= sv_speedHackKick`, with default 3 and minimum 1.
  - The kick path is the same silent disconnect: `RA_DisconnectedPlayer`, `bb_serverDisconnectClient`, delete, `PLAYER_DISCONNECT`.
- **Dead player:** `validator.reset` sits in the same `else` branch where `speedHackCount = 0` was.
- **Slot reuse:** `createNewPlayerSV` (`game/src/Game.cpp:1621`) is the only server-side place a `Player` is created. Server and Game are created together (`game/src/SceneNet.cpp:41,82`). So calling `reset` right after `createNewPlayerSV` matches the old constructor reset. A slot left by a speed-kicked player keeps a score of 3 until the next occupant, who gets it reset, so this is equivalent.

## Findings

1. **Medium: tests: the frame-rate rule, the knockback exemption and both resets are not pinned.** `tests/test_validator.cpp:50`
   - `window()` always sends `cf.frameID = 1000`. `lastFrame` is 0, so it becomes 1000 and `clientFrames` is always 0, which means `frameHack` is never true in any test.
   - The refactor rewrote that formula (`game/src/ServerValidator.cpp:33-34`). Deleting the frame check, or breaking its tolerance, would leave all four tests green.
   - Also untested:
     - the knockback exemption (`framesSinceKnockback > 60`);
     - the reset while dead (`game/src/ServerRecv.cpp:852`);
     - the reset on slot reuse (`game/src/Server.cpp:391`);
     - the kick text: swapping "primary" and "secondary" isn't caught, since the step requires that the existing kick messages are reused.
   - The step note "fail with the speed threshold at 4 or the weapon check off" is true, but it overstates coverage of "today's behaviour".
   - Fix: add these cases.
     - A frame-hack window: `frameID` advancing by about 200 per 90-tick window, at legal speed, kicks after 3.
     - Two bad windows, then die/respawn (or disconnect and let a new client take the slot), then two bad windows: the player is not kicked.
     - A bad-speed window inside 60 ticks after a hit does not score.
   - Give each one fix proof (revert the line, see the test fail).

2. **Medium: structure: the speed check is split across three places, not consolidated.** `game/src/ServerRecv.cpp:777-792`, `:852-855`, `game/src/ServerValidator.cpp:29-47`
   - The step goal is "all server-side cheat checks live in one `ServerValidator`". The score moved, but the window gate (`frameSinceLast >= 90`) and the window bookkeeping (`frameSinceLast/lastFrame/currentFrame = 0`, twice) stay inline in the handler.
   - The dead-player branch now resets the validator and then three `Player` fields by hand, next to each other. That is one concept in two owners.
   - A reader still has to hold the handler, `Player` and the validator in their head to understand one rule.
   - Code-judo fix (all within Allowed paths): call `onCoordFrame` on every accepted frame from an alive player. It returns `ACCEPT` before the window closes; on window close it runs the check and clears the window fields. A dead player calls one `validator.reset(slot, player)` that clears the score and the window together. This deletes the duplicated three-line resets and the gate from `ServerRecv.cpp`.
   - Moving the window fields themselves off `Player` is outside Allowed paths ("`speedHackCount` only"): note that as a PNS (step 9 will need it anyway) rather than doing it.

3. **Medium: 8.2 deviation needs the owner's sign-off, not just a Note.** `docs/roadmap/phase-b-security-infrastructure-anti-cheat/step8-server-validator.md:35-36` vs Notes `:48`
   - 8.2 requires "configurable thresholds and actions (log, warn, kick, temp-ban)". What shipped:
     - one threshold;
     - one fixed action (kick);
     - the weapon rule doesn't score at all (it kicks immediately);
     - no `onShoot`.
   - The deviation is recorded in the Notes, with temp-ban deferred to PNS-40 and log/warn to step 9 (step 9 lines 29-30 do cover log/warn). But the Tasks text still promises the full set.
   - Under REVIEW.md §5 ("bends … the plan" means "confirm with the owner first") and §3 (a rejected alternative counts as a non-obvious decision), this needs the owner's sign-off in the review record, or an amended 8.2 task text.
   - Fix: get the owner's confirmation, and edit 8.2 to say what is delivered, with pointers to step 9 and PNS-40.

4. **Low: spaghetti: duplicated condition on the new-player path.** `game/src/Server.cpp:391`
   - `if (playerID != -1) validator.reset(playerID);` comes right before `if (playerID == -1) {…} else strcpy(...)`.
   - Fix: move the reset into the existing `else` branch (it needs braces).

5. **Low: API: the out-parameter and a redundant argument.**
   - `game/src/ServerValidator.h:12`: `const char *& which` is an out-parameter that is only meaningful on `KICK`.
     - Return the reason instead, e.g. `const char * badSpawnWeapon(const net_clsv_spawn_request &)` giving `nullptr`, `"primary"` or `"secondary"`.
     - Or use a `Verdict { ACCEPT, KICK_PRIMARY, KICK_SECONDARY }`.
     - Either way, the caller's `which = nullptr` initialisation goes away.
   - `ServerValidator.h:15`: `onCoordFrame(int slot, const Player & player, …)` passes the slot twice. Use `player.playerID`, or pass only the slot and look the player up.

6. **Low: config example not updated.** `config/bv2.example.cfg:110`
   - `sv_validateWeapons` is listed there, but the new `sv_speedHackKick` is not, so operators won't discover it.
   - `config/` is outside the step's Allowed paths. Either get the owner's approval written next to the path, or add a PNS entry. Don't silently skip it.

7. **Low: unclear var help text.** `game/src/GameVar.cpp:451`
   - `"3 s speed or frame-rate violations in a row"` reads as "3 seconds of violations".
   - Suggested: `"[int : 3-second windows with a speed or frame-rate violation in a row that kick, 1 to unlimited (default 3)]"`.

8. **Low: commit hygiene.**
   - `30316c5` is titled `step8.2` but also does all of 8.3 (move checks, remove `speedHackCount`). AGENTS.md asks for one commit per task item. Mention 8.3 in the subject (`step8.2+8.3: …`) or split it in future steps.
   - `15eace7` is labelled `step8.4` but holds only step notes and PNS-40, not configuration work.
   - Nothing is mechanical mixed with functional changes. Identity is Hub-Tansky.

**Docs checked, nothing wrong:**
- `ARCHITECTURE.md` rows for `ServerValidator.{h,cpp}`, `tests/test_validator.cpp` and the `tests/CMakeLists.txt` description are present, and the architecture check passes.
- PNS-40 is well formed.
- `grep speedHackCount game/src` is empty.
- The Note about `docs/analysis/02_DATA_STRUCTURES.md` is correct, since those docs cite upstream line numbers.
- Status is IN PROGRESS, which is correct because no `reviews/step8.md` exists yet.

## Not checked / unsure

- I did not check out `63fa416` to confirm that the 8.1 tests pass before the refactor and fail with threshold 4 or the weapon check off. The author claims this in the Notes. By reading the test (2 bad, clean, 2 bad, then the 3rd bad kicks), threshold 4 would fail.
- I did not run `tools/review.sh --full`, the ASan presets, or the Linux/Windows builds. I also did not check the signedness of `char` in `vel` on Linux ARM; it is unchanged from before either way.
- I did not check how `sv_*` variables reach clients, or whether a new pushed variable affects older clients or needs a `GAME_VERSION` bump. My assumption is that it doesn't, because the wire structs are untouched.
- I did not check whether `console->sendCommand("set sv_speedHackKick 1")` in the test leaks into later tests if a CHECK fails midway (the restore at `tests/test_validator.cpp:98` still runs, since CHECK doesn't abort).
