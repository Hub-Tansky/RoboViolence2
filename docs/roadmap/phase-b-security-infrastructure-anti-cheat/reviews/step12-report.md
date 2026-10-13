SKILL: Launching skill: thermo-nuclear-code-quality-review

Review of `git diff ac296ba...e4d6921` (PR #43, Phase B step 12: remote admin). I reviewed the code read-only. The macos-arm64 build was up to date, and `ctest -E master` passed 16 of 16. I reproduced findings 1, 3 and 5 against the existing `build/macos-arm64-asan/runtime/bv2dedicated`, which already includes AdminApi, using a scratch pref dir and port 18080.

## Findings

1. **Critical: `restart` or `disconnect` sent through the API is a use-after-free that kills the server, and that request is never audited.**
   - Where: `game/src/AdminApi.cpp:184-188` together with `game/src/Console.cpp` (`restart` does `ZEVEN_SAFE_DELETE(scene)`; `disconnect` calls `Scene::disconnect()`, which deletes `server`).
   - Problem: `serve()` runs the command synchronously inside `Server::update` → `AdminApi::update`. Both commands delete the `Server` and the `AdminApi` it owns. Control then returns to `update()`, which uses `c` (`send`, `connections.erase`) and the dead `this`.
   - Reproduced: `curl -H "Authorization: Bearer …" --data restart …/command` gives ASan `heap-use-after-free AdminApi.cpp:184 in AdminApi::update`. `disconnect` gives the same.
   - Audit gap: `audit()` runs after `send`, so the command that killed the server leaves no line in `admin-audit.log`.
   - The in-game `NET_SVCL_CONSOLE` path has the same latent hazard. This PR opens it to a network API.
   - Fix (pick one):
     - Queue the command and run it at the end of `AdminApi::update` after all socket work; return to the caller immediately afterwards and touch no members.
     - Or refuse the commands that tear down the scene (`restart`, `disconnect`, `dedicate` when not running) in `serve()`.
   - Also write the audit line before running the command.
   - Add a test that sends `restart`.

2. **High: an unauthenticated client can exhaust the server's connections and file descriptors through the public 8443 port.**
   - Where: `game/src/AdminApi.cpp:152-157, 171`.
   - Problem: the accept loop drains every pending connection with no cap on `connections`. Each connection may sit for `TIMEOUT` = 5 s. The rate limit applies only to completed requests.
   - How it is reached: Caddy streams the body upstream. An internet client can send full headers plus `Content-Length: 4000` and trickle the body. Each such request holds one game-side socket for 5 s and costs one `recv` per frame.
   - Consequence: a few thousand concurrent requests cost CPU on the game thread and push fd numbers up. babonet uses `select`/`FD_SET` (`engine/babonet/src/cConnection.cpp:221`, `cIncConnection.cpp:91`), so player sockets with fd ≥ FD_SETSIZE (1024) corrupt memory. Hitting the container's nofile limit instead blocks player joins.
   - Fix:
     - Cap `connections` (for example 8) and close anything accepted beyond the cap at once.
     - Cut the timeout to about 1 s.
     - Add `request_body { max_size 4KB }` and short server timeouts (`read_body`, `read_header`) to the Caddyfile.

3. **High: any peer that reaches the port can bypass the rate limit and forge the audit source with `X-Forwarded-For`.**
   - Where: `game/src/AdminApi.cpp:179-180`.
   - Problem: XFF is trusted from every peer. The default bind is `127.0.0.1` with no proxy, and in compose anything on the compose network (including `master`) can reach port 8080 directly.
   - Reproduced: 7 bad-token requests with `X-Forwarded-For: 203.0.113.1..7` all returned 401; none got 429.
   - The code also takes the leftmost element. That is client-controlled as soon as any proxy appends (Caddy with `trusted_proxies`, or a CDN in front). It is correct today only because Caddy replaces untrusted XFF.
   - Fix: take the rightmost element, which the nearest proxy adds. Honour XFF only when the TCP peer is the proxy, for example a `zsv_adminApiTrustedProxy` setting, or "never when the peer is loopback and no proxy is configured". Otherwise use `c.peer`.

4. **Medium: the `budget` map grows without bound and is walked every frame.**
   - Where: `game/src/AdminApi.cpp:147, 197`.
   - Problem: each new source adds an entry that is never removed, and `for (auto & b : budget)` runs every frame. Through Caddy, an attacker with an IPv6 /64, or anyone using the XFF spoof in finding 3, can grow it forever.
   - Fix: in the refill loop, erase entries that are back at the full 5 tokens. Absent means full, so `serve` already handles that case.

5. **Medium: the audit log can be forged.**
   - Where: `game/src/AdminApi.cpp:194-195, 76`.
   - Problem: the request line is cut at `\r\n` only, so a bare `\n` inside it lands verbatim in `admin-audit.log`. This happens before authentication.
   - Reproduced: sending `GET /x\n2099-01-01T00:00:00Z 203.0.113.10 ok set zsv_adminPass *** HTTP/1.1` directly wrote a forged second line (`2099-… 203.0.113.10 ok set zsv_adminPass ***`).
   - Caddy (Go) normalises line endings, so this works only from direct access (localhost or the compose network). The XFF value from finding 3 has the same problem.
   - Fix: cut the request line at the first `\r` or `\n`, and replace control characters with `?` in every field `audit()` writes.

6. **Medium: the runbook's audit-log command fails.**
   - Where: `docs/ops/runbook.md` ("Audit log" row): `docker compose … exec server tail admin-audit.log`.
   - Problem: the server image's `WORKDIR` is `/opt/bv2` (`packaging/docker/Dockerfile:45`), but the log is in the pref dir `/var/lib/bv2` (`BV2_PREF_DIR`).
   - Fix: `exec server tail /var/lib/bv2/admin-audit.log`.

7. **Medium (code judo, dead attack surface): the whole legacy `RA_*` subsystem is now unreachable but stays in the code.**
   - Where: `game/src/CMaster.cpp:390-590`, `CMaster.h:79,225-229`, `Console::add` → `master->RA_ConsoleBroadcast` (`Console.cpp:114`).
   - Problem: with `RA_LOGIN` always refused, `remoteAdmin` is never true. Every other `RA_*` case (`RA_COMMAND`, `RA_MAP_FILE_HEADER`/`CHUNK`, which write files, and so on) is dead. So is the `RA_ConsoleBroadcast`/`RA_NewPlayer`/`RA_PositionBroadcast` traffic, which still loops over peers on every console line.
   - The diff patched one branch and left about 200 lines of authentication-gated wire handling, including a file writer, as dormant code.
   - Fix: delete the `RA_*` cases, the `remoteAdmin`/`m_currentFile` peer fields and the `RA_*` broadcast methods and their call sites. Leave the message IDs as gaps per AGENTS.md. If that is judged too large for this step, record it as a `possible-new-scope.md` entry.

8. **Low: the listener state is modelled inconsistently, so `close()` runs every frame when the API is off.**
   - Where: `game/src/AdminApi.cpp:103-110, 138-144`.
   - Problem: `close()` resets `boundPort` but not `boundAddress`, and `boundAddress` starts empty. On every default server (port 0, bind `127.0.0.1`), `address != boundAddress` is always true, so `close()` runs every frame forever.
   - A failed `listen` also leaves `boundPort = port`, so it is never retried until the variable changes. That may be intended, but it is undocumented.
   - Fix: keep one `wanted` (port, address) pair. Assign it whenever it changes, then `close()` and `listen()` if the port is above 0.

9. **Low: Windows `SO_REUSEADDR` allows port hijacking.**
   - Where: `game/src/AdminApi.cpp:120`.
   - Problem: on Windows, `SO_REUSEADDR` lets another local process bind the same address and port and receive bearer tokens.
   - Fix: use `SO_EXCLUSIVEADDRUSE` under `BV2_PLATFORM_WINDOWS`, keeping `SO_REUSEADDR` on POSIX.

10. **Low: request framing is fragile.**
    - Where: `game/src/AdminApi.cpp:41-51, 169, 184`.
    - `header()` lowercases and searches the whole buffer, body included. Restrict it to `request.substr(0, headEnd)`.
    - `Transfer-Encoding: chunked` is not handled. Caddy sends chunked upstream when the client gave no length, for example HTTP/2 without content-length. The game then sees `length = 0` and runs the chunk-size line as the command. Reject `Transfer-Encoding` with 411.
    - `atoi` on `Content-Length` is UB on overflow, and negative values wrap.
    - A non-blocking `send` can partly send and then `close`. Closing with unread input (the 413 path) sends an RST, so the client usually sees a reset instead of the 413.
    - Requests that time out or are half-closed get no audit line, although the runbook says there is "one line per request". Either audit them as `timeout` or reword the docs.
    - CI checks only `GET /status` through Caddy. Add a `POST /command` (for example `info`) through the proxy so body forwarding is covered.

11. **Low: docs drift.**
    - ADR 0015 "Decision" names `sv_adminApiPort`; the variable is `zsv_adminApiPort`.
    - `AdminApi.h:32` says "`audit` gets the logged command and result", but the out-parameters are `command`/`result`. Better, return a small `struct Served { std::string response, command, result; }` instead of two out-parameters.
    - The new env variables `BV2_ADMIN_API_PORT`/`BV2_ADMIN_API_BIND` are missing from the env-layer lists in ADR 0007 (`docs/decisions/0007…:15`, in scope), `AGENTS.md:35` and `config/README.md:4,6` (both out of scope; record a PNS entry or get the owner's approval).

12. **Low: socket handles are stored as `long long` and cast at every use.**
    - Where: `AdminApi.h:20,24` and about 8 casts in `AdminApi.cpp`.
    - Problem: `long long` is used only to keep winsock out of the header.
    - Fix: `std::uintptr_t` (SOCKET is `UINT_PTR`) with one `toSocket()` helper, or a pimpl, removes the cast noise.

13. **Low: test gaps.**
    - Where: `tests/test_admin_api.cpp`.
    - Not covered: a request split across frames (partial reads, the main new parsing logic), 413, 404, a timed-out connection, XFF source handling, a runtime port change, and the `restart`/`disconnect` crash from finding 1.
    - The fixed port 10209 can collide with other parallel ctest runs.
    - Windows has no coverage of the socket branch.
    - The fix proof in Notes is only asserted; there is no pasted failing assertion yet. That is expected at step close, but needed for the record.

14. **Low: commit hygiene and ops.**
    - 46c865e combines task items 12.2 and 12.3 ("(12.3)" in the message), against "one commit per task item".
    - `caddy:2.11.7` is pinned by tag, not digest. It runs as root, unlike the non-root rule step 11 set for the other images.
    - The CI TLS step has no readiness retry for Caddy, unlike the master listing check above it. Wrap it in the same polling loop to avoid a flaky `000`.

## Not checked / unsure
- Windows compile: the branch is not pushed, so no CI run exists. By reading only: include order is fine (the force-included `platform.h` pulls no `windows.h`; `winsock2.h` comes first). `socklen_t` comes from `ws2tcpip.h`. The `inet_addr`/`inet_ntoa` deprecations are covered by `/wd4996`. `WSAGetLastError` is used correctly. `WSAStartup` comes from `bb_init`. I saw nothing that should break under `/W4 /WX`, but nothing has proved it.
- Caddy behaviour is from documentation, not a run:
  - It replaces untrusted XFF.
  - It matches the `localhost` site for a `Host: localhost:8443` header.
  - It issues the internal certificate in time for CI.
- I did not run the compose stack, the CI workflow or `tools/review.sh`.
- I did not empirically test slowloris or fd exhaustion (finding 2); it is reasoned from the code.
- Leaking secrets through command output: I found no echo of secret values in `set`. On a listen-server client build, `connect <ip> <port> <password>` would be logged unmasked, because masking covers only `set`. I did not check this further.
- I did not check whether `sendSVChange` broadcasting `set sv_password …` to clients is a pre-existing leak. It is outside this diff.
