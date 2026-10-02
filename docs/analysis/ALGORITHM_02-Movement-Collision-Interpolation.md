# ALGORITHM 02 — Movement, Collision and Remote Interpolation

> Covers: local babo physics (client), grid collision (`MapRender.cpp`), ray casting (`Map.cpp`), remote-entity smoothing (`CoordFrame::interpolate`), and the server's speed-hack check.

---

## 1. Local player physics (client-authoritative)

Runs only for the locally controlled babo (`remoteEntity == false`), every 1/30 s.

### 1.1 Integration and friction

```
position += vel * dt                                    # explicit Euler
s = |vel|
if s > 0:
    k = 1 if (snow theme && sv_slideOnIce && on ice tile) else 4
    s = max(0, s - k * dt)                              # linear (Coulomb-like) friction
    vel = normalize(vel) * s
position.z = 0.25                                       # babo radius; the game is planar
```
[VERIFY: BaboViolent2/Code/PlayerUpdate.cpp:252-279]

Friction is a **constant deceleration** (4 u/s², or 1 u/s² on ice), not a multiplicative damping.

### 1.2 Input acceleration (`controlIt`)

```
accel = 12.5            (4.0 on ice)
absolute mode: vel.y += ±accel*dt (W/S), vel.x += ±accel*dt (D/A)
scope mode   : same along aim "front" / "right" vectors
clamp |vel| ≤ 3.25
```
- Acceleration constants: [VERIFY: BaboViolent2/Code/PlayerUpdate.cpp:476-480]
- Absolute (default) controls: [VERIFY: BaboViolent2/Code/PlayerUpdate.cpp:533-548]
- Speed clamp at 3.25: [VERIFY: BaboViolent2/Code/PlayerUpdate.cpp:655-660]

**Terminal speed derivation.** Holding one key: `dv/dt = 12.5 − 4 = 8.5 u/s²` until the clamp; so top speed 3.25 u/s is reached in ≈0.38 s. Released, it decays at 4 u/s² → stops in ≈0.81 s. Diagonals are clamped to the same 3.25 (no diagonal speed bonus).

### 1.3 Recoil and knock-back

- Firing pushes the shooter backward: `vel -= aimDir * reculVel`: [VERIFY: BaboViolent2/Code/Weapon.cpp:381]
- Server `PLAYER_HIT` carries a knock-back velocity added to the local player: [VERIFY: BaboViolent2/Code/ClientRecv.cpp:862-869]

These can momentarily exceed 3.25 u/s, because the clamp only runs inside `controlIt`.

### 1.4 Orientation

Yaw = angle between +Y and the aim vector, with sign from X:
[VERIFY: BaboViolent2/Code/PlayerUpdate.cpp:339-345]

$$\theta = \arccos(\hat d_y)\cdot\frac{180}{\pi},\qquad \theta \leftarrow -\theta \text{ if } \hat d_x > 0$$

The visual "rolling ball" rotates the orientation matrix about `right = move × Z` by `π·|Δpos|` radians per tick. A ball of radius 0.25 rolling without slipping would turn `|Δpos| / 0.25 = 4·|Δpos|` rad, so the visual roll runs at π/4 ≈ 0.79 of the physical rate (purely cosmetic):
[VERIFY: BaboViolent2/Code/PlayerUpdate.cpp:348-358]

---

## 2. Grid collision

Called for the local player after integration, and for minibots on the server:
[VERIFY: BaboViolent2/Code/Game.cpp:630-633] [VERIFY: BaboViolent2/Code/Game.cpp:551-552]

### 2.1 `performCollision(lastCF, CF, radius)` — swept, axis-separated

[VERIFY: BaboViolent2/Code/MapRender.cpp:425-648]

```
(x, y) = floor(CF.position), clamped to [1, size-2]
Y axis (only the side we move toward):
   for each of the 3 cells in the neighbouring row (x-1, x, x+1):
      if wall AND the swept AABB overlaps it (uses lastCF.x for the X extent):
          CF.y = wall edge ± (radius + ε)
          vel.y = -vel.y * 0.45                      # BOUNCE_FACTOR
X axis: same with the neighbouring column, using lastCF.y for the Y extent
lastCF.position = CF.position
```

- Clamp of the probe cell: [VERIFY: BaboViolent2/Code/MapRender.cpp:463-473]
- Moving −Y, three wall checks: [VERIFY: BaboViolent2/Code/MapRender.cpp:475-516]
- Bounce: [VERIFY: BaboViolent2/Code/MapRender.cpp:487]
- `COLLISION_EPSILON 0.05`, `BOUNCE_FACTOR 0.45`: [VERIFY: BaboViolent2/Code/Map.h:98-99]

Why mix `lastCF` and `CF`: resolving Y using the *old* X extent (and vice versa) is a cheap separating-axis trick that prevents snagging on corners when sliding along a wall.

