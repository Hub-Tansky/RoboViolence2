# 0039: Widescreen (Stretch) shows more map sideways instead of stretching a 4:3 view

- Status: Accepted
- Date: 2026-10-27
- Affects: `game/src/ViewGeometry.h`, `IntroScreen::beginFrame`, `Game::render`, the camera clamp in `Map::update` (PR #113)

## Context
- The camera sits a fixed height above the player with a fixed 60° vertical field of view, and the HUD is laid out on a virtual 800x600 canvas, so every resolution shows the same part of the map. A higher resolution is only sharper (BV2 design).
- "Widescreen (Stretch)" (`r_widescreen 1`) drew that 4:3 view through a 4:3 projection into the whole 16:10 or 16:9 window, so every model was 20-33% wider than tall. The owner reported it at 1920x1200 on 2026-10-27 ("big balls, big walls").

## Decision
- Every 3D projection takes its aspect from the viewport it draws into (`view::aspect(view::current())`). Stretch keeps the vertical view and shows more map to the left and right (Hor+). "Widescreen (Bars)" keeps a 4:3 viewport between black bars; Standard at 4:3 resolutions is unchanged.
- The camera's side margin at the map edges scales with the aspect (5 units at 4:3), so the void past the edge stays off screen.
- Owner approved the visibility difference on 2026-10-27 (PR #113 thread).

## Consequences
- A Stretch player sees about 20% (16:10) to 33% (16:9) more map horizontally than a Standard or Bars player in the same match. Bars stays the equal-vision option.
- The HUD is still the 800x600 canvas stretched to the window in Stretch mode; other `r_widescreen` sites still carry their own Bars arithmetic until they move to `view::current()`.
- Rejected: keep the 4:3 horizontal extent and crop top and bottom (Vert-). It removes the distortion but takes vertical vision away from widescreen players.
- Rejected: keep the stretched image. It distorts every model.
- Rejected: a player-chosen camera zoom. It would hand vision to whoever zooms out; if ever added, the server sets the limit.
