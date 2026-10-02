# Architecture decision records

One file per non-obvious decision: `NNNN-kebab-title.md`, numbered in order, never reused.

## Rules

- An accepted ADR is never edited except for its **Status** line. To change a decision, write a new ADR and mark the old one `Superseded by NNNN`.
- Keep it under one screen. Cite code as `path:line`; link the step file it affects.
- Add every new ADR to the index below and to the `ARCHITECTURE.md` inventory.

## Format

```markdown
# NNNN: <decision as an imperative phrase>

- Status: Accepted | Superseded by NNNN | Deprecated
- Date: YYYY-MM-DD
- Affects: <step file §section>

## Context
Facts that force the decision, with `path:line` citations. 2–5 bullets.

## Decision
What we do, in one or two sentences.

## Consequences
- What gets easier or harder; follow-up work it creates.
- Rejected options, one line each, with the reason.
```

## Index

| # | Decision | Status |
|---|---|---|
| [0001](0001-ninja-generator-on-all-presets.md) | Use the Ninja generator on all presets, MSVC included | Accepted |
| [0002](0002-compile-out-libcurl.md) | Compile out libcurl in Phase A | Accepted |
| [0003](0003-keep-opengl-2.1-then-sdl-gpu.md) | Keep OpenGL 2.1 for Phase A; isolate the renderer, then move it to SDL_GPU | Accepted |
| [0004](0004-project-name-roboviolence2.md) | Name the project RoboViolence 2, an unofficial fork of BaboViolent 2 | Accepted |
