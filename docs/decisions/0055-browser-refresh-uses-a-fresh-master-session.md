# 0055: The server browser's Refresh uses a fresh master session

- Status: Accepted
- Date: 2026-11-15
- Affects: `game/src/CMaster.cpp` (`requestGames`, `sendPacket`), `game/src/CMainTab.*` (`showBrowser`), `tests/test_master_refresh.cpp`, `tests/e2e/e2e.py` (`browser_tab_list`); amends [ADR 0043](0043-first-start-fullscreen-desktop-and-game-browser.md); ROBO-217

## Context
- Owner report, 2026-11-15: a server hosted from the Host tab did not show in a friend's browser until the friend restarted the game. Refresh did nothing.
- The client keeps one TCP session to the master for its whole life; the master never times out a player session (`MasterClient::Update`). A player who browses and then idles keeps a session with no traffic, and a home router or NAT can drop it without telling either end. Refresh then wrote the list request into that dead session and waited forever. babonet has no keepalive or reply timeout, so nothing noticed; a restart opened a new session, which is why it helped.
- `sendPacket` also dropped a list request whenever another message was queued, for example a host's heartbeat waiting for the connection.

## Decision
- `CMaster::requestGames` closes the current session and opens a new one for every list request (Refresh, the Browser tab, `refreshgames`). One TCP connect per click.
- Exception: a game that hosts a server (`RunningServer`) keeps its session. Its heartbeats every 20 s keep the session alive, and the master may still owe it ban answers (`CACHE_ANSWER`).
- A list request always queues, and replaces a list request still queued.
- Every opening of the Browse tab requests the list, as Refresh does. Before, only the menu's first opening did, and only while no master session was open, so the tab kept showing a stale list.

## Consequences
- The master sees one disconnect and one connect per Refresh. Listed games don't depend on the connection that registered them (`cBV2game` times out on missing heartbeats only), so a host's own Refresh would not drop its game either.
- e2e `browser_tab_list`: a server starts after the client's first (empty) list; reopening the Browse tab must list it.
- `tests/test_master_refresh.cpp` runs the game's `CMaster` against the real master through a relay that goes silent, the way a router drops a session, and refreshes while a heartbeat waits for the connection. The hosting exception (`RunningServer`) has no test: it needs a running `Server`, which this test doesn't start.
- Rejected: TCP keepalive on the babonet client (minutes to detect with OS defaults, and per-OS socket options), a reply timeout with retry (Refresh would still wait for the timeout every time the session is dead), and the master pinging idle players (protocol change for every client and the master).
- The earlier rule "keep the master session open for the next refresh" (`pingReceived`) still holds for rows of one listing: no reconnect per row.
