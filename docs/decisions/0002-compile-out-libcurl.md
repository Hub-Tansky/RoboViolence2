# 0002: Compile out libcurl in Phase A

- Status: Accepted
- Date: 2026-10-02
- Affects: [step 3 §3.1, §3.3](../roadmap/phase-a-modern-portable-build/step3-dependency-upgrades.md), [PNS-15](../roadmap/possible-new-scope.md#pns-15-account-and-ladder-system-service-endpoints)

## Context

- `CCurl` (`BaboViolent2/Code/CCurl.cpp`) is the only libcurl user. It talks to the account/ladder backend (`gameVar.db_accountServer`, read from `bv2.db` at `GameVar.cpp:49-52`) and to match-report URLs (`Server.cpp:982-988`, behind `sv_report`, default off).
- That backend was `ladder.rndlabs.ca`, which is gone. Step 1 already blanks its host, so account login is disabled until configured.
- Calls: server auth on join (`ServerRecv.cpp:491`), client status and friends (`CStatus.cpp:97`, `CFriends.cpp:185`), report upload (`Server.cpp:985`).
- Failed auth is non-fatal: the player stays with `userID` 0; the kick is commented out (`Server.cpp:638-643`).
- Upgrading means configuring TLS on three OSes (Schannel, OpenSSL) for a service that doesn't exist.

## Decision

Add the CMake option `BV2_WITH_HTTP`, default **OFF**. When OFF, `CCurl.cpp` is replaced by a stub with the same `CCurl.h` API whose requests finish at once with 0 bytes received, and `curl` is not in `vcpkg.json`. When ON, link vcpkg `curl` 8.x as step 3 §3.3 originally described.

## Consequences

- No libcurl and no TLS stack in Phase A builds; one fewer dependency on all three OSes. The callers stay unchanged.
- Accounts, friends, ladder stats and report upload are inactive in Phase A, as they already are after step 1.
- Callers must handle an empty response without crashing; `CStatus` and `CFriends` need checking (`Server.cpp:605` already does).
- An own account/stats service (PNS-14, PNS-15) turns `BV2_WITH_HTTP` on, or replaces `CCurl` with a client chosen then.
- Rejected: upgrade now (cost with no user); delete `CCurl` and its callers (larger diff in UI code that §D may keep).
