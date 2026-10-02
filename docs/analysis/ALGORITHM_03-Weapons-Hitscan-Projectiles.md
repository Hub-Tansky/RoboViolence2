# ALGORITHM 03 — Weapons, Hitscan, Projectiles and Damage

> Server-authoritative combat: the client *requests* a shot; the server re-traces it, applies damage, and broadcasts the result.

---

## 1. Weapon table

`Weapon(dko, sound, fireDelay, name, damage, impressision, nbShot, reculVel, startImp, weaponID, projectileType)`
[VERIFY: BaboViolent2/Code/Weapon.cpp:33]

Values from the client build (the dedicated-server copy is identical except where noted):
[VERIFY: BaboViolent2/Code/GameVar.cpp:272-300] [VERIFY: BaboViolent2/Code/GameVar.cpp:303-331]

| ID | Weapon | fireDelay (s) | damage (life = 1.0) | spread ° (max / start) | rays | recoil | type |
|---|---|---|---|---|---|---|---|
| 0 | SMG | 0.10 | 0.10 | 8 / 1 | 1 | 0.5 | hitscan |
| 1 | Shotgun | 0.85 | 0.21 | 20 / 12 | 5 | 3.0 | hitscan |
| 2 | Sniper | 2.00 | 0.30 | 0 / 0 | 1 (2–3 at runtime) | 3.0 | hitscan |
| 3 | Dual MG | 0.10 | 0.13 | 10 / 2 | 1 | 0.8 | hitscan |
| 4 | Chain gun | 0.10 | 0.19 | 15 / 5 | 1 | 2.0 | hitscan |
| 5 | Bazooka | 1.75 | 0.75 | 0 | 1 | 3.0 | rocket |
| 6 | Photon rifle | 1.50 | 0.24 | 0 | 1 | 5.0 | piercing hitscan |
| 7 | Flame thrower | 0.10 | 0.08 | 10/10 client, **6/6 dedicated** | 1 | 0 | short hitscan |
| 8 | Grenade | 1.00 | 1.50 (radial) | — | 1 | -1 | grenade |
| 9 | Molotov | 1.00 | 0.15 | — | 1 | -1 | molotov → flames |
| 10 | Knives | 1.00 | 0.60 (radius 1) | — | — | 0 | melee |
| 11 | Nuke bot | 12.0 | 8.0 | — | — | 0 | melee/bot |
| 12 | Shield | 3.0 | 0 | — | — | 0 | buff |
| 13 | Minibot (`_PRO_`) | 1.0 | 0.05 | — | — | 0 | bot |

- Weapon ID constants: [VERIFY: BaboViolent2/Code/GameVar.h:27-41]
- Flame-thrower spread differs between builds: [VERIFY: BaboViolent2/Code/GameVar.cpp:295] vs [VERIFY: BaboViolent2/Code/GameVar.cpp:326]
- Sniper uses 3 rays when the camera is zoomed out (`camPos.z >= 10`), else 2: [VERIFY: BaboViolent2/Code/PlayerUpdate.cpp:569-581] [VERIFY: BaboViolent2/Code/ServerRecv.cpp:958-964]
- Pro-mode tuning overrides nuke/shield delay and chain-gun recoil at runtime: [VERIFY: BaboViolent2/Code/Game.cpp:185-200]
- Several damages are overridable on `sv_serverType == 1` servers (`sv_smgDamage`, `sv_sniperDamage`, …): [VERIFY: BaboViolent2/Code/Player.cpp:1116-1138]

---

## 2. Hitscan pipeline

### 2.1 Client: `Weapon::shoot` → `Game::shoot`

1. Cooldown / overheat / photon-charge gates: [VERIFY: BaboViolent2/Code/Weapon.cpp:288-311]
2. Muzzle position = model nuzzle offset rotated by yaw; aim direction = yaw rotated `(0,1,0)`: [VERIFY: BaboViolent2/Code/Weapon.cpp:370-377]
3. Recoil: [VERIFY: BaboViolent2/Code/Weapon.cpp:381]
4. `Game::shoot` builds the request: [VERIFY: BaboViolent2/Code/Game.cpp:1096-1129]

