# KEY QUESTIONS — Design Q&A, Pitfalls and Defect Register

> Answers are backed by code. The defect register at the end ranks what a relaunch should fix first.
> Severity: **Critical** = remote crash/compromise by any player; **High** = exploitable cheating or data corruption; **Medium** = wrong behaviour under realistic conditions; **Low** = cosmetic/edge.

---

## Part A — "Why" questions

### Q1. Why does everything run at exactly 30 Hz, and why do counters look like "frames"?
`dkcInit(30)` fixes the step, and `dkcGetElapsedf()` returns the constant `1/30` rather than measured time, so the whole simulation is deterministic per step and frame counts are time: 30 frames = 1 s.
[VERIFY: BaboViolent2/Code/main.cpp:495] [VERIFY: Engine/DukZeven/Code/dkc.cpp:47-50] [VERIFY: Engine/DukZeven/Code/dkc.cpp:98]
Pitfall: comments often say seconds where the code means frames; e.g. "3sec" next to a 300-frame (10 s) timeout.
[VERIFY: BaboViolent2/Code/Server.cpp:1057]

### Q2. Why is movement client-authoritative?
Everything travels over TCP (`UDPenabled=false`), so server-side input simulation would add a full RTT plus head-of-line blocking to every step. Instead the client integrates and collides its own babo and uploads position+velocity; the server only rejects obviously impossible speeds.
[VERIFY: BaboViolent2/Code/Server.cpp:153] [VERIFY: BaboViolent2/Code/PlayerUpdate.cpp:249-315] [VERIFY: BaboViolent2/Code/ServerRecv.cpp:807-854]

### Q3. Why does the send interval depend on ping?
On a reliable ordered stream, sending faster than the connection drains just grows the queue. Both sides wait `max(avgPing, sv_minSendInterval)` frames between coord frames, so the rate adapts to the link.
[VERIFY: BaboViolent2/Code/PlayerUpdate.cpp:292] [VERIFY: BaboViolent2/Code/Server.cpp:1133]

### Q4. Why the magic `/90` in interpolation?
It makes the Bézier end tangents equal to the sender's velocity: with `N` frames between keys (`N/30` s), a Hermite curve needs control offsets of `v·Δt/3 = v·N/90`. See `ALGORITHM_02` §4.2.
[VERIFY: BaboViolent2/Code/Player.h:97-122]

### Q5. Why do hitscan shots stop at the nearest babo when the loop has no "min distance" logic?
`segmentToSphere` shortens the segment to the hit point on every match, so later candidates must be nearer. See `ALGORITHM_03` §2.4.
[VERIFY: BaboViolent2/Code/Helper.cpp:505-509] [VERIFY: BaboViolent2/Code/Game.cpp:1545-1551]

### Q6. Why is there a `_PRO_` protocol at all?
Pro moves shot spread to the server (client sends direction, server rolls the random spread), adds a client binary checksum challenge, minibots and Pro-only tuning. Pro and non-Pro versions (21100 vs 21000) refuse each other.
**Update (step 1.6/1.7):** the non-Pro ruleset and the client binary checksum challenge (with its `BadChecksum` SQL insert, Q-S3) were removed; Pro is the only ruleset.
[VERIFY: BaboViolent2/Code/Game.cpp:1118-1126] [VERIFY: BaboViolent2/Code/ClientRecv.cpp:38-80] [VERIFY: BaboViolent2/Code/Server.h:30-36]

### Q7. Why two `Game` objects when hosting?
The listen server runs a real `Server` and a real `Client` in one process, talking over loopback TCP. This keeps a single code path for hosted and remote play at the cost of simulating the world twice.
[VERIFY: BaboViolent2/Code/SceneNet.cpp:40-60]

---

## Part B — "How" questions

