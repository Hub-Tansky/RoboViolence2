# Master server

Registry of game servers for the lobby list, with a ban list, backed by SQLite (`master.db`, `web.db`).

- Sources: `Source/src/`. Networking comes from `Engine/babonet` and `Engine/DukZeven` (dkc); the master keeps no copies.
- `CClient` / `CServer` are master-specific variants of the babonet client/server classes. Their names differ from `Engine/babonet/Code/cClient.*` / `cServer.*` only by case, which collides on macOS and Windows; rename them before Step 2 builds both together.
- `cMSstruct.h` here is an older subset of `BaboViolent2/inc/cMSstruct.h`; reconcile in Step 2.
- Schema seeds: `content-seed/master.sql`, `content-seed/web.sql`.
- No build system until Step 2. The autotools outputs and the 2012 binaries were removed.
