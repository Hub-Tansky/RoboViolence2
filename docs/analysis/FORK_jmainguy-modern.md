# Fork review: Jmainguy/BaboViolent2 `modern`

Verdict: **don't merge. Harvest individual fixes.** Done: see [Harvest list](#harvest-list-implemented-2026-10-02).

Reviewed 2026-10-01 against a local copy at `../various/BaboViolent2-modern` (no `.git`) and a clone of `https://github.com/Jmainguy/BaboViolent2` (`modern` @ `9b2516af`). Paths below are relative to the fork root unless prefixed with `ours:`.

## Lineage

```
Daivuk/BaboViolent2 master (7171c84)   == ours, byte-identical except our docs/AGENTS.md
  └─ Daivuk "modern" (6 commits, 2018-09-09..12): code moved to src/, CMake, SDL2 window/input, imgui, profile UI removed
       └─ Jmainguy "modern" (36 commits, 2026-05-30..06-02, partly Cursor-generated): Linux servers, net fixes, CI, packaging
```

- Diff from our tree: 1041 files, +250k/−170k lines (layout move plus vendored deps). There is no common file layout to merge into.
- The fork targets Linux dedicated and master servers with a public master at `babo.soh.re`. The client is a secondary target.

## What the fork changes

| Area | Change | Evidence |
|---|---|---|
| BaboNet 64-bit | `UINT4` = `unsigned int` on LP64; handshake and packet header written as fixed 4-byte fields | `src/Engine/Babonet/global.h:45`, `cIncConnection.cpp:147`, `cClient.cpp:860` |
| BaboNet bugs | `sprintf(LastError, LastError)` format-string bug removed; `select()` uses the max fd, not `Listener+1`; `FD_CLR` on client removal; send failure returns the disconnected ID; 10 s handshake timeout; hostname resolution in `bb_clientConnect` | `cClient.cpp`, `cServer.cpp`, `cConnection.cpp` |
| Join handshake | Handles `GAMEVERSION` arriving before `NEWPLAYER` (`pendingVersionAccept`). Pong sent immediately, before the join gate | `src/Game/ClientRecv.cpp:95`, `Client.h` |
| Ping watchdog | Skips players who are loading or haven't spawned yet; resets on team change | `src/Game/Server.cpp:934` |
| Anti-cheat | Speed-hack kick removed (false kicks on uncapped FPS). PRO executable checksum query removed from the server | `src/Game/ServerRecv.cpp:813`, `:482` |
| Accounts | HTTP auth (`CCurl`) and account UI removed. `player_info.username/password` sent empty | `ServerRecv.cpp:482`, `src/Menu2/CUserLogin.cpp` |
| Map download | Client skips the download if `main/maps/<name>.bvm` exists locally. No checksum, so a different local map with the same name desyncs | `ClientRecv.cpp:429`, `src/Game/Map.cpp` |
| Master | Master source copied from ours (`ours:MasterServer/Source/src`) with fixes: sends all rows at once, keeps the TCP session across refreshes, null-checks SQLite results. Default master hard-coded to `babo.soh.re:10207` | `src/MasterListingServer/cMasterServer.cpp`, `src/Game/Master/CMaster.cpp:945` |
| Portability | POSIX `GetMapList` and `CHost` map list, lowercase asset paths, `pthread_t` on macOS, null guards in menus | `Map.cpp`, `src/Menu2/CHost.cpp` |
| Audio | SDL2_mixer backend, defined **only on Linux** when the package is found. Windows and macOS builds have no audio | `CMakeLists.txt:148`, `src/Engine/Zeven/dks/dks.cpp:33` |
| Debug | `c_netlog` / `BV2_NETLOG` `[net]` tracing across client, server and master | `Client.cpp`, `Server.cpp` |
| Build/CI | CMake 3.10, C++11, git submodules (SDL, curl, imgui) and vendored SQLite. CI: Linux native plus Windows via MinGW. **macOS removed from CI** (`5f50bbbc`). release-please packaging | `.github/workflows/ci.yml` |

## Protocol impact

