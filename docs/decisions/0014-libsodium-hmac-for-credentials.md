# 0014: Use libsodium for the credential HMAC and nonces

- Status: Accepted
- Date: 2026-10-12
- Affects: [step 10](../roadmap/phase-b-security-infrastructure-anti-cheat/step10-server-credentials.md) §10.1

## Context
- The join password crosses the wire in cleartext (`net_clsv_gameversion_accepted::password`, `game/src/netPacket.h:73`); the admin login sends unsalted MD5 (`net_clsv_admin_request`, `:86-89`). KEY_QUESTIONS Q-S5.
- A challenge-response needs HMAC-SHA256, a CSPRNG for the server's nonce and a constant-time compare. OpenSSL was removed in Phase A; nothing linked provides them.
- PNS-14 (accounts) will need a password KDF later.

## Decision
Link vcpkg `libsodium` (static on every triplet) and use `crypto_auth_hmacsha256_*`, `randombytes_buf` and `sodium_memcmp`, behind `game/src/Credentials.h`.

## Consequences
- One small dependency on every target; its `crypto_pwhash` covers PNS-14's KDF.
- macOS and Linux build it with autotools, already a documented prerequisite.
- Rejected: mbedtls (a larger API; the nonce needs its entropy and DRBG set-up); an in-repo SHA-256 (our own crypto code and a random source per OS).
