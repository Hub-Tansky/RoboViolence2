# Step 3: Packet fuzz/replay harness and master-server tests

**Status:** IN PROGRESS

**Depends on:** [Step 1](step1-playtest-builds-and-build-guides.md). **Next:** [step4-packet-hygiene.md](step4-packet-hygiene.md). **Index:** [README.md](README.md)

## Scope

| Field | Value |
|---|---|
| Goal | ctest can feed any byte sequence to `Server::recvPacket` and to the master server, replay a recorded corpus, and fuzz each message type under ASan |
| In scope | Tasks 3.1–3.4 below |
| Out of scope | Fixing the defects the harness finds (steps 4–7); the gameplay replay recording of C.9 (PNS-13); changing packet layouts |
| Allowed paths | `tests/**`, `tools/fuzz-*`, `game/CMakeLists.txt` (test hooks only), `masterserver/CMakeLists.txt` (test hooks only), `.github/workflows/build.yml` (fuzz job), `docs/analysis/KEY_QUESTIONS.md` (defects found), `ARCHITECTURE.md`, `docs/roadmap/**` |
| Inputs | `AGENTS.md`, `ARCHITECTURE.md`, earlier review records `reviews/step*.md`, `docs/analysis/KEY_QUESTIONS.md` Parts C–D, `game/src/netPacket.h`, `tests/CMakeLists.txt`, `tests/smoke_server.py`, `masterserver/README.md` |
| Deliverables | In-process harness target, corpus in `tests/corpus/`, master register/list tests, a CI fuzz job (not required) |
| Definition of done | See "Acceptance checks". All must pass. `ARCHITECTURE.md` is updated |

## Context

- `Server::recvPacket(char*, int typeID, unsigned long bbnetID)` (`game/src/ServerRecv.cpp:38`) is the single entry point for client messages. It `memcpy`s raw structs from `game/src/netPacket.h`.
- The only tests today: `tests/test_netpacket.cpp` (layout), `test_config.cpp`, `test_fileio.cpp` and `smoke_server.py` (start/quit).
- `MasterClient` is reconstructed and its 60 s timeout is an assumption (PNS-4, `masterserver/README.md`). The master builds SQL with `sprintf` (`masterserver/src/cMasterServer.cpp`, 21 sqlite calls).
- Steps 4–7 need a failing test before each fix.

## Tasks

### 3.1 In-process server harness

- A test library that builds the `CONSOLE` server sources with a fake babonet send layer. It starts a `Game` on a placeholder map and exposes `deliver(typeID, bytes, bbnetID)` and the captured outgoing packets.

### 3.2 Corpus and replay

- `tests/corpus/<message>/*.bin`: one valid sample per message type, built from `netPacket.h` structs by a generator.
- A replay test runs every corpus file through `deliver` and checks there is no crash and the expected replies.

### 3.3 Fuzzer

- A libFuzzer target where available (Clang), plus a portable random-mutation runner for MSVC/GCC. Runs per message type for a time budget; crashes are saved into the corpus.
- CI job `fuzz` (Linux, ASan, 2 min per message type, not required).

### 3.4 Master-server tests

- Start `bv2master` on a loopback port with a scratch `master.db`. Test register, list, heartbeat timeout and server removal.

## Critical files

- `game/src/ServerRecv.cpp`, `game/src/netPacket.h`, `game/CMakeLists.txt`
- `masterserver/src/cNetManager.cpp`, `masterserver/src/cMasterServer.cpp`
- `tests/CMakeLists.txt`

## Acceptance checks

```bash
ctest --test-dir build/linux-x64-asan --output-on-failure -R "replay|master"
```

- Passes. Known-crashing inputs (Q-S2 out-of-range `playerID`) are present in the corpus and marked expected-failure until step 4.
- The CI `fuzz` job runs and uploads any new crash inputs as artifacts.
- `tools/review.sh --full <preset>` passes, and the fresh-context review record `reviews/step3.md` and the raw reviewer report `reviews/step3-report.md` (with the skill's load line) exist ([REVIEW.md](../../../REVIEW.md) section 5).
- `ARCHITECTURE.md` lists every file added, moved or removed in this step.
- The secret scan (`gitleaks git --staged` / CI) is clean.
