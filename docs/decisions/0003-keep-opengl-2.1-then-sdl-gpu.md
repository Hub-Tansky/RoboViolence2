# 0003: Keep OpenGL 2.1 for Phase A; isolate the renderer, then move it to SDL_GPU

- Status: Accepted
- Date: 2026-10-02
- Affects: [step 3 §3.5, §3.7](../refactoring/phase-a-modern-portable-build/step3-dependency-upgrades.md), [future-phases §F](../refactoring/future-phases.md#f-rendering-and-ui)

## Context

- Rendering is fixed-function OpenGL 1.x: 3,235 `gl*` calls in 40 `.cpp` files, 144 `glBegin` blocks in 28 files, 28 display-list calls, 40 GLU calls, no shaders or buffer objects (counted 2026-10-02 over `BaboViolent2/Code` and `Engine`).
- macOS runs this only in Apple's deprecated legacy 2.1 context. If Apple removes OpenGL, the Mac client stops working, and macOS is a release-blocking target.
- A GL 3.3 core renderer would still run on Apple's deprecated OpenGL (frozen at 4.1), so it doesn't remove that risk. Only a Metal-backed API does.
- The dedicated server (`CONSOLE`) already builds without rendering, so rendering is the most separable part of the game.

## Decision

1. **Phase A:** keep OpenGL 2.1 compatibility + GLU, loaded through glad on an SDL3 context.
2. **Isolate (refactor, no new dependency):** gather the `glBegin` blocks, display lists and GLU calls into one batched sprite/mesh renderer that still uses GL 2.1.
3. **Replace:** move that renderer's backend to **SDL_GPU** (Metal, Vulkan, D3D12).

Stage 1 starts after the first playable match. Stage 2 starts when Apple announces OpenGL removal or once regular players exist, whichever comes first.

## Consequences

- Phase A ships with no rendering rewrite. The macOS risk stays until stage 2; check Apple's release notes every year.
- After stage 1, a backend swap touches one module instead of 40 files.
- SDL_GPU needs shaders per backend (SPIR-V, MSL, DXIL), built with SDL_shadercross.
- Rejected: GL 3.3 core (same cost as stage 2, macOS risk remains); bgfx / sokol_gfx (new dependency; fallback only if SDL_GPU lacks something stage 1 needs); rewriting the game (throws away the networking and gameplay, which are the value).
