# Step 10 review

Reviewer: fresh-context subagent, /thermo-nuclear-code-quality-review, 2026-10-12
Diff: 0ff5b65...7a48dba
Skill evidence: `SKILL: Launching skill: thermo-nuclear-code-quality-review` (raw report: [step10-report.md](step10-report.md))

## review.sh

`tools/review.sh --full macos-arm64` and `--full macos-arm64-asan` at c8d68c6: all checks PASS, 16/16 ctests.

## Findings

| # | Finding | Outcome |
|---|---|---|
| 1 | `sv_password` in cleartext in the master heartbeat (relayed to every browser) and the LAN reply | fixed in 9d000dd (`*` when set); not covered by a test (master state is private), in the step notes |
| 2 | A 20-byte `GAMEVERSION` breaks the "wrong version" message across builds | fixed in 9d000dd: `NET_SVCL_JOIN_CHALLENGE` (134) carries the nonce; `GAMEVERSION` stays 4 bytes |
| 3 | `sv_password` guard is a one-off in Console | fixed in 9d000dd: `dksvarIsSecret` in `GameVar::sendOne` (both) and `Server::sendSVChange`; the truncation message no longer echoes the password |
| 4 | Tests exercise the server only | accepted, stated in the step notes; the owner's join and admin test covers the client |
| 5 | `sodium_init` not called on the client | fixed in 9d000dd (`ready()` before every use) |
| 6 | Nonce and proof sizes defined twice; `joinNonce` uninitialised | fixed in 9d000dd (static_asserts against `Credentials.h`; `= {}`) |
| 7 | `prove` allocates | fixed in 9d000dd (feeds the HMAC state) |
| 8 | KEY_QUESTIONS cites an amended commit and over-claims; ARCHITECTURE stale | fixed in c8d68c6 |
| 9 | Master protocol compatibility undocumented | fixed in c8d68c6 (step notes: deploy together; old MAC-only bans lapse) |
| 10 | 49940c5 also carries the relay blanking and the cache index fix | rejected: pushed history not rewritten; both named in the step notes |

Pre-existing items from "Not checked": PNS-43 (`PLAYER_INFO` before the handshake, IPs relayed, the master's `roxbabo`).

## Fix proof

- Join: with the server's check skipped, `test_join_password.cpp:71,75` fail.
- Admin: with the server's check skipped, `test_admin_login.cpp:60` fails.
- Relay: without the blanking, `test_join_password.cpp:82` fails.
- Variable change: without the `sendSVChange` guard, `test_join_password.cpp:109` fails.
- The `sendOne` guard has no failing test: `dksvarGetFormatedVar` already sends `***`.

## Not done / unsure

- Acceptance: the owner joins a password-protected server and logs in as admin on one OS; the step stays IN PROGRESS until then.
- `macos-universal` (triplet `universal-osx`) builds libsodium with autotools for two architectures; CI on this PR shows whether it builds.