What goes in `p2` depends on the build:

| Build | `p1` | `p2` sent | Spread computed by |
|---|---|---|---|
| `_PRO_` | muzzle ×100 | **direction** ×100 | server |
| non-Pro | muzzle ×100 | client-computed endpoint (with client-side random spread) ×100 | client |

[VERIFY: BaboViolent2/Code/Game.cpp:1118-1126]

In non-Pro, the client also loops `nbShot` times with its own spread: [VERIFY: BaboViolent2/Code/Weapon.cpp:397-406]

### 2.2 Server: rate limiting (`NET_CLSV_PLAYER_SHOOT`)

[VERIFY: BaboViolent2/Code/ServerRecv.cpp:944-1024]

- Accepted from alive players, or dead for < 0.2 s (last-gasp shots): [VERIFY: BaboViolent2/Code/ServerRecv.cpp:951]
- Shotgun/sniper: first ray allowed if `elapsed + 0.061 > fireDelay`; subsequent rays in the same burst counted up to 5 (shotgun) / 3 (sniper): [VERIFY: BaboViolent2/Code/ServerRecv.cpp:966-984]
- Automatics: allowed if `elapsed + 0.051 (0.04 CG) > fireDelay`: [VERIFY: BaboViolent2/Code/ServerRecv.cpp:1000-1016]

The fudge terms (0.04–0.061 s ≈ 1–2 frames) absorb jitter between client and server ticks.

> ⚠ `gameVar.weapons[playerShoot.weaponID]` indexes a 20-element array with a client-supplied `char` that is never range-checked; the server also trusts `weaponID` here instead of the player's actual `weapon->weaponID`, so the rate limit can be evaluated against the wrong weapon's `fireDelay`.
> [VERIFY: BaboViolent2/Code/ServerRecv.cpp:966] [VERIFY: BaboViolent2/Code/GameVar.h:312]

### 2.3 Server: ray construction (`Game::shootSV`)

[VERIFY: BaboViolent2/Code/Game.cpp:1221-1287] then per-ray [VERIFY: BaboViolent2/Code/Game.cpp:1359-1585]

Under `_PRO_`:

```
dir = p2 (unit direction from client)
range = 128           (flame thrower: sv_ftMaxRange scaled down the longer it fires)
p2 = dir * range
p2 = rotate(p2, U(-imp, +imp), Z)          # yaw spread
p2 = rotate(p2, U(0, 360), dir)            # roll around the aim axis
p2.z *= 0.5
p2 += p1
```
[VERIFY: BaboViolent2/Code/Game.cpp:1392-1418]

Spread growth: `currentImp += 3`, clamped to `impressision`, once per trigger pull: [VERIFY: BaboViolent2/Code/Game.cpp:1281-1285]

Shotgun (Pro): five rays at fixed yaw offsets −10°, −5°, 0°, +5°, +10°; the offset index is smuggled through the `imp` argument (1…5) and decoded by `ident = (int)imp`: [VERIFY: BaboViolent2/Code/Game.cpp:1238-1251] [VERIFY: BaboViolent2/Code/Game.cpp:1364-1391]
Its range is then clamped per pellet: in non-Pro mode outer pellets get 1/3 and inner pellets 2/3 of `sv_shottyRange`; in Pro mode `sv_shottyDropRadius / sin θ`: [VERIFY: BaboViolent2/Code/Game.cpp:1422-1459]

### 2.4 Server: hit resolution