### Q8. How do I run a Linux dedicated server?
`make` in the repo root builds `bv2dedicated` into `BaboViolent2/Content/` (needs sqlite3, curl, OpenSSL, GLU dev libs). Pass a script name as `argv[1]` to run `execute <file>`; config is `main/bv2.cfg`.
[VERIFY: Makefile:1-7] [VERIFY: BaboViolent2/Code/main.cpp:610-615] [VERIFY: BaboViolent2/Code/main.cpp:489-491]
The Makefile hard-codes `-D _PRO_`, so the server only accepts Pro clients (version 21100).
[VERIFY: Makefile:5] [VERIFY: BaboViolent2/Code/Server.h:33]

### Q9. How do I add a new network message?
1. Define `#define NET_…` + struct in `netPacket.h` (pick an unused ID in the right range).
2. Send with `bb_serverSend((char*)&s, sizeof(s), ID, dest)` / `bb_clientSend(...)`.
3. Handle in `Server::recvPacket` / `Client::recvPacket` with `memcpy` into the struct.
4. If the client must process it before `gotGameState`, add it to the whitelist.
[VERIFY: BaboViolent2/Code/ServerRecv.cpp:40-48] [VERIFY: BaboViolent2/Code/ClientRecv.cpp:84-94]
Pitfall: bump `GAME_VERSION_*` if the struct layout changes, because structs are raw-copied and old peers will misparse silently.
[VERIFY: BaboViolent2/Code/Server.h:33-35]

### Q10. How do I add a server variable?
Declare it in `GameVar`, register with `dksvarRegister("sv_name [type : doc]", &var, min, max, …)`. `sv_*` vars are pushed to clients on join via `gameVar.sendSVVar` and on change via `Server::sendSVChange`.
[VERIFY: BaboViolent2/Code/GameVar.cpp:346-347] [VERIFY: BaboViolent2/Code/ServerRecv.cpp:336] [VERIFY: BaboViolent2/Code/Server.cpp:1502]

### Q11. How are maps distributed?
Clients download missing `.bvm` files from the game server in 250-byte chunks, rate-limited by `sv_maxUploadRate`. 3D `modelmaps` are not transferred.
[VERIFY: BaboViolent2/Code/Server.cpp:1340-1388] [VERIFY: BaboViolent2/Code/Map.cpp:437-439]

### Q12. What external services does the game depend on (relaunch concern)?
- Account auth: default URL is empty since the baseline redaction (was the 2012 ladder host), overridden from `bv2.db` `LauncherSettings.AccountURL`. [VERIFY: BaboViolent2/Code/GameVar.cpp:555] [VERIFY: BaboViolent2/Code/GameVar.cpp:49-52]
- Master server list: read from `bv2.db` by `CMaster::GetMasterInfos`. Since `b19bc06`, a missing DB or row means no master is dialled. [VERIFY: BaboViolent2/Code/CMaster.cpp:930]
- Report upload URLs: configured with `addreporturl`. [VERIFY: BaboViolent2/Code/Server.h:183]

For a relaunch, these endpoints (and `bv2.db` defaults) need to point at infrastructure you control; the auth request is plain HTTP.

---

## Part C — Security findings

### Q-S1. Can one client act as another player? — **Yes (High)**
Most client→server messages carry a `playerID` that the server uses directly as `game->players[playerID]` without checking that the slot belongs to the sending connection (`bbnetID`). Only `COORD_FRAME` compares `babonetID`.

