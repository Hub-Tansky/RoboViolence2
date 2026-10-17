SKILL: Launching skill: thermo-nuclear-code-quality-review

Review of branch refactor/phase-c-step6-server-and-master (`git diff origin/main...HEAD`, 6 commits b2ee2f1..15759ab). Uncommitted SECURITY.md and step9 file ignored.

What checks out:
- 6.1: the length rule runs before the HMAC check. `adminFails[slot]` resets when a connection joins (Server.cpp:403) and the 5th failure disconnects. The test covers the short password, 4 failures still connected, and the 5th disconnecting.
- 6.2: `banPasswordMatches` is correct for secrets of 7 characters or fewer, and it bounds the compare on the unterminated `stCacheUnban.Pass`. The test shows `roxbabo` refused and the configured secret accepted.
- 6.4: the LAN browser peers survive. `ReceivePeersPacket` still handles `NET_BROADCAST_GAME_INFO` and `NET_BROADCAST_QUERY` (CMaster.cpp:300-385), and the peer timeout loop stays. The `remoteAdmin` console command (Console.cpp:785) only calls `scene->join` and is untouched. `grep RA_|roxbabo` over game/src and masterserver/src finds nothing.
- Commits are one per task, with no mechanical changes mixed in. ARCHITECTURE.md drops RemoteAdminPackets.h and adds ADR 0022. No file crosses 1k lines because of this diff; the step deletes about 640 lines.

Findings:

1. **Medium**: `KILL_SERV` lookup goes stale after the game server reconnects. `masterserver/src/cMasterServer.cpp:404-435` (CreateGame update path) and `:477` (GetGameByConnection).
   - Problem: games outlive their connection; `RemoveClient` does not remove them. When the server's master link drops, `CMaster::sendPacket` reconnects with a new babonetID. The next heartbeat matches the existing entry by ip:port and memcpys only `GameInfos`, so `G->BaboID` keeps the dead connection's ID. `KILL_SERV` from the new connection then finds nothing, and the game stays listed until GAME_TIMEOUT.
   - Before this step the lookup was by source IP and worked after a reconnect, so this is a regression. It also misroutes `CACHE_LIST_REMOTE` (cNetManager.cpp:277), which was already wrong before this step.
   - Fix: set `G->BaboID = newGame->BaboID;` in both update branches. Add a test step: register on connection A, re-register on B, send KILL_SERV from B, expect the list to be empty.

2. **Medium**: ADR 0022's trust rule can be bypassed with `Priority != 0`. `masterserver/src/cBV2game.cpp:26`.
   - Problem: the constructor overwrites `GameInfos.ip` with the chosen address only when `GameInfos.Priority == 0`. `Priority` comes from the wire, so a heartbeat from any public source with `Priority=1` is listed at whatever `row.ip` it carries, with a 5x timeout and high-priority placement.
   - This predates the step, but ADR 0022 now says the master uses an advertised address only from private sources, and the code doesn't enforce that.
   - Fix: let the constructor always use the `ip` argument, which deletes that special case; UpdateGame already decides the address. Also zero `row->Priority` in UpdateGame, or record client-set Priority as PNS.

3. **Low (stale doc)**: `docs/ops/runbook.md:72-74`, the "Master address limitation" section, still says remote clients can't join a co-located server from the browser. With `BV2_PUBLIC_ADDRESS` that is no longer true. Fix: replace the section with one line pointing to `BV2_PUBLIC_ADDRESS` and ADR 0022, or delete it.

4. **Low (stale doc)**: `docs/ops/runbook.md:70` says "The old UDP remote admin (`RA_LOGIN`) is refused." It is now removed; reword or delete. Also `docs/decisions/0015-admin-http-api-behind-tls-proxy.md:8,13` cites `CMaster.cpp:390-470` and says the login "is refused". ADRs are history, so leave the text, but a one-line "superseded detail: removed in Phase C step 6" note is optional.

5. **Low (stale doc)**: `config/README.md:6`, the loader-order line, still lists only 4 env vars. The line above it now lists 9. Fix: replace the list with "environment (the variables above)".

6. **Low (dead comments from the RA removal)**: four comments were left behind with nothing under them.
   - `game/src/GameVar.h:561` and `:563` ("send server var to a peer(remote admin)", "send one var to a peer( remote admin )") now sit above the babonetID `sendOne` and misdescribe it.
   - `game/src/Server.h:169` ("Send player list to a remote admin").
   - `game/src/CMaster.h:168` ("hlper function to know if the current peer is remote admin authorized").
   - Stray blank lines remain at Console.cpp:112, SceneNet.cpp:215, ServerRecv.cpp:447 and :596.
   - Fix: delete them.

7. **Low (silent misconfiguration)**: `masterserver/src/cNetManager.cpp:10`. A `BV2_MASTER_BAN_PASS` longer than 7 characters makes every ban fail with nothing logged; this is documented, but an operator only finds out when bans don't work. Fix: print one warning at master start when the secret is longer than 7, or compare only the first 7 characters. The compare is also not constant-time; that is minor given PNS-66.

8. **Low**: `game/src/ServerRecv.cpp:244-248` logs every admin request to the console when the password is short. Step 9's rate limits bound it, but a spammer can flood the log. Fix: log once (a static flag), or drop the log and keep the startup notice. Optional.

9. **Low (structure)**: the address choice is split across two places, `UpdateGame` (cMasterServer.cpp:386-389) and the cBV2game constructor's Priority branch. Applying fix 2 makes `UpdateGame` the only place. Also, `IsPrivateAddress` is a public static on `cMasterServer` only so the test can reach it; a free function in the .cpp with a declaration in the header would also do. It's acceptable as is.

