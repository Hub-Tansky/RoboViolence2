# 0038: Dead account pages are hidden; admin and master ban secrets meet NIST SP 800-63B-4

- Status: Accepted
- Date: 2026-10-27
- Affects: `game/src/CProfile.cpp`, `CMainTab.cpp`, `ClientRecv.cpp`, `ServerRecv.cpp`, `Server.h`, `Console.cpp`, `masterserver/src/cNetManager.cpp`, both `cMSstruct.h`; amends [0010](0010-bv2-asset-compat-and-gui-freeze.md) (owner-approved [GUI] exception) and [0014](0014-libsodium-hmac-for-credentials.md)

## Context
- Player accounts don't work: the master has no account handlers and never sends `CLIENT_HASH`, so `CMaster::hasValidHashSeed()` is false and nothing is sent. The client would hash with unsalted MD5, cap passwords at 16 characters and set no minimum. The Profile tab still offered My Account, Friends, Players, Statistics, Register Clan and Clans; only the Babo appearance page works.
- Every join sent the MD5 of `cl_accountPassword` to the game server.
- NIST SP 800-63B-4 (2025): at least 15 characters for a password used alone, limit failed attempts, salted key derivation for stored passwords. OWASP ASVS 5.0 V6.2: minimum 8, 15 recommended.
- In-game admin login needed 12 characters and dropped a connection after 5 failures, but a reconnect started over. The master ban password was at most 7 characters, in clear on the wire, with no attempt limit.

## Decision
- Owner-approved [GUI] exception (2026-10-27, project thread "Check account password rules vs 2026 standards"): hide the Profile sub-tab row and rename the tab "Appearance" (Host narrows from 90 to 80 so the row keeps its width); keep the account code for a later, safe rewrite (Project item "Player accounts - restore tab and functions, make it safe"). The client sends zeros instead of the password MD5.
- `zsv_adminPass` needs 15 characters for in-game admin login. From the 5th failure from one IP, that IP's admin logins are refused for 60 s, doubling per failure up to an hour, across reconnects.
- `cacheban`, `cachebanremote` and `cacheunban` send `Time` and an HMAC-SHA256 keyed by `BV2_MASTER_BAN_PASS` over the packet with the proof zeroed. The master needs a 15+ character secret and a proof within 5 minutes of its clock. Every packet the master reads must have its exact wire size.

## Consequences
- `stCacheBan` grows from 68 to 96 bytes and `stCacheUnban` from 12 to 38: a hard cut for bans between old game servers and a new master, which refuses the old sizes. Bans come only from the owner's servers.
- Prod's `BV2_MASTER_BAN_PASS` and any admin password under 15 characters must be replaced before deploying this.
- A captured ban packet can be replayed within 5 minutes; bans and unbans repeat harmlessly. One captured proof allows offline guessing of the ban password, as for admin proofs, so the length matters.
- The admin HTTP API keeps its per-source token bucket and no length check.
- No throttle on wrong ban proofs: with a 15+ character key online guessing is hopeless, and a public throttle would let anyone block bans.
- With `BV2_WITH_HTTP=ON`, `CStatus.cpp` and `CFriends.cpp` would still send the unsalted MD5 of `cl_accountPassword` to `db_accountServer`; the player-accounts item removes them.
- Rejected: master bans from localhost only. Game servers run in containers and reach the master over the Docker network.