| Message | Unbound use of `playerID` | Consequence |
|---|---|---|
| `TEAM_REQUEST` | [VERIFY: BaboViolent2/Code/ServerRecv.cpp:634-648] | move any player to spectator/other team |
| `PLAYER_CHANGE_NAME` | [VERIFY: BaboViolent2/Code/ServerRecv.cpp:924-931] | rename anyone |
| `VOTE` | [VERIFY: BaboViolent2/Code/ServerRecv.cpp:67-80] | vote on behalf of others |
| `SPAWN_REQUEST` | [VERIFY: BaboViolent2/Code/ServerRecv.cpp:674-721] | force-spawn others / pick their weapons |
| `PLAYER_SHOOT`, `PLAYER_PROJECTILE`, `SHOOT_MELEE` | [VERIFY: BaboViolent2/Code/ServerRecv.cpp:948] [VERIFY: BaboViolent2/Code/ServerRecv.cpp:1029] [VERIFY: BaboViolent2/Code/ServerRecv.cpp:143] | fire on behalf of others |
| `PONG` | [VERIFY: BaboViolent2/Code/ServerRecv.cpp:657-663] | fake someone's ping (range-checked since `46c8572`, still not bound to the sender) |
Contrast with the coord-frame check: [VERIFY: BaboViolent2/Code/ServerRecv.cpp:798]
Fix: resolve the slot from `bbnetID` once at the top of `recvPacket` and ignore the packet's `playerID`.

### Q-S2. Can a malformed packet crash the server? — **Yes (Critical)**
`playerID` is a signed `char` (−128..127) indexing a 32-element array, never range-checked. A value ≥ 32 or < 0 reads outside `players[]`, and a non-null garbage pointer is then dereferenced.
[VERIFY: BaboViolent2/Code/netPacket.h:40] [VERIFY: BaboViolent2/Code/Game.h:34] [VERIFY: BaboViolent2/Code/ServerRecv.cpp:674]
`weaponID` likewise indexes `gameVar.weapons[20]` unchecked: [VERIFY: BaboViolent2/Code/ServerRecv.cpp:966] [VERIFY: BaboViolent2/Code/GameVar.h:312]
The client-side mirror (`createNewPlayerCL`) *does* range-check, which shows the intended pattern: [VERIFY: BaboViolent2/Code/Game.cpp:1692-1695]

### Q-S3. SQL injection? — **Yes (High on master, Medium on Pro server)**
- Master server: player-supplied MAC (from `PLAYER_INFO`) is forwarded by the game server and interpolated into `SELECT … WHERE IP='%s' OR MAC='%s'`. Up to 19 attacker characters reach SQL. [VERIFY: BaboViolent2/Code/ServerRecv.cpp:500-509] [VERIFY: MasterServer/Source/src/cNetManager.cpp:245-251] [VERIFY: MasterServer/Source/src/cMasterServer.cpp:165]
- Pro game server: player name interpolated into `INSERT INTO BadChecksum`. [VERIFY: BaboViolent2/Code/ServerRecv.cpp:1198]
- Game/map names on the master are "escaped" by replacing `'` with a backtick only. [VERIFY: MasterServer/Source/src/cMasterServer.cpp:642-662]
Fix: `sqlite3_prepare_v2` + bound parameters.

### Q-S4. Can a server attack its clients? — **Medium**
`NET_SVCL_SV_CHANGE` is executed as `set <anything>` on the client; `CSystemVariable::command` accepts any registered variable, not just `sv_*`. A malicious server can rewrite client settings (name, key binds, render options, stored account username/password vars), which the client saves back to `main/bv2.cfg` on exit.
[VERIFY: BaboViolent2/Code/ClientRecv.cpp:562-566] [VERIFY: Engine/DukZeven/Code/dksvar.cpp:36-46] [VERIFY: Engine/DukZeven/Code/CSystemVariable.cpp:240-263] [VERIFY: BaboViolent2/Code/GameVar.cpp:562-566] [VERIFY: BaboViolent2/Code/main.cpp:923]
Fix: on the client, reject `SV_CHANGE` unless the variable name starts with `sv_`.