```
if wall between player centre and muzzle: nudge p1 off the wall         # anti shoot-through
if rayTest(p1, p2): p2 = wall hit point                                  # clip at wall
for every other alive player j:
    if segmentToSphere(p1, p2, pos_j, 0.25):    # mutates p2 → closest point
        hitPlayer = j
hitPlayer.hitSV(weapon)
broadcast NET_SVCL_PLAYER_SHOOT {p1, p2, normal ×120, hitPlayerID}
```
- Muzzle-in-wall nudge: [VERIFY: BaboViolent2/Code/Game.cpp:1464-1467]
- Wall clip: [VERIFY: BaboViolent2/Code/Game.cpp:1470-1473]
- Player loop: [VERIFY: BaboViolent2/Code/Game.cpp:1536-1555]
- Damage + broadcast: [VERIFY: BaboViolent2/Code/Game.cpp:1559-1583]

**Why the last match is the nearest.** `segmentToSphere` shortens `p2` to the closest point on the segment whenever it reports a hit:
[VERIFY: BaboViolent2/Code/Helper.cpp:476-510]
So after a hit on player A, the segment ends at A; a later player B can only match if it intersects that shorter segment, i.e. lies at least as close to `p1` as A. The loop therefore converges on (approximately) the nearest target in O(P).

The ordinary hitscan branch does **not** filter teammates; friendly fire is decided later in `hitSV` (§4). Photon/flame branches filter by team up front: [VERIFY: BaboViolent2/Code/Game.cpp:1492-1496]

> ⚠ Degenerate ray: `segmentToSphere` divides by `l = |p2 − p1|`. If a wall is hit at the muzzle (`p2 == p1`), `u /= 0` yields NaNs; the comparison then fails and no hit registers, so it's harmless but undefined-behaviour-adjacent.
> [VERIFY: BaboViolent2/Code/Helper.cpp:479-481]

### 2.5 Photon rifle and flame thrower (piercing)

For these, `p3` is reset to the full `p2` after every hit, so **every** player on the line is damaged (piercing):
[VERIFY: BaboViolent2/Code/Game.cpp:1475-1513]

The photon rifle additionally leaves a lingering beam: `incShot = 30`, and every 3rd frame for 1 s the server re-tests the same segment and deals `damage/2`:
[VERIFY: BaboViolent2/Code/Game.cpp:1477-1482] [VERIFY: BaboViolent2/Code/Game.cpp:397-431]

Upper bound on photon damage to one target: 0.24 initial + 10 × 0.12 lingering = 1.44 (> full life), if the target stays in the beam for a whole second.

---

## 3. Projectiles

### 3.1 Spawn

Client sends `NET_CLSV_SVCL_PLAYER_PROJECTILE {position ×100, vel ×10 (a unit direction), type}`; the server validates cooldown, ammo and (for rockets) that the player holds a bazooka, then creates the authoritative instance and relays the packet to everyone.
- Handler: [VERIFY: BaboViolent2/Code/ServerRecv.cpp:1025-1102]
- Rocket requires bazooka in hand: [VERIFY: BaboViolent2/Code/ServerRecv.cpp:1041-1048]
- Pro remote detonation (second trigger pull detonates the rocket in flight): [VERIFY: BaboViolent2/Code/ServerRecv.cpp:1049-1057]
- Ammo check for grenades / molotovs: [VERIFY: BaboViolent2/Code/GameSpawn.cpp:440-449]
- Server instance `remoteEntity = false`, client mirrors `remoteEntity = true`: [VERIFY: BaboViolent2/Code/GameSpawn.cpp:472] [VERIFY: BaboViolent2/Code/GameSpawn.cpp:521]

> The server trusts the client's spawn **position**: nothing checks it against the player's position, so a modified client can spawn a rocket anywhere on the map (still subject to cooldown).

### 3.2 Initial velocities (constructor)

| Type | Initial velocity | Lifetime | Evidence |
|---|---|---|---|
| Rocket | dir × 2.5 | 10 s | [VERIFY: BaboViolent2/Code/GameProjectile.cpp:71-86] |
| Grenade | dir × 5, +5 up | 2 s fuse | [VERIFY: BaboViolent2/Code/GameProjectile.cpp:159-168] |
| Molotov | dir × 6, +2 up | 10 s | [VERIFY: BaboViolent2/Code/GameProjectile.cpp:170-178] |
| Life pack / dropped weapon / dropped grenade | given | 20 / 30 / 25 s | [VERIFY: BaboViolent2/Code/GameProjectile.cpp:180-211] |

