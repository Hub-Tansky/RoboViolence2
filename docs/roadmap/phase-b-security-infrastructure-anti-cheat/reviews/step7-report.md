SKILL: Launching skill: thermo-nuclear-code-quality-review

Review of `git diff 001dfaa...c7624d1` (Phase B step 7, PR #37). This was a read-only review: I did not edit, commit or push anything.

Verification run:
- `cmake --build --preset macos-arm64` printed "no work to do", so the build was already current.
- `ctest`: 12/12 passed, including `crash_fixes` and `babonet_large_message`.
- `tools/review.sh` (fast mode): all checks pass (architecture, hygiene, content-case, original-assets, secrets, diff-rules).
- Every changed path is inside the step's Allowed paths.

## Findings

1. **Medium: the R15 fix still gives wrong averages after the first map change.** `game/src/Player.cpp:843` (`Player::reinit`) together with `game/src/PlayerUpdate.cpp:37-40`.
   - The new code relies on `pingSum == Σ pingLog[]`.
   - `reinit()` sets `pingSum = 0` and `avgPing = 0` but leaves `pingLog[]` and `pingLogID` alone.
   - `reinit()` runs for every player on every `Game::resetGameType` (`Game.cpp:179`, i.e. every map change) and on a team switch (`ServerRecv.cpp:630`).
   - Afterwards `pingSum` is permanently `Σnew − Σold`. A steady-ping player's average falls to about 0 and is clamped to 1 for the rest of the session.
   - `avgPing` sets the position send interval (`Server.cpp:1051`, `PlayerUpdate.cpp:287`), so in real play the defect is still there. The old code had the same flaw, but R15 is not fixed in practice.
   - `r15` only tests a fresh player, so it misses this.
   - Fix: in `reinit()`, zero `pingLog[]` and `pingLogID` next to `pingSum = 0`.
   - Test: extend `r15` to call `p->reinit()` after the first window, then run another `2*PING_LOG_SIZE` samples and `CHECK(avgPing == 10)`.

2. **Low–Medium: commit hygiene. The R7 test overflows under ASan for six commits.**
   - `cccdb0f` (R7) adds `tests/test_babonet.cpp`, which sends 10000 bytes. That message overflows `cClient::Send`'s fixed 3072-byte buffer (R31).
   - The R31 fix only arrives in `c7624d1`, the last commit, so `cccdb0f`..`d14642f` fail the Linux ASan smoke job and can't be bisected.
   - The R31 commit is also labelled `step7.1` but sits after `step7.4`.
   - `step7.4` isn't a task in the step file, which defines only 7.1–7.3.
   - Fix: reorder so the R31 commit comes before (or is folded with) the R7 test. Rename `step7.4` to fit the step's task numbering, or add a 7.4 task to the step file.

3. **Low: the R31 row has no commit hash.** `docs/analysis/KEY_QUESTIONS.md` R31 row reads "**Fixed in step 7**". The acceptance check requires "marked fixed … with the commit". Fix: add `(c7624d1)`, or the new hash after the reorder in finding 2.

4. **Low (code quality): `autoBalance`'s lambda has a redundant parameter and a dead branch.** `game/src/Server.cpp:1347-1380`.
   - `carriedFlag` always equals `toTeam` (blue=0 carries flag 1 = RED, red=1 carries flag 0 = BLUE). Use `game->map->flagState[toTeam]` and drop the parameter; the comment explaining it goes too.
   - `if (player->teamID != oldTeam)` is always true. `player` comes from the other team, and `assignPlayerTeam` returns either `toTeam` or `PLAYER_TEAM_SPECTATOR` (`Game.cpp:929-930`). Both differ from `oldTeam`.
   - Fix: remove the `if` and always broadcast. The lambda then becomes `balance(from, toTeam, n)`.

5. **Low (code quality): R10 copies the same interior-bounds condition twice.** `game/src/MapRender.cpp:643` and `:674`.
   - The full condition `cells && x >= 1 && x <= size[0]-2 && y >= 1 && y <= size[1]-2` appears in both places.
   - Fix: compute `const bool interior = cells && …;` once after `x`/`y` and use it in both `if`s. That also removes the "(R10: same bounds as above)" comment.

6. **Low: `tests/test_babonet.cpp` is missing `#include <algorithm>`.**
   - Line 37 uses `std::equal` and only compiles today through transitive includes from `<vector>`/`<thread>`.
   - Fix: add `#include <algorithm>`.
   - Related: the test hard-codes port 10208 with no `RESOURCE_LOCK`, while the master test locks 10207. Add `RESOURCE_LOCK bv2_babonet_port`, or reuse a lock, if parallel ctest is ever used.

7. **Low: `r12` tests less than its comment says.** `tests/test_crash_fixes.cpp` `r12`.
   - The comment says "returned (and taken) within 0.25". Only the return branch (`ServerCTF.cpp:122`) is exercised; the take branch changed at `ServerCTF.cpp:99` has no test.
   - Fix: also put a RED player 0.4 and then 0.2 from the dropped blue flag and check `flagState[0]`. Or drop "(and taken)" from the comment.

8. **Low (behaviour note for the PR text): an R8 move refused by the approval list still counts.** `game/src/Server.cpp:1365-1376`.
   - When `isApproved` refuses the target team, the player becomes a spectator. That is now broadcast correctly, which fixes the desync.
   - But it still uses up one of `nbToSwitch`, so the teams may stay unbalanced that cycle. The old code moved such players to spectator too, so this isn't a regression.
   - Say it in the PR's R8 behaviour description, which the step requires. No code change is needed under AGENTS' rule to ignore corner cases.

## Checked and correct

- **R7**: the partial-body offset now matches the full-body branch (`cClient.cpp:737` vs `:764`). The 10000-byte message over a 3072-byte `recv` forces at least three partial reads, so the test exercises the defect.
- **R31**: the buffer is sized as 3072 plus every queued header and body. That covers the batch, because `Send` returns after the first flush past `DataRate`.
- **R9**: `Map map(mapList[i], game, 0)`.
  - On the server `isServer` is true, so no textures are loaded. dko models are not cached or shared (`dko.cpp:129,574`), so destroying the temporary `Map` can't free the running map's models.
  - Removing the trailing `mapList[0]`/`""` returns is safe, because both branches before them return.
  - The test is meaningful: `mapList` holds the current map (`Server.cpp:155`), so the old code reloaded it and reset `flagState`.
  - The non-CONSOLE "LOADING" flash on a listen server comes from the `Map` constructor. It existed before (twice per measured map) and now happens once.
- **R13**: `>=` is correct. The test reaches index 10 of a 10-element vector.
- **R8**: half the difference, never the flag carrier, a different player each time (erased from `from`), and the real team is broadcast. 1v5 ends 3v3 with 2 broadcasts; the old code sent 3 broadcasts for 1 real move.
- **R12**: the flag indices and log text are right (`ServerCTF.cpp:253` is the flag-1/red branch). PNS-39 is recorded.
- **R18 rest**: the size_t widening is correct.
  - The deleted CAStar copy code was buggy (its `memcpy` copied `m_mapNodesRef` by element count, not bytes) and is now `= delete`, which the compiler enforces.
  - The tinyxml `mult` widening is fine.
- **R19**: `%lu` is correct.
- **R6**: correct.
- The `ARCHITECTURE.md` rows and `KEY_QUESTIONS` rows R6–R19 match the code and the commits.

## Not checked / unsure

- I did not run the ASan preset or redo the fix proofs.
  - R9's fix proof (indexing an emptied `mapInfoList` that still has capacity) and R13's rely on ASan seeing an out-of-bounds index inside the vector's capacity or a freed `CString`.
  - On libc++ (macOS) vector annotations catch this. Linux libstdc++ without `_GLIBCXX_SANITIZE_VECTOR` may not.
  - Someone should confirm the reverted-fix runs fail on the Linux ASan CI preset, not only on macOS.
- Whether `babonet_large_message` is stable on Windows CI, given R28 (Windows babonet crashes on connection close). The test never disconnects cleanly before exit.
- The R8 PR description (required by the step) wasn't checked; I didn't open the PR body.
- The review record `reviews/step7.md` and `--full` aren't done yet, which is expected while Status is IN PROGRESS.
