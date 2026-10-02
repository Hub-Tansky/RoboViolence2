# ALGORITHM 01 — babonet Transport, Framing and Liveness

> Library: `Engine/babonet/Code` (author Marc Durocher, 2006).
> Consumers: `Server.cpp`, `Client.cpp`, `CMaster.cpp`.

---

## 1. Concept

babonet exposes three roles behind a flat C API:

| Role | API | Used for |
|---|---|---|
| Server | `bb_serverCreate/Update/Send/Receive/DisconnectClient` | game server |
| Client | `bb_clientConnect/Update/Send/Receive` | game client, master-server link |
| Peer (P2P UDP) | `bb_peerBindPort/Update/Send/Receive` | LAN discovery, remote admin |

[VERIFY: Engine/babonet/Code/baboNet.h:98-148]

`bb_serverUpdate` returns a *signed* connection event: positive = new client's babonetID, negative = `-babonetID` of a client that disconnected, 0 = nothing, `BBNET_ERROR (-999999999)` = fatal.
[VERIFY: Engine/babonet/Code/baboNet.h:118] [VERIFY: Engine/babonet/Code/baboNet.h:45]
The game server branches on exactly these cases: [VERIFY: BaboViolent2/Code/Server.cpp:447-534]

---

## 2. Wire format of a TCP batch

`cClient::Send` drains the whole TCP send queue into one buffer (3072 bytes) and prefixes it with a 9-byte key block.
[VERIFY: Engine/babonet/Code/cClient.cpp:823-845]

```
offset  size  field
0       4     "RND1"                       (RND_KEY)
4       4     hashed sequence number       (see §3)
8       1     NbPacket = number of messages in this batch
9       …     repeated NbPacket times:
                u16 Size  | u16 TypeID | Size bytes of payload (the netPacket.h struct)
```

- Key constants: [VERIFY: Engine/babonet/Code/cClient.h:36-37]
- Header struct `{Size, typeID}`: [VERIFY: Engine/babonet/Code/cPacket.h:43-47]
- Per-message header + payload copy: [VERIFY: Engine/babonet/Code/cClient.cpp:853-866]
- `NbPacket` is patched into byte 8 just before sending: [VERIFY: Engine/babonet/Code/cClient.cpp:872] [VERIFY: Engine/babonet/Code/cClient.cpp:910]

### Batching rule

Messages are appended until `packed >= DataRate`; the batch is then sent and the function **returns**, leaving the rest of the queue for the next update. Otherwise, whatever accumulated is sent at the end.
[VERIFY: Engine/babonet/Code/cClient.cpp:869-900] [VERIFY: Engine/babonet/Code/cClient.cpp:907-931]

`DataRate` is described as the per-send cap ("default 1400"): [VERIFY: Engine/babonet/Code/cClient.h:93]

Complexity per call: `O(k)` memcpy for the `k` messages that fit in the batch. At most one batch above `DataRate` per update, so throughput per client is bounded by `DataRate × updates/s` (≈ 1400 × 30 ≈ 42 KB/s at defaults). Since `updateNet` runs twice per server tick, the real bound may be 2× that ([VERIFY: BaboViolent2/Code/Server.cpp:750] [VERIFY: BaboViolent2/Code/Server.cpp:1337]).

> Buffer bound: the 3072-byte buffer is safe only while `DataRate + largest message + 9 < 3072`. With `DataRate` 1400 and the largest game message ~250 bytes (`net_svcl_map_chunk`) this holds.
> [VERIFY: Engine/babonet/Code/cClient.cpp:828] [VERIFY: BaboViolent2/Code/netPacket.h:550-554]

---

## 3. The "hashed sequence number"

Each batch carries 4 characters derived from a per-connection counter:

```
LastPacketID += 1
digest = MD5( decimal_string(LastPacketID) )           // 32 hex chars
id     = digest[1] digest[3] digest[5] digest[7]        // 4 hex chars
```
[VERIFY: Engine/babonet/Code/cClient.cpp:938-962]

The receiver computes the same thing from its own `PendingID` counter and **rejects the stream** (returns 1 = "potential hacker") if the 4 characters differ, then increments `PendingID`.
[VERIFY: Engine/babonet/Code/cClient.cpp:773-797] [VERIFY: Engine/babonet/Code/cClient.cpp:604-607]