### Q-S5. Are credentials protected? — **No (High)**
- Server password: sent in clear inside `GAMEVERSION_ACCEPTED` (16 bytes). [VERIFY: BaboViolent2/Code/netPacket.h:67-71]
- Admin login (non-Pro): the string `"user pass"` in clear; Pro: unsalted MD5 of each (a replayable password equivalent). [VERIFY: BaboViolent2/Code/ServerRecv.cpp:285] [VERIFY: BaboViolent2/Code/ServerRecv.cpp:212-223]
- Remote admin: plaintext user/pass over UDP; no attempt throttling; afterwards authorised by `peerID` (IP:port), which is spoofable over UDP. [VERIFY: BaboViolent2/Code/CMaster.cpp:381-405] [VERIFY: BaboViolent2/Code/CMaster.cpp:449-465]
- Account password: unsalted MD5 sent to every game server you join, which forwards it to the account server over HTTP. A malicious game server collects reusable hashes. [VERIFY: BaboViolent2/Code/ClientRecv.cpp:272-274] [VERIFY: BaboViolent2/Code/ServerRecv.cpp:463-483]

### Q-S6. Map download path handling — **Medium**
The client-supplied `mapName` (16 bytes, not guaranteed NUL-terminated) is formatted into `main/maps/%s.bvm` and opened. `../` sequences let a client read other `.bvm`-suffixed files outside `main/maps`; the missing terminator can read past the struct.
[VERIFY: BaboViolent2/Code/ServerRecv.cpp:47-56] [VERIFY: BaboViolent2/Code/Server.cpp:1357-1359]

### Q-S7. Amplification — **Medium**
- `UPDATE_SKIN` (Pro): the server broadcasts it once *per connected player* (the loop sends to everyone each iteration): N² messages per request. [VERIFY: BaboViolent2/Code/ServerRecv.cpp:1218-1227]
- `PLAY_SOUND`: any client packet is relayed verbatim to every other client without validation. [VERIFY: BaboViolent2/Code/ServerRecv.cpp:537-551]

### Q-S8. Is the babonet "RND1" hash a security feature? — **No**
No secret; the sequence is derivable from the public source. See `ALGORITHM_01` §3.
[VERIFY: Engine/babonet/Code/cClient.cpp:938-962]

---

## Part D — Robustness defects (crashes / wrong behaviour)

