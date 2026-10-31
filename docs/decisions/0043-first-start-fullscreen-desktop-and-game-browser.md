# 0043: A new player starts fullscreen at the desktop resolution, on the Game Browser tab

- Status: Accepted
- Date: 2026-10-30
- Affects: `config/bv2.example.cfg`, `r_resolution` in `game/src/GameVar.cpp`, `dkwInit` in `engine/zeven/src/dkw.cpp`, the resolution list in `game/src/COption.cpp`, the first tab in `game/src/CMainTab.cpp`, per-golden config in `tests/screenshots.py`, the LAN broadcast destination of `SendTo`/`ReceiveFromPeers` in `engine/babonet/src/cPeer2Peer.cpp`; amends [0026](0026-screenshot-goldens.md) (a golden may have a `<name>.cfg`); owner request in the project thread "Set game startup defaults" (2026-10-30), which is the [GUI] approval (ADR 0010)

## Context
- The game's built-in default is fullscreen, but the shipped `bv2.example.cfg` (the first config layer, ADR 0007) said `r_fullScreen false` and `r_resolution 1024 768`, so every new player on Windows, macOS and Linux got a small window.
- None of the fixed resolutions in Options matches most current screens, and an exclusive mode switch to a 4:3 or 16:10 mode on a 16:9 or Retina screen blurs or letterboxes the game.
- The menu opened on News, a hard-coded post (PR #106); players come to find a game.

## Decision
- `r_resolution 0 0` means the desktop's size. `dkwInit` then creates the window at the primary display's desktop mode (in screen coordinates, so HiDPI gets its full pixel count) and, in fullscreen, sets no display mode: fullscreen without a mode switch. Windowed with `0 0` opens maximized and restores to 3/4 of the desktop, centred.
- `bv2.example.cfg` and the built-in defaults are now `r_fullScreen true` and `r_resolution 0 0`.
- Options lists "Desktop" first in both resolution lists (one table per view mode in `COption.cpp`). A saved size that is no row (set in `bv2.cfg` or the console) selects no row and is kept until the player picks one.
- The main menu opens on the Game Browser and requests the game list as clicking its button does (`CMainTab::showBrowser`).

## Consequences
- Only new players get the change: `bv2.cfg` saves every variable, so an existing config keeps its saved `r_fullScreen` and `r_resolution`; a player can't be told apart from one who chose windowed. To get the new defaults, tick Full screen and pick Desktop in Options, or delete `bv2.cfg`.
- Every start now asks the master and the LAN for the game list, also a client started with `connect <host>`. The LAN query no longer looks up the computer's own host name to guess a subnet broadcast address (that lookup blocked the game loop for ~30 s on macOS CI); it sends to the limited broadcast address 255.255.255.255 (`engine/babonet/src/cPeer2Peer.cpp`). Limited broadcast leaves only through the default-route interface, which is no worse than the old classful guess from the host's first address.
- `r_refreshRate` only applies to a fixed resolution in fullscreen.
- Test clients (`tests/autotest_client.py`, the playtest agent) pin `r_resolution 1024 768` windowed, so the screenshot goldens keep their size; the main-menu golden now shows the Game Browser, the options golden the Desktop row. A golden may add config lines in `tests/golden/<name>.cfg` (amends ADR 0026): `desktop` runs fullscreen with `0 0` and must be the 1280x720 Xvfb screen; `options-custom` saves 1000x750 and must show no resolution row selected.
- Rejected: a first-run-only switch (a marker file) that would also move existing players; we can't tell their choice from the old default.
- Rejected: keeping the host-name lookup (null-checked, or moved to the `cDNSquery` thread), which keeps the stall risk and the classful guess; and enumerating interfaces (`getifaddrs`/`GetAdaptersAddresses`) for per-interface directed broadcasts, which is more platform code than a LAN list needs today.
