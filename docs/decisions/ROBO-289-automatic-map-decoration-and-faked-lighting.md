# ROBO-289: Map decoration is automatic with optional overrides; map lighting is faked without shaders

- Status: Accepted
- Date: 2026-11-28
- Affects: [game/CONTEXT.md](../../game/CONTEXT.md) section Maps; `game/src/MapRender.cpp`, `game/src/Map.h` (`MAP_VERSION`), the editor; keeps [0003](0003-keep-opengl-2.1-then-sdl-gpu.md); rv2-docs ROBO-289 and its children ROBO-291 to ROBO-298 (owner grill 2026-11-28)

## Context
- Maps look much worse than the concept art. Today the floor is one texture plus a dirt splat, and every wall cell is a 1x1 box with one texture on all faces and vertex-colour shading. A `.bvm` cell stores only passable, height and dirt.
- The concepts get their look from wall runs with corners, pillars and panels, floor plates with cracks and rubble, contact shadows, and glows (lava, vents, light shafts). The desert-arena reference adds mixed wall heights and props on wall tops.
- ADR 0003 keeps the renderer fixed-function OpenGL 2.1 until the SDL_GPU move, so there are no shaders.
- The 10 maps in `content/maps/` should all improve without being edited again.

## Decision
1. **Automatic first.** The game picks each wall piece, floor variant, decal, spray and wall-top prop from the cell's neighbours and a hash of the map content plus the cell position. Every client and every screenshot gets the same result, and editing one cell only changes the pieces next to it.
2. **Overrides later and optional.** A new `MAP_VERSION` adds an optional decoration section to `.bvm` that holds decoration overrides per cell (ROBO-298). A map without the section looks as the automatic pick makes it. No old-reader support (ADR ROBO-278).
3. **Box walls with autotiled art, not a 3D kit.** Wall cells stay boxes; the wall piece picks their art. The camera is near top-down, so wall tops and lips fill the screen, and the image-model workflow makes 2D textures well and meshes poorly.
4. **Lighting is faked, no shaders (ADR 0003 stands).** A baked light/AO pass as a second texture, additive glow sprites and the existing particles for embers and smoke. Lighting and wall-top props each get an off switch.
5. **Decoration is visual only.** Heights 1 to 5 keep their bullet and movement behaviour; props and sprays never block anything and never hide a player or a flag. Map art never uses team blue or red and stays below player, bullet and pickup contrast.

## Consequences
- All existing maps change look on the first release that ships ROBO-292; there is no per-map opt-out before ROBO-298.
- The decoration must be deterministic; any random choice in the renderer that is not seeded from the map breaks screenshots across clients.
- Baked light is static: no moving or flickering lights beyond what sprites and particles can fake. Real lights wait for SDL_GPU and its shaders.
- The SDL_GPU port gets textures and sprites only, no GLSL to port.
- Rejected: an editor-painted-only format (the 10 maps would need repainting before anything improves); a 3D modular wall kit (mesh art is the image model's weak point and the view hides most of it); GLSL 1.20 shaders now (amends ADR 0003 and is ported twice).