| ID | Sev | Defect | Evidence |
|---|---|---|---|
| R1 | High | `fclose(fic)` outside `if (fic)` in map upload loop → `fclose(NULL)` if a requested map file doesn't exist | [VERIFY: BaboViolent2/Code/Server.cpp:1359-1383] |
| R2 | High | Rocket owner dereferenced without null check (owner disconnected mid-flight) | [VERIFY: BaboViolent2/Code/GameProjectile.cpp:733] [VERIFY: BaboViolent2/Code/GameProjectile.cpp:756] |
| R3 | High | `if (gameVar.sv_serverType = 1)` assignment flips servers to Pro rules | [VERIFY: BaboViolent2/Code/GameProjectile.cpp:718] |
| R4 | High | Master ban answer kicks `players[ID]` without null check | [VERIFY: BaboViolent2/Code/Server.cpp:684-693] |
| R5 | High | Map loader: unknown version leaves `cells` uninitialised while `isValid` stays true; `size` read from file is never bounds-checked (clients load server-supplied maps) | [VERIFY: BaboViolent2/Code/Map.cpp:104] [VERIFY: BaboViolent2/Code/Map.cpp:214-432] [VERIFY: BaboViolent2/Code/Map.cpp:369-371] |
| R6 | Medium | `clientProjectiles.erase(projectiles.begin()+i)` erases with the wrong vector's iterator | [VERIFY: BaboViolent2/Code/Game.cpp:851] |
| R7 | Medium | babonet partial-body receive writes at offset 0 (corrupts messages split over ≥3 reads) | [VERIFY: Engine/babonet/Code/cClient.cpp:743] |
| R8 | Medium | Auto-balance moves one player per cycle, miscounts, and can desync team on approval rejection | [VERIFY: BaboViolent2/Code/Server.cpp:1440-1461] |
| R9 | Medium | `queryNextMap` reloads the current map mid-game and can index an empty vector | [VERIFY: BaboViolent2/Code/Server.cpp:383-387] [VERIFY: BaboViolent2/Code/Server.cpp:423-425] |
| R10 | Medium | `collisionClip` reads neighbour cells before clamping to the map interior | [VERIFY: BaboViolent2/Code/MapRender.cpp:660-686] |
| R11 | Medium | Non-atomic `bool` spin-lock between console thread and game thread | [VERIFY: BaboViolent2/Code/main.cpp:418-451] |
| R12 | Low | CTF dropped-flag radii differ between flags (0.5 vs 0.25); wrong log text for red return | [VERIFY: BaboViolent2/Code/ServerCTF.cpp:99] [VERIFY: BaboViolent2/Code/ServerCTF.cpp:217] [VERIFY: BaboViolent2/Code/ServerCTF.cpp:253] |
| R13 | Low | Pro SND spawn bound check uses `>` instead of `>=` | [VERIFY: BaboViolent2/Code/GameSpawn.cpp:195] |
| R14 | Low | No cap on catch-up steps in `dkcUpdateTimer` | [VERIFY: Engine/DukZeven/Code/dkc.cpp:159-164] |
| R15 | Low | Ping moving average divides a 59-sample sum by 60 | [VERIFY: BaboViolent2/Code/PlayerUpdate.cpp:37-42] |
| R16 | Critical | `FileIO` returns file text through `CString(char* fmt, ...)` (`engine/zeven/include/CString.h:62`), so a `%` in file data is a printf format string; `getString()` writes an unbounded string into `char tmp[256]`. Reached by files from disk and by downloaded maps. Found by CodeQL (`cpp/tainted-format-string`), 2026-10-07 | [VERIFY: game/src/FileIO.cpp:73] [VERIFY: game/src/FileIO.cpp:213] [VERIFY: game/src/FileIO.cpp:225] |
| R17 | High | Config loader reads `ficIn >> variable` into `char variable[256]` with no width: a long token overflows the stack. CodeQL `cpp/dangerous-cin` | [VERIFY: engine/zeven/src/CSystemVariable.cpp:93] [VERIFY: engine/zeven/src/CSystemVariable.cpp:134] |
| R18 | High | Allocation sizes `w*h*3` / `w*h*bpp` computed in `int` before widening; with sizes read from map or texture files they overflow and under-allocate. CodeQL `cpp/integer-multiplication-cast-to-long` | [VERIFY: game/src/Map.cpp:1112] [VERIFY: game/src/Map.cpp:1682] [VERIFY: engine/dko/src/ePTexture.cpp:214] [VERIFY: game/src/CHost.cpp:167] [VERIFY: game/src/CAStar.cpp:91] [VERIFY: game/src/CAStar.cpp:95] [VERIFY: game/src/CAStar.cpp:195] [VERIFY: game/src/tinyxmlparser.cpp:467-494] |
| R19 | Low | `%i` used for an `unsigned long` `bbnetID`. CodeQL `cpp/wrong-type-format-argument` | [VERIFY: game/src/Console.cpp:2253] |
| R20 | Medium | `dksPlayMusic` streams music with `MA_SOUND_FLAG_STREAM`; when the file is missing (placeholder data has no `Menu.ogg`), `ma_sound_init_from_file` fails and frees the stream while miniaudio's job thread still writes to it: heap-use-after-free, timing-dependent. Seen once in the CI smoke job (ASan), 2026-10-07 | [VERIFY: engine/zeven/src/dks.cpp:241] [VERIFY: game/src/Scene.cpp:205] |
| R21 | Critical | Any client crashes the server with a chat message containing `%s`: `console->add(chat.message)` turns it into `CString(char* fmt, ...)`, a printf format string. Found by the step 3 fuzzer; input `tests/corpus/clsv_svcl_chat/crash-format-string.bin` | [VERIFY: game/src/ServerRecv.cpp:521] |
| R22 | Critical | `NET_SVCL_CONSOLE` from any client (before the admin check): `CString adminCommand = buffer` reads the payload as a NUL-terminated format string, so a short unterminated or `%`-bearing message crashes the server. Found by the step 3 fuzzer; input `tests/corpus/svcl_console/crash-unterminated.bin` | [VERIFY: game/src/ServerRecv.cpp:167] |
| R23 | Critical | `NET_CLSV_ADMIN_REQUEST` from any client: `CString loginRecv(adminRequest.login)` uses the login as a printf format. Found by the step 3 fuzzer; input `tests/corpus/clsv_admin_request/crash-format-string.bin` | [VERIFY: game/src/ServerRecv.cpp:190] |
| R24 | High | `NET_CLSV_MAP_REQUEST`: `mtrans.mapName = request.mapName` reads the 16-byte name past its end when it has no `\0` (stack overflow read). Found by the step 3 fuzzer; input `tests/corpus/clsv_map_request/crash-unterminated.bin` | [VERIFY: game/src/ServerRecv.cpp:50] |
| R25 | High | `NET_CLSV_PLAYER_SHOOT` and `NET_CLSV_SVCL_PLAYER_PROJECTILE` dereference a null `player->weapon`: from a joined player who never spawned (`Game::shootSV`), or one who died less than 0.2 s ago (the server accepts shots then). Found by the step 3 fuzzer; no corpus input (state-dependent), the fuzzer hits it in seconds on both types | [VERIFY: game/src/Game.cpp:1220] [VERIFY: game/src/ServerRecv.cpp:925-931] [VERIFY: game/src/ServerRecv.cpp:1017] |
| R26 | Unknown | `NET_CLSV_SVCL_VOTE_REQUEST`: the fuzzer aborts the server (exit 134, no sanitizer report) after many invalid votes; cause not analysed. Not reproduced since the fuzzer spawns its player and frees disconnected slots; `last.txt` now gives the seed and input count for an exact rerun | [VERIFY: game/src/ServerRecv.cpp:94-139] |
| R27 | Medium | babonet `cPacket::~cPacket` freed its `new char[]` buffer with `delete` (undefined behaviour on every sent packet, client and server). Found by Linux ASan in the step 3 master test; fixed in step 3 | [VERIFY: engine/babonet/src/cPacket.cpp:213] |
| R28 | High | On Windows, babonet crashes (access violation) when a TCP connection closes: a client's `bb_clientDisconnect` right after receiving, and the master dropping timed-out clients. macOS and Linux (ASan) are clean. Found by the step 3 master test (CI runs 38073594887, 38074969674); cause not found. The Windows ctest `master_expected_fail_r28` passes while it crashes | [VERIFY: tests/test_master.cpp] [VERIFY: engine/babonet/src/cConnection.cpp] |

---

## Part E — Recommended fix order for a relaunch

1. **Packet hygiene layer** in `Server::recvPacket`: resolve the slot from `bbnetID`, reject out-of-range `playerID`/`weaponID`, enforce expected sizes (fixes Q-S1, Q-S2, W5).
2. **Crash fixes** R1–R5 (each is a one- to three-line change).
3. **SQL** via prepared statements (Q-S3).
4. **Client-side `SV_CHANGE` filter** (Q-S4).
5. **Credentials**: TLS or at minimum challenge-response for server/admin passwords; stop forwarding account hashes to third-party servers (Q-S5).
6. Replace hard-coded `rndlabs.ca` endpoints (Q12).
7. Consider enabling UDP for coord frames (`bb_serverCreate(true, …)`) only after auditing babonet's UDP path, which this analysis did not cover.

---

## Verification checklist

- [x] Every security claim tagged to the handler line that trusts the input
- [x] Each crash defect traced to the exact dereference
- [ ] Exploitability of Q-S2 not demonstrated (analysis only; no PoC built)
- [ ] Master-server receive buffer sizes (`stCacheBanned`) not inspected