### 3.3 Integration (`Projectile::update`)

- **Rocket**: speed capped at 10, then `vel += vel·dt·3` (exponential acceleration, ×e³ per second until the cap), explicit Euler position: [VERIFY: BaboViolent2/Code/GameProjectile.cpp:458-472]
  From 2.5 u/s, `v(t) = 2.5·e^{3t}` reaches the cap at `t = ln(4)/3 ≈ 0.46 s`.
- **Molotov / flame**: ballistic, `g = 9.8`: [VERIFY: BaboViolent2/Code/GameProjectile.cpp:474-490]
- **Grenade & pickups**: ballistic; on `rayTest` hit: position = hit + 0.01·n, `vel = reflect(vel, n) · 0.65`; they stop when slow and on the ground: [VERIFY: BaboViolent2/Code/GameProjectile.cpp:601-644]

### 3.4 Detonation (server only, `!remoteEntity`)

| Type | Trigger | Effect | Evidence |
|---|---|---|---|
| Grenade | fuse expires | `radiusHit(pos, 3, WEAPON_GRENADE)`, `EXPLOSION` r=1.5 | [VERIFY: BaboViolent2/Code/GameProjectile.cpp:647-705] |
| Rocket | babo within 0.25 (not owner), wall hit, or remote-detonate | `radiusHit(pos, zookaRadius)`; default 3.0 | [VERIFY: BaboViolent2/Code/GameProjectile.cpp:715-775] |
| Molotov | babo within 0.25 or wall | spawns flame projectiles | [VERIFY: BaboViolent2/Code/GameProjectile.cpp:813-925] |
| Flame | babo within 0.5 | sticks to player, periodic `radiusHit(.5)` | [VERIFY: BaboViolent2/Code/GameProjectile.cpp:493-560] |
| Life pack | babo within 0.25 | `life += 0.5` (max 1) | [VERIFY: BaboViolent2/Code/GameProjectile.cpp:927-945] |
| Dropped grenade | babo within 0.25 | `nbGrenadeLeft += 1` (max 3) | [VERIFY: BaboViolent2/Code/GameProjectile.cpp:947-965] |

### 3.5 Radial damage (`Game::radiusHit`)

[VERIFY: BaboViolent2/Code/Game.cpp:1592-1637]

For each player within `radius` of the blast whose centre has line-of-sight (`!rayTest`):

$$\text{damage} = \begin{cases} D_w & \text{sameDmg (knives)}\\ \left(1 - \dfrac{d}{r}\right) D_w & \text{otherwise}\end{cases}$$

Linear falloff from full damage at the centre to 0 at `r`. With grenade `D = 1.5`, `r = 3`: lethal (≥1.0) within `d ≤ 1`.
The owner is included (self-damage) except for knives: [VERIFY: BaboViolent2/Code/Game.cpp:1603-1606]
If the owner's slot is empty (they disconnected), **no one** takes damage: [VERIFY: BaboViolent2/Code/Game.cpp:1599]

### 3.6 Deletion & sync

Server deletes after a one-tick grace (`reallyNeedToBeDeleted`) and broadcasts `DELETE_PROJECTILE {uniqueID}`:
[VERIFY: BaboViolent2/Code/Game.cpp:819-839]
Per-tick coord-frame sync for projectiles exists but is commented out; clients simulate projectiles locally from the spawn packet: [VERIFY: BaboViolent2/Code/Server.cpp:1194-1215]

---

## 4. Damage application (`Player::hitSV`)

[VERIFY: BaboViolent2/Code/Player.cpp:1111-1490]