What this is and isn't:

| Property | Assessment |
|---|---|
| Detects replayed/injected batches from a naïve third party | Yes, if they don't know the counter |
| Cryptographic authentication | **No.** No secret key: anyone with the (GPL) source computes the same sequence from 1 upward |
| Integrity of the payload | **No.** Payload bytes are not hashed |
| Entropy | 4 hex chars = 16 bits |

It is best understood as an anti-tamper speed bump against packet-editing tools of the era, not as security.

---

## 4. Receive state machine (`cClient::ReceiveStream`)

TCP delivers an arbitrary byte stream; `ReceiveStream` re-frames it with three states:

```
          ┌──────────────┐  9 bytes     ┌────────────────┐ 4 bytes  ┌───────────────┐
 start ──►│WaitingForKey │─────────────►│WaitingForHeader│─────────►│ reading body  │
          └──────▲───────┘ check "RND1" └───────▲────────┘ Size>0   │ (Size bytes)  │
                 │         check hash           │ Size==0           └──────┬────────┘
                 │                              │ (emit packet)            │ emit packet
                 └──── NbPacket reaches 0 ◄──────┴─────────────────────────┘
```

- Key state incl. partial-key handling: [VERIFY: Engine/babonet/Code/cClient.cpp:549-613]
- Header state incl. partial-header handling: [VERIFY: Engine/babonet/Code/cClient.cpp:615-710]
- Body state: [VERIFY: Engine/babonet/Code/cClient.cpp:711-751]
- A finished message becomes a `cPacket` on `ReceivedPackets`: [VERIFY: Engine/babonet/Code/cClient.cpp:725]
- `NbPacket` counts down to 0, then the machine expects a new key: [VERIFY: Engine/babonet/Code/cClient.cpp:731-738]

### ⚠ Bug: body split across 3+ reads is corrupted

When a message body does not fit in the current `recv` chunk, the partial copy always writes at **offset 0**:

```cpp
memcpy(lastPacket.data, buf + nread, nbytes - nread);        // line 743
bytesRemaining = bytesRemaining - (unsigned short)(nbytes - nread);
```
[VERIFY: Engine/babonet/Code/cClient.cpp:743-744]

The *completing* copy uses the correct offset `lastPacket.data + (Size - bytesRemaining)` ([VERIFY: Engine/babonet/Code/cClient.cpp:717]).

- 2 reads: first partial copies to offset 0 (correct by coincidence), final copy lands at the right offset → OK.
- 3+ reads: the 2nd partial copy overwrites bytes at offset 0 → payload corrupted.

Because game messages are small (<300 bytes), a body rarely spans three TCP reads, which is why this went unnoticed. Fix: `memcpy(lastPacket.data + (lastPacket.Size - bytesRemaining), …)`.

### Other properties

- `Size == 0` messages (type-only signals like `NET_SVCL_AUTOBALANCE`) are emitted without a body: [VERIFY: Engine/babonet/Code/cClient.cpp:686-707] [VERIFY: BaboViolent2/Code/Server.cpp:1260]
- The receive queue is FIFO: `AddReceivedPacket` pushes at the head and `GetReadyPacket` pops from the tail, so messages reach the game in arrival order. Each pop walks the whole list, i.e. `O(n)` per message and `O(n²)` to drain `n` queued messages: [VERIFY: Engine/babonet/Code/cClient.cpp:758-771] [VERIFY: Engine/babonet/Code/cClient.cpp:799-821]
- A receiver allocates `new char[Size]` with an attacker-chosen `Size ≤ 65535`: bounded, not exploitable by itself: [VERIFY: Engine/babonet/Code/cClient.cpp:682-685]

---

## 5. TCP vs UDP selection

```
cServer::Send(data, size, type, destination, protocol):
    isUDP = UDPenabled ? (protocol != 0) : false
    destination < 1  → queue on every client
    else             → queue on getClientByID(destination)
```
[VERIFY: Engine/babonet/Code/cServer.cpp:880-906]

The game server is created with `UDPenabled = false`, so **every** message uses TCP ([VERIFY: BaboViolent2/Code/Server.cpp:153]). See `03_DATA_FLOW.md` §5 for the consequences.