Limitations (from the code shape):
- Only the 3×3 neighbourhood of the *destination* cell is checked, so a displacement > 1 cell per tick can tunnel. At the 3.25 u/s clamp and 1/30 s ticks, displacement is ≈0.11 cells, so in normal play this cannot happen; knock-back velocities are char-quantised to ≤12.7 u/s → ≤0.42 cells/tick, still safe.
- With a 3D map (`dko_mapLM`), the grid is bypassed and `dkoSphereIntersection` is iterated with a slide response: [VERIFY: BaboViolent2/Code/MapRender.cpp:427-457]

### 2.2 `collisionClip(CF, radius)` — penetration fix-up

[VERIFY: BaboViolent2/Code/MapRender.cpp:650-720]

1. Push out of the four orthogonal neighbours if overlapping: [VERIFY: BaboViolent2/Code/MapRender.cpp:658-680]
2. Clamp to the map border ring: [VERIFY: BaboViolent2/Code/MapRender.cpp:683-686]
3. If the centre is inside a wall cell, eject toward the nearest passable side: [VERIFY: BaboViolent2/Code/MapRender.cpp:689-720]

> ⚠ Step 1 indexes `cells[y*size + (x±1)]` **before** step 2 clamps `x, y`. If a position ever arrives with `x = 0` or `y = 0` (e.g. a bad knock-back), `x-1 = -1` reads outside the row (and at `y = 0`, `(y-1)*size` reads before the array).
> [VERIFY: BaboViolent2/Code/MapRender.cpp:665] [VERIFY: BaboViolent2/Code/MapRender.cpp:675]

### 2.3 Babo-vs-babo

Client-side only, for the local player: if another alive babo (both alive > 3 s) is within 0.5 (two radii), push to 0.51 apart and reflect velocity with the bounce factor, then re-run map collision:
[VERIFY: BaboViolent2/Code/Game.cpp:609-628]

The 3-second grace period avoids spawn-overlap pushes: [VERIFY: BaboViolent2/Code/Player.h:241-242]

### 2.4 Stuck-in-wall recovery

If after collision the local player's cell is a wall (or outside a 3D map), the client asks the server to respawn it:
[VERIFY: BaboViolent2/Code/Game.cpp:636-664]

---

## 3. Ray casting (`Map::rayTest`)

Used for hitscan, explosions line-of-sight, projectile collision.
[VERIFY: BaboViolent2/Code/Map.cpp:1431-1578]

### 3.1 Algorithm

A dominant-axis grid march (a simplified DDA):

```
if 3D map: delegate to dkoRayIntersection (scaled ×10), plus a floor test
if start cell is a wall below its height: hit at p1
axis = X if |Δx| > |Δy| else Y ; direction = sign
loop:
    stop (no hit) if (i,j) left the map or passed p2 along the dominant axis
    test cells (i,j), and its two neighbours across the minor axis
    step one cell along the dominant axis, recompute the minor index from the line equation
```
- Early exit inside wall: [VERIFY: BaboViolent2/Code/Map.cpp:1466-1480]
- Dominant axis choice: [VERIFY: BaboViolent2/Code/Map.cpp:1484-1506]
- March + 3-cell test (X case): [VERIFY: BaboViolent2/Code/Map.cpp:1527-1537]

Testing the two minor-axis neighbours compensates for the line crossing into an adjacent row between dominant-axis steps (a true DDA would visit exactly those cells).

### 3.2 Per-cell test (`rayTileTest`)

[VERIFY: BaboViolent2/Code/Map.h:415-521]

For the cell box `[x, x+1] × [y, y+1] × [0, h]`:

1. Passable cell: only the floor plane `z = 0` can be hit: [VERIFY: BaboViolent2/Code/Map.h:427-447]
2. Wall cell: test the top face `z = h`, then the four side faces, each with a parametric intersection

$$t = \frac{|x_1 - p_{1,x}|}{|p_{2,x} - p_{1,x}|},\qquad p = p_1 + t\,(p_2 - p_1)$$

and a bounds check on the other two coordinates. On a hit, **`p2` is overwritten with the hit point** and `normal` is set to the face normal.
[VERIFY: BaboViolent2/Code/Map.h:466-477]

Output contract: `rayTest` returns true and shortens `p2` in place. Callers rely on this mutation, e.g. `shootSV` clips the bullet at the wall: [VERIFY: BaboViolent2/Code/Game.cpp:1470-1473]

Complexity: `O(L)` cells for a ray of length `L` cells (≤ 3 tile tests per step).

---

## 4. Remote-entity interpolation

### 4.1 Ingest (`Player::setCoordFrame`)

On each received `COORD_FRAME` for a remote player:
[VERIFY: BaboViolent2/Code/Player.cpp:1505-1549]