```
cdamage = explicit damage, or weapon default (or sv_*Damage on pro servers)
photon (pro): distance-shaped multiplier, 4 curve families (sv_photonType)
flame (pro) : (1 - dist / sv_ftMaxRange) * sv_ftDamage
if protection > 0.6 : cdamage *= 0.5          # shield
if immuneTime > 0.3 : cdamage = 0             # spawn protection
if same team (TDM/CTF):
    apply only if sv_friendlyFire or self-damage; reflected damage optional
else:
    apply
life -= cdamage ; broadcast PLAYER_HIT {damage = remaining life}
if life <= FLT_EPSILON: kill, drop life pack + weapon + grenades, update score
```
- Shield / immunity: [VERIFY: BaboViolent2/Code/Player.cpp:1179-1187]
- Photon distance curves (arctan, hyperbolic, Lorentzian): [VERIFY: BaboViolent2/Code/Player.cpp:1150-1166]
- Friendly-fire branch: [VERIFY: BaboViolent2/Code/Player.cpp:1199-1222]
- Death drops: life pack + dropped weapon + one dropped grenade per remaining grenade: [VERIFY: BaboViolent2/Code/Player.cpp:1254-1300]
- `PLAYER_HIT.damage` carries the remaining life ("** New, la vie restante **"): [VERIFY: BaboViolent2/Code/netPacket.h:273]

Photon Pro curves, with `a = sv_photonVerticalShift`, `c = sv_photonDamageCoefficient`, `b = sv_photonHorizontalShift`, `m = sv_photonDistMult`, `x` = distance:

| `sv_photonType` | multiplier |
|---|---|
| 1 | `a + c·(π/2 − atan((x − b)·m))` |
| 2 | `a + c / ((x − b)·m)` |
| 3 | `a + c / (1 + ((x − b)·m)²)` |
| other | `c` |

---

## 5. Known defects in this subsystem

| # | Defect | Evidence |
|---|---|---|
| W1 | `if (gameVar.sv_serverType = 1)` is an **assignment**: every projectile update forces pro mode and overwrites bazooka damage with `sv_zookaDamage` | [VERIFY: BaboViolent2/Code/GameProjectile.cpp:718-725] |
| W2 | Rocket hit paths dereference `scene->server->game->players[fromID]` without a null check → server crash if the shooter disconnects while the rocket flies | [VERIFY: BaboViolent2/Code/GameProjectile.cpp:733] [VERIFY: BaboViolent2/Code/GameProjectile.cpp:756-759] |
| W3 | `map && A \|\| B \|\| C \|\| D` precedence: for pickup types the `map` null-guard does not apply | [VERIFY: BaboViolent2/Code/GameProjectile.cpp:612] |
| W4 | Client-side projectile list erases with an iterator from the **other** vector: `clientProjectiles.erase(projectiles.begin()+i)` (UB) | [VERIFY: BaboViolent2/Code/Game.cpp:851] |
| W5 | `weaponID` from the network indexes `gameVar.weapons[20]` unchecked | [VERIFY: BaboViolent2/Code/ServerRecv.cpp:966] [VERIFY: BaboViolent2/Code/ServerRecv.cpp:1069] |
| W6 | Projectile spawn position is client-controlled | [VERIFY: BaboViolent2/Code/GameSpawn.cpp:464-472] |
| W7 | Non-Pro builds accept the client-computed ray endpoint (client decides spread) | [VERIFY: BaboViolent2/Code/Game.cpp:1122-1126] [VERIFY: BaboViolent2/Code/Game.cpp:1392-1418] |

W1 is the most consequential: `sv_serverType` becomes 1 as soon as any projectile is updated (on server and clients alike), which activates every `sv_serverType == 1` branch in `hitSV` (per-weapon `sv_*Damage` overrides, photon/flame distance curves) and the Pro rocket remote-detonation path. An admin who sets `sv_serverType 0` will see it flip back to 1 at the next grenade or rocket. Fix: `==`.
[VERIFY: BaboViolent2/Code/Player.cpp:1116] [VERIFY: BaboViolent2/Code/ServerRecv.cpp:1049]
