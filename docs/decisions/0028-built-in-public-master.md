# 0028: The client and server ship with the public master built in

- Status: Accepted
- Date: 2026-10-20
- Affects: [ADR 0007](0007-data-root-pref-dir-config-layers.md) config layers; first public release (v0.0.2)

## Context
- `cl_masterServers` shipped empty, so a fresh install had no server browser and its servers listed nowhere until the player set `BV2_MASTER_SERVERS`.
- The owner decided (2026-10-20) that production runs one master at `master.roboviolence.com:10207` and that the client must point at it by default.
- The first-run `bv2.cfg` of builds up to 0.0.1 saved `cl_masterServers ""`. Read back as layer 2, it would hide any new default.

## Decision
- The built-in value of `cl_masterServers` is `master.roboviolence.com:10207` (`game/src/GameVar.cpp`), for the client and the dedicated server.
- The master list is part of the release, not a player setting: `bv2.cfg` neither saves it nor sets it. `config/local/*.cfg`, `--config <file>` and `BV2_MASTER_SERVERS` still override it.
- `none` turns the master list off (`BV2_MASTER_SERVERS=none`, or `cl_masterServers "none"` in a local file). Tests and CI start servers and clients with `none`, so they never register with the public master.

## Consequences
- A player's console change to `cl_masterServers` lasts until quit.
- A dedicated server with `sv_gamePublic true` (the default) lists itself on the public master after its first 20 s heartbeat. A private test server sets `BV2_MASTER_SERVERS=none` or `sv_gamePublic false`.
- Moving the master to another name needs a client release; the DNS record is the cheap way to move it to another host.
- Host names resolve on the main thread (`cClient`, babonet). A slow resolver delays the server browser refresh and a server's heartbeat; resolving off the main thread is follow-up work.