```
drop if out of order (packet.frameID < netCF1.frameID)
netCF0          = currentCF          # start the new curve where we are *now*
netCF0.frameID  = netCF1.frameID     # …but timestamp it with the previous key
cFProgression   = 0
currentCF.vel   = packet.vel
netCF1          = packet (position/100, vel/10, aim/100, frameID)
if netCF0.frameID == 0: netCF0 = netCF1  # first packet: snap
```

Starting from `currentCF` (not the old `netCF1`) guarantees C⁰ continuity: the drawn babo never jumps on packet arrival.

### 4.2 Evaluate (`CoordFrame::interpolate`)

[VERIFY: BaboViolent2/Code/Player.h:78-137]

```
progress += 1
N = to.frameID - from.frameID          # sender frames between the two keys
if progress > N:                        # past the newest key
    if progress < 15: position += vel * dt     # dead-reckon
    else:             position  = to.position  # give up, snap
elif progress >= 0:
    t = progress / N
    T = N / 90                                  # "animTime"
    if cl_cubicMotion:
        position = Bezier(from.pos, from.pos + from.vel*T, to.pos - to.vel*T, to.pos, t)
    else:
        position = to.position                  # no smoothing
```

`cubicSpline` is the cubic Bernstein (Bézier) form:
[VERIFY: BaboViolent2/Code/Helper.cpp:435-438]

$$B(t) = (1-t)^3 P_0 + 3t(1-t)^2 P_1 + 3t^2(1-t) P_2 + t^3 P_3$$

With `P1 = P0 + v0·T`, `P2 = P3 − v1·T` this is a cubic Hermite curve with end tangents `3·v·T`. Matching the true velocity would need `T = Δt/3` where `Δt = N/30` s; indeed

$$T = \frac{N}{90} = \frac{1}{3}\cdot\frac{N}{30}$$

so the curve's end velocities equal the sender's velocities: the `/90` constant is exactly `3 × 30 Hz`. This is the key design insight of the smoothing.

Playback speed: the curve is traversed one step per *local* frame over `N` steps, i.e. in the same wall-clock time the sender took, so the remote babo is displayed ≈ one send-interval late (plus network latency).

### 4.3 Where it is used

- Remote players: [VERIFY: BaboViolent2/Code/PlayerUpdate.cpp:240-247]
- Minibots (`_PRO_`): [VERIFY: BaboViolent2/Code/PlayerUpdate.cpp:219-226]

Aim point (`mousePosOnMap`) is interpolated with a similar Bézier whose inner control points are built from the aim *delta* (not a velocity): [VERIFY: BaboViolent2/Code/Player.h:123-128]

---

## 5. Server-side movement validation (anti speed-hack)

Because movement is client-authoritative, the server only sanity-checks `COORD_FRAME`s:
[VERIFY: BaboViolent2/Code/ServerRecv.cpp:796-854]

```
ignore unless player alive AND packet.babonetID == player.babonetID
every ≥ 90 server frames (3 s):
    suspicious if (clientFrameDelta > serverFrameDelta + 5)   # clock running fast
               or |vel| > 3.3                                  # faster than the 3.25 clamp
    3 consecutive suspicious windows → disconnect ("speed hack")
    otherwise reset the counter
accept: player.setCoordFrame(packet)
```

Since `47e2974` (fixes false kicks reported by the Jmainguy fork):

- the frame rule allows `serverFrameDelta + 5 + serverFrameDelta / 10`;
- the speed rule uses horizontal speed > 3.25 + 0.15 (quantisation margin);
- the speed rule is skipped for 60 server ticks after a shot hits the player (`Player::framesSinceKnockback`, reset in `Game::shootSV` / `shootMinibotSV`), because the client adds knockback on top of the clamp.

- Frame-delta rule: [VERIFY: BaboViolent2/Code/ServerRecv.cpp:817-818]
- Kick after 3: [VERIFY: BaboViolent2/Code/ServerRecv.cpp:826-842]
- `frameSinceLast` is advanced once per server tick in `Player::update`: [VERIFY: BaboViolent2/Code/PlayerUpdate.cpp:89]

Assessment: only the velocity *reported in one sampled packet* per 3 s window is checked, not the actual displacement between positions. A client can move faster than 3.25 u/s by teleporting positions while reporting small velocities. The displacement-based teleport check exists but is commented out ("too many problems"): [VERIFY: BaboViolent2/Code/ServerRecv.cpp:857-904]

---

## 6. Verification checklist

- [x] Constants (12.5, 4, 1, 3.25, 3.3, 0.45, 0.05, 0.25) each traced to a line
- [x] Bézier / Hermite equivalence and the `/90` constant derived from `Player.h:97` and `Helper.cpp:437`
- [x] Collision order (performCollision then collisionClip) from `Game.cpp:630-633`
- [ ] `dkoSphereIntersection` / `dkoRayIntersection` internals (Engine/dko) not analysed