10. **Info (test coverage)**: no end-to-end test shows that a public-source heartbeat's `ip` is ignored. The in-process test always connects from loopback; `IsPrivateAddress` is unit-checked instead. That is acceptable for this step; say it in the step record.

11. **Info (bookkeeping)**: PNS-44, PNS-42 and PNS-46 still read "Accepted" in `docs/roadmap/possible-new-scope.md`, and the step **Status** is IN PROGRESS. Both are expected to change when the step closes.

Verdict: not ready to close. The step is a net simplification (a large dead subsystem deleted) and REVIEW.md section 2 is met: tests with fix proofs, ADR 0022, ARCHITECTURE.md and the env lists all updated. Fix 1 (a real regression with a one-line fix and a test) and fix 2 (the ADR's guarantee not enforced; deleting the Priority branch fixes it) before closing. Findings 3-6 are quick doc and comment cleanups worth doing in the same pass. The rest are optional.

## Round 2

SKILL: Launching skill: thermo-nuclear-code-quality-review

Round 2 review of `git diff 15759ab..HEAD` (5 commits, 12 files, +25/-17), checked against the round-1 report and REVIEW.md section 2. I modified no files; the uncommitted owner files were ignored.

**Round-1 findings resolved**
- **#1 (KILL_SERV after reconnect):** `G->BaboID = newGame->BaboID` is now set in both update branches (masterserver/src/cMasterServer.cpp:417 and :438).
  - New test phase at tests/test_master.cpp:149-158: register on connection A, re-register on B, KILL_SERV from B, list is empty.
  - Fix proof is the author's.
- **#2 (Priority bypass):** `UpdateGame` zeroes `row->Priority` (cMasterServer.cpp:389). The cBV2game constructor now always uses the chosen `ip` with a bounded `snprintf` (cBV2game.cpp:26). No overlap: `ip` points into the caller's row, not into `GameInfos`.
  - This also resolves #9's split address decision: `UpdateGame` is now the only place.
- **#3, #4, #5:** docs/ops/runbook.md:70 and :72-74 and config/README.md:6 are corrected.
- **#6:** the dead remote-admin comments and stray blank lines are removed.
- **#7:** start-up warning for an unset or too-long ban password (cNetManager.cpp:23-25).
- **#8:** the short-password notice is logged once (ServerRecv.cpp:246-248).
- **Left as agreed:** #4's ADR 0015 note, #9's `IsPrivateAddress` placement, #10 to be noted in the record.

**Findings**

1. **Low (left half-dead by #2's fix):** `Priority` is now always 0 on the master.
   - Both clients already sent 0 (game/src/CMaster.cpp:375, :572), and `UpdateGame` forces it.
   - So `GAME_TIMEOUT * 5` (cBV2game.cpp:33) and the `lastHighPriority` logic in `CreateGame` (cMasterServer.cpp:397, 406, 429, 447-452) only ever take one path.
   - That path inserts a new game before the list tail, an odd ordering that predates the step.
   - Fix: delete the priority logic (plain timeout, append at the tail) and the comment at cMSstruct.h:152. Or record it as a PNS item for later. It is outside the round-2 minimum, so it does not block.

2. **Low (duplication):** `CreateGame` still has two identical "update existing game" blocks: one in the loop and one for the tail (cMasterServer.cpp:409-422 and :427-443). The #1 fix had to be applied twice, which shows the risk.
   - Fix: loop `for (G = Games; G; G = G->Next)`, or extract a `findGame(ip, port)` helper so there is one update path. This step's fix grew a pre-existing pattern; it did not create it.

3. **Low (formatting):** the `G->BaboID` line at cMasterServer.cpp:438 has one tab too many compared with its block.
   - Fix: `git clang-format` on that line or a manual one-tab fix.

4. **Low (leftover blank lines):** deleting the comments left a double blank line at game/src/CMaster.h:167-168 and game/src/Server.h:168-169. Fix: drop one blank line in each.

5. **Low (duplicated rule):** the ban-secret rule (set, at most 7 characters) is written twice: in `banPasswordMatches` (cNetManager.cpp:8-12) and in the `Init` warning (cNetManager.cpp:23-25).
   - Fix: one `static const char * banSecret()` that returns the usable secret or nullptr, used by both. Or accept it as is: it is two lines.

6. **Info (logged-once notice):** `static bool told` in ServerRecv.cpp:246 is never reset. If the operator fixes `zsv_adminPass` at runtime and later sets a short one again, nothing is logged. That is acceptable under the AGENTS.md corner-case rule. The `told = true;` after the `if` could go inside it, but that is cosmetic.

7. **Info (test strength):** the reconnect phase only shows that KILL_SERV from the new connection works. It does not show that the old connection can no longer kill the game. That is enough for the regression; no change needed.

8. **Info (bookkeeping, expected at close):**
   - Step **Status** still reads IN PROGRESS.
   - PNS-42, -44 and -46 in docs/roadmap/possible-new-scope.md are still open.
   - The round-1 report is untracked and should be recorded in docs/roadmap/phase-c-installers-and-fixes/reviews/step6.md together with this round.
   - #10 needs its note in the record.

**REVIEW.md section 2**
- Ground rules hold; there are no [GUI] or asset changes.
- The #1 fix has a test with fix proof. The #2 change is enforced in code, and ADR 0022 now matches it.
- Docs are updated. No mechanical changes are mixed with functional ones in a commit.
- Every round-1 finding is acted on or explicitly deferred.

**Verdict:** ready to close once the bookkeeping in #8 is done. #3 and #4 are cheap whitespace fixes worth folding in. #1 and #2 are optional structural follow-ups: delete them now or record them as a PNS item. No blockers.
