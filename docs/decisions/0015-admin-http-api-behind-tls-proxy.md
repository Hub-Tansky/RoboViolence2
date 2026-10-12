# 0015: Remote admin is an HTTP API behind a TLS reverse proxy

- Status: Accepted
- Date: 2026-10-12
- Affects: [step 12](../roadmap/phase-b-security-infrastructure-anti-cheat/step12-remote-admin.md) §12.1

## Context
- Operators need console commands on `bv2dedicated` without the game client. The old remote admin (`RA_*` over babonet UDP peers, `game/src/CMaster.cpp:390-470`) logs in with the cleartext admin password and then trusts the peer's IP:port.
- The game binary has no TLS: OpenSSL was removed and libcurl is compiled out ([ADR 0002](0002-compile-out-libcurl.md)). libsodium is linked ([ADR 0014](0014-libsodium-hmac-for-credentials.md)).
- The servers ship as containers with a compose example (step 11).

## Decision
`bv2dedicated` serves a small HTTP/1.1 admin API (`POST /command`, `GET /status`) on `zsv_adminApiPort` (0 = off), bound to the compose network only; a Caddy container terminates TLS in front of it. Requests carry the admin password as a bearer token, checked in constant time; the legacy `RA_*` login is refused.

## Consequences
- No TLS or certificate code in the game; Caddy handles certificates. The API port must never be published; only Caddy's is.
- Per-source rate limit and an append-only audit log in the pref dir, secrets masked.
- Rejected: RCON over TLS in the binary (mbedtls: a second crypto library and certificate handling in the game); SSH to the container console (an SSH account per operator, no per-command audit or limits).