Broadcast is `destination = 0`, which is also the default argument: [VERIFY: Engine/babonet/Code/baboNet.h:120]

---

## 6. Application-level liveness: ping in frames

The game does not use babonet timeouts to detect dead clients; it runs its own ping on top.

```
per player i, per server tick:
  skip if status == LOADING, or DEAD and never spawned (timeAlive == 0)   # since 46c8572
  if !waitForPong:
      if currentPingFrame >= 30:              # once per second
          currentPingFrame = 0; waitForPong = true
          send NET_SVCL_PING(i)
  else:
      ping = max(ping, currentPingFrame)       # grows while waiting
      if currentPingFrame > 30:                # >1 s without pong
          currentCF = netCF1; connectionInterrupted = true; sendPosFrame = 0
      if currentPingFrame > 300:               # 300 frames = 10 s
          disconnect
  currentPingFrame++
on PONG(i): waitForPong = false; ping = currentPingFrame   # i range-checked since 46c8572
```
Since `46c8572` the watchdog is also reset (`waitForPong = false`, `currentPingFrame = 0`) on `GAMEVERSION_ACCEPTED` and on a team change. The client answers `PING` before its join gate, echoing the slot from the ping.
- Loop: [VERIFY: BaboViolent2/Code/Server.cpp:1018-1078]
- Pong handler: [VERIFY: BaboViolent2/Code/ServerRecv.cpp:653-666]

**Units.** At 30 Hz ([VERIFY: BaboViolent2/Code/main.cpp:495]), `ping` is a round-trip in frames (≈33.3 ms each). The code comment on the disconnect threshold says "3sec", but `300` frames is **10 seconds**.
[VERIFY: BaboViolent2/Code/Server.cpp:1057]

### Smoothed ping (`Player::updatePing`)

A 60-entry ring buffer sampled every `pingLogInterval = 0.05 s` keeps a running sum:
[VERIFY: BaboViolent2/Code/PlayerUpdate.cpp:29-48] [VERIFY: BaboViolent2/Code/Player.cpp:61]

```
pingLog[k] = ping
pingSum   += pingLog[k]
pingSum   -= pingLog[(k+1) mod 60]         # the oldest sample
avgPing    = max(1, pingSum / 60)
```

Derivation. After the write at slot `k`, the sum contains the samples from steps `k-58 … k` (59 samples); the one at `k+1` (written 59 steps ago) has just been subtracted and will be overwritten next. So:

$$\text{avgPing} = \frac{1}{60}\sum_{j=0}^{58} \text{ping}_{k-j} \approx \tfrac{59}{60}\,\overline{\text{ping}}$$

a slight (1.7 %) under-estimate of the 3-second window mean. `avgPing` directly sets the coord-frame send interval (§ `03_DATA_FLOW.md` 4).

### Max-ping enforcement

`ping * 33 > sv_maxPing` (ms) for more than `maxTimeOverMaxPing = 5 s` → player is moved to spectator (not kicked):
[VERIFY: BaboViolent2/Code/Server.cpp:1084-1098] [VERIFY: BaboViolent2/Code/Server.cpp:40]

---

## 7. Design rationale (as evidenced)

| Choice | Evidence | Effect |
|---|---|---|
| Everything on TCP | `bb_serverCreate(false,…)` | No packet loss handling in game code; latency spikes under loss (head-of-line blocking) |
| Ping-scaled send rate | `sendPosFrame >= avgPing` | Avoids flooding a TCP stream that cannot drain faster than one RTT |
| Quantised structs (short ×100, char ×10) | `netPacket.h` | Small messages (~30 bytes per coord frame) |
| Raw `memcpy` of structs | every handler | Zero serialisation cost; ABI/endianness coupled |

---

## 8. Verification checklist

- [x] Batch layout read from `cClient::Send`
- [x] Hash algorithm read from `GetLastPacketID` / `GetPendingID`
- [x] Receive state machine traced line by line; partial-body offset bug confirmed at `cClient.cpp:743`
- [x] `UDPenabled` path traced from `Server::host` → `cServer::Send`
- [ ] `cServer::ReceivePacketsFromClients` / `UpdateConnections` not traced line by line (connection accept thread)
- [ ] UDP P2P (`cPeer2Peer.cpp`) reliability layer not analysed
