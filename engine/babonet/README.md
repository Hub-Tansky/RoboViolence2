# babonet

Networking library: TCP (with optional UDP) client/server, a UDP peer-to-peer manager, async DNS and a thread wrapper.

- **API:** `include/baboNet.h`, prefix `bb_*` (`bb_serverCreate`, `bb_serverSend`, `bb_clientConnect`, ...). `include/CThread.h` (thread wrapper), `include/md5class.h` (`CMD5`, hex MD5), `include/cMSstruct.h` (master-server protocol structs).
- **Internals:** `src/` (`cClient`, `cServer`, `cConnection`, `cPeer*`, `cPacket`, `md5c`). The master server also reads `src/cPacket.h` and `src/cConnection.h`.
- **Depends on:** `zeven_core`, pthreads; `ws2_32` and `iphlpapi` on Windows.
- **Used by:** `bv2`, `bv2dedicated`, `bv2master`.

## Gotchas

- Wire fields are 32-bit (`INT4`/`UINT4`, `platform.h`). The handshake and packet headers are fixed 4-byte fields.
- Connection handles are `babonetID`, not game `playerID`; `bb_serverSend` destination 0 is broadcast.
- Protocol analysis: [../../docs/analysis/ALGORITHM_01-Networking.md](../../docs/analysis/ALGORITHM_01-Networking.md).
