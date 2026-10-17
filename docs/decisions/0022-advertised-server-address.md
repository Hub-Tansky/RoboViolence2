# 0022: The master lists an advertised address only from private sources

- Status: Accepted
- Date: 2026-10-17
- Affects: [Phase C step 6](../roadmap/phase-c-installers-and-fixes/step6-server-and-master.md) task 6.3; PNS-44

## Context
- The master files a game under the heartbeat's source address. A server in Docker beside the master (the compose example) or behind NAT shows up at an address remote players can't reach.
- Trusting any advertised address would let anyone list a fake server under someone else's IP.

## Decision
- `sv_publicAddress` (env `BV2_PUBLIC_ADDRESS`, dotted IPv4) goes into the heartbeat's `ip` field.
- The master uses it only when the heartbeat comes from a private or loopback address (10/8, 127/8, 172.16/12, 192.168/16); otherwise it keeps the source address. Owner-approved default (2026-10-17).
- `KILL_SERV` finds the game by the connection that registered it and its port, not by the listed address.

## Consequences
- A co-located or LAN server can be listed at its public address without a list of trusted servers on the master.
- A host on the master's own private network can list any address; the master's network is the operator's.
- Rejected: a configured allow-list of servers on the master (more config for a small group of servers).
