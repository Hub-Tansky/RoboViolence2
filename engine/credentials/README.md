# credentials

HMAC-SHA256 proofs, nonces and constant-time checks, so the join, admin and master ban passwords never cross the wire.

- **API:** `include/Credentials.h`, namespace `credentials`: `prove`/`check` answer a server's nonce (join, admin; ADR 0014); `signPacket`/`checkPacket` sign a whole wire struct with a `Proof[32]` field (master bans; ADR 0038); `MIN_SECRET_LENGTH` (15, NIST SP 800-63B-4).
- **Depends on:** libsodium.
- **Used by:** `bv2`, `bv2dedicated`, `bv2master`, tests (`test_credentials`, `test_master`).

## Gotchas

- `signPacket` hashes the struct's bytes as sent, padding included; wire structs are `#pragma pack(1)`, so there is none.
