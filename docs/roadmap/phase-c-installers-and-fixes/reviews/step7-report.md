SKILL: Launching skill: thermo-nuclear-code-quality-review

## Step 7 review (R28, Windows TCP-close crash), fresh context

**Root cause check:** the fix is correct. The server-side constructor `cClient(sockaddr_in*, int, UINT4)` (engine/babonet/src/cClient.cpp:72) zeroed only `masterS`. `Disconnect()` (cClient.cpp:544-559) runs `FD_CLR(fd, &master)` on that set. On Windows, `FD_CLR` loops up to the set's `fd_count`, and here that count was uninitialised heap memory, so the loop read past the array. That matches the ASan report at :547/548.

The double close is also real. `RemoveClient` closed the socket, then `delete C` → `~cClient` → `Disconnect()` closed the same handle again. That happens on POSIX too, not only on Windows.

**Paths checked:**
- **Server-side cClient deletion:**
  - `RemoveClient` (all callers: cServer.cpp:316, 327, 398, 428, 448, and `bb_serverDisconnectClient`) is now correct.
  - `~cServer` (cServer.cpp:1078) deletes clients without `RemoveClient`. That gives one close via `~cClient`, and `master` is now zeroed, so it is fine.
  - The deletes at cServer.cpp:642, 657 and 686 are for `cIncConnection`, whose constructor zeroes its own sets (cIncConnection.cpp:56).
- **Client side:**
  - `cClient(HostIP,…)` zeroes `master` (cClient.cpp:169), and `cConnection` zeroes it again (cConnection.cpp:69).
  - `bb_clientDisconnect` (baboNet.cpp:628-637) calls `Disconnect()` and then `delete`. The second `Disconnect` sees `FileDescriptor == 0`, so there is no double close.
- **Other fd_sets:** `cServer`, `cPeer2Peer`, `cUDPserver`, `cIncConnection` and `baboNet` P2P all call `FD_ZERO`. The only other uninitialised ones are `master`/`masterS` in the default `cClient()` constructor (cClient.cpp:25), which nothing calls.

## Findings

1. **High (out of the diff, but on the same close path).** cClient.cpp:583: `ReceiveStream` calls `Disconnect()` on a **server-side** client when the partial "RND1" key is wrong, then returns 0.
   - The client stays in `Clients` with `FileDescriptor = 0`, and its closed socket stays in `cServer::master`.
   - The next `cServer::ReceivePacketsFromClients` `select` (cServer.cpp:371) fails on the closed socket (WSAENOTSOCK / EBADF) and returns 0 on every tick. The server stops receiving from everyone.
   - `IsReadyToReceive` cannot recover either: it tests `errno == 9`, and Windows `select` does not set `errno`.
   - If `RemoveClient` is ever reached for this client, it runs `FD_CLR(0)` and `CloseSocket(0)`. On POSIX that closes stdin.
   - Any peer that sends a bad partial key can trigger this. Before this fix, the path crashed on Windows (the same `FD_CLR` overflow); now it wedges the server.
   - Fix: replace `Disconnect();` with `return 1;`. That is what the full-key path does at :623/:628 ("potential hacker"), and the caller (cServer.cpp:437-448) then runs `RemoveClient`.
   - If you rule it out of scope, record it in KEY_QUESTIONS as a new R-number and in possible-new-scope.md.

2. **Medium (step not closable yet).** `.github/workflows/r28-windows.yml` and its ARCHITECTURE.md row (ARCHITECTURE.md:171) are still present, but task 7.3 and the Allowed paths say to delete them. The commit "step7.3: the master test runs on Windows again" did not delete them. Fix: delete both in a 7.3 commit before the PR.

3. **Medium (fix proof missing).** REVIEW.md §2 requires a test with fix proof, and the acceptance check needs the loop to pass 50/50 on ASan and Debug *after* the fix, plus CI `build (win-x64-msvc)` running `master`. Only the before-fix evidence (50/50 failing) exists so far. Fix: record the post-fix run IDs (loop and normal CI) in reviews/step7.md. The re-enabled `master` ctest is an acceptable regression test: the uninitialised `fd_count` overflow cannot happen on POSIX, so no cross-OS test is needed.

4. **Low (stale doc).** docs/analysis/KEY_QUESTIONS.md:161, R28: the description column still says "cause not found. The master test doesn't run on Windows until this is fixed (step 5)", which contradicts the new fixed note in the same row. Fix: cut those clauses.

5. **Low (the explanation does not cover one symptom).** The original R28 text names two crash sites:
   - a client's `bb_clientDisconnect` right after receiving;
   - the master dropping timed-out clients.

   The fix note only explains the server side, and the client-side `cClient` already zeroed `master`. Fix: say in KEY_QUESTIONS or the review record that the client-side report was the same in-process server crash, or that it is unexplained.

6. **Low (structure / code judo).** The three `cClient` constructors each initialise about 25 members by hand. That duplication is why one constructor missed `FD_ZERO(&master)`, and the default constructor still misses both sets. Value-initialising in the header (`fd_set master{}; fd_set masterS{};` in cClient.h:104-105) would cover every constructor and make the two `FD_ZERO` lines redundant. Alternatively, delete the unused `cClient()`. Optional under the MVP rule; this is how the bug class gets fixed for good.

7. **Low (comment accuracy).** cServer.cpp:970 says "Windows may have reused the handle". POSIX reuses the lowest free fd, so reuse is more likely there, not less. Fix: "the handle may have been reused".

8. **Low (bookkeeping at close).** docs/roadmap/possible-new-scope.md:296, PNS-38 status, still reads "R28 → Phase C step 7". The step file Status is IN PROGRESS. Update both when the step closes.

9. **Info.** Commits are cleanly split (7.1 workflow, 7.2 code, 7.3 CMake, then docs), with no mechanical/functional mixing. ARCHITECTURE has the step file row. No ADR is needed. No README is affected; engine/babonet/README.md has nothing about this. No file-size or spaghetti growth: the diff is 2 lines of code.

## Verdict
The core fix is correct and minimal, and it covers every path that deletes a server-side `cClient`. It is not ready to close:
- Finding 1 should be fixed (one line) or recorded.
- The diagnostic workflow and its ARCHITECTURE row must be deleted.
- The post-fix Windows CI evidence must be recorded.
- The R28 description must be cleaned up.