| Layer | Compatible with 2.11 Pro? | Why |
|---|---|---|
| Game packets | Yes | `netPacket.h` differs from ours only in `#ifdef` → `#if defined`. `GAME_VERSION_SV/CL` still 21100 (`src/Game/Server.h:33`, `Client.h:23`) |
| BaboNet transport | Yes | The LP64 fix makes 64-bit builds emit the same bytes as the original 32-bit Win32 build. Before `97b2ce7`, ours got a 4-byte `UINT4` only when `LINUX64` was defined, so other LP64 builds such as macOS used 8 bytes. Ported now |
| Master protocol | **No** | `stBV2list.Version` and `stBV2row.Version` grow from `char[5]` to `char[16]` (`src/Game/Master/cMSstruct.h:276,309`, `src/Version.h:12`). Fork servers and clients only work with the fork master |
| Behaviour | Partly | Servers no longer enforce PRO checksums, accounts or speed-hack kicks. Old clients still connect |

Breaking the protocol is fine for us: Phase A already allows it ([../refactoring/phase-a-modern-portable-build/README.md](../refactoring/phase-a-modern-portable-build/README.md), "Protocol"). The fork keeps the game wire format; it doesn't fix it. Step 4's fixed-width `netPacket.h` with size asserts and its single version bump are still needed, because the fork has neither and dropped macOS arm64 (unsigned `char`) from CI.

## Why not merge

- **Layout:** `src/` flat layout vs our target `engine/ game/ masterserver/` ([../refactoring/phase-a-modern-portable-build/README.md](../refactoring/phase-a-modern-portable-build/README.md#target-repository-layout-ai-ready)). A merge would be a rewrite either way.
- **Encoding damage:** Latin-1 bytes were replaced by U+FFFD in 68 `src` files (41 already damaged in Daivuk `modern`). The French comments can't be recovered from the fork. Our step 1 §1.4 conversion is lossless.
- **Conflicting decisions:** SDL2 + SDL2_mixer + submodules vs our SDL3 + miniaudio + vcpkg. Windows and macOS are silent. No macOS CI.
- **Rule violations:** committed binaries (`src/MasterListingServer/libBaboNet.a`, `libDKC.a`, `linuxmaster`, `Content/*.dll`) and a hard-coded public host (`babo.soh.re`), both banned by our Phase A rules.
- **Anti-cheat removed** rather than fixed. Future phases need it ([../refactoring/future-phases.md](../refactoring/future-phases.md)).

## Harvest list: implemented 2026-10-02

All eight fixes are ported, one commit each, on top of the sanitised baseline `5eb7aad`. Each commit message credits the fork commit (GPLv3). BaboNet fixes are applied to both copies: `Engine/babonet/Code` and `MasterServer/Source/src`. The game wire format is unchanged (`GAME_VERSION` still 21100).

| Fix | Fork source | Commit | Notes |
|---|---|---|---|
| BaboNet fixed 4-byte handshake/header fields | `bf8392a0` | `97b2ce7` | Also the `%ld`→`%lu` MD5 packet-ID input and the master's `sizeof(size_t)` key offset |
| `sprintf(LastError, …)` format string, `select` max fd, `FD_CLR`, disconnect return | `bf8392a0` | `b33d79c` | Beyond the fork: `bb_serverUpdate` passes the send-side `-(NetID)` through, so the slot is cleared |
| Hostname resolution in `bb_clientConnect` | `049b68e8` | `860d382` | |
| Handshake ordering, immediate pong, ping-watchdog exemptions | `bf8392a0` | `46c8572` | Account fields still sent; `PONG` `playerID` range-checked |
| POSIX `GetMapList`, `CHost` map list, lowercase asset paths | `bf8392a0` | `6618202` | `hit.wav` path fixed in code (no duplicate asset); `'/'` in map paths |
| Master: send all rows, keep session, SQLite null-checks, `IP[16]` zero-init | `ac302e90`, `bf8392a0` | `b19bc06` | No default public master; `AccountURL` no longer a format string |
| `c_netlog` tracing | `bf8392a0` | `c733d52` | Join path only, no per-packet log; env `BV2_NETLOG` |
| Speed-hack false kicks fixed, check kept | `ServerRecv.cpp:813` comment | `47e2974` | Our design: 5 + 10% frame tolerance; speed check skipped for 2 s after a hit (knockback) |

Not ported: local-map skip without a checksum, removal of accounts and checksums, the `Version[16]` change on its own (fold it into our single protocol bump), packaging and release-please, the `babo.soh.re` defaults, and the 0.5 s `ConnCheck` throttle removal in `bb_serverUpdate`.

Verification: each commit compiles on macOS (clang, LP64), for BaboNet and for the dedicated (`CONSOLE`) variant of the game files touched, with no new errors. Client-only files were checked with stub Windows/FMOD headers, comparing errors before and after. The master server still can't build as a unit (its `src/` lacks the master-client class header; step 2), so its files were checked with shim headers. Nothing was run end-to-end.
