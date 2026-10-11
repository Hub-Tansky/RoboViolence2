# 0013: Restore the Normal and Pro rulesets (fix R3)

- Status: Accepted
- Date: 2026-10-11
- Affects: [Phase B step 5](../roadmap/phase-b-security-infrastructure-anti-cheat/step5-crash-fixes.md) task 5.1; KEY_QUESTIONS R3

## Context
- `sv_serverType` selects the ruleset: 0 Normal, 1 Pro (`game/src/GameVar.cpp:366`). Pro changes weapons (`Game::UpdateProSettings`), the shotgun spread (`Game.cpp`), Photon and flame hits (`Player.cpp`), rocket damage and remote detonation (`GameProjectile.cpp`, `ServerRecv.cpp`).
- Step 1.7 removed the compile-time `_PRO_` switch (the non-Pro protocol); the runtime `sv_serverType` branches stayed.
- `GameProjectile.cpp:714` reads `if ((gameVar.sv_serverType = 1))`: an assignment. The first rocket of a game switches the server (and each client running the same code) to Pro, so a server that set `sv_serverType 0` has played Pro since then. The CTF launch script sets 0 (`content/LaunchScript/CTF.cfg:41`); the example config sets 1 (`config/bv2.example.cfg:28`).

## Decision
Make it a comparison, so `sv_serverType 0` plays the Normal rules again (owner, 2026-10-11).

## Consequences
- Servers that set 0, including the CTF launch script, change gameplay: rocket damage 0.75 instead of `sv_zookaDamage`, no remote rocket detonation, the Normal shotgun, Photon and flame rules. Players who knew those servers as Pro will notice.
- Servers that want today's behaviour set `sv_serverType 1`.
- Changing `sv_serverType` reapplies the weapon table on the server (`Console::sendCommand` → `Game::UpdateProSettings`), as clients do on `SV_CHANGE`; before, the assignment hid the missing server update.
- Rejected: Pro only (remove the Normal branches). It matches what has actually been played, but drops a ruleset the variable promises.
