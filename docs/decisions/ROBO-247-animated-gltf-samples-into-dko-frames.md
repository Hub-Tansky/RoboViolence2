# ROBO-247: An animated glTF is sampled into DKO frames at 10 per second

- Status: Accepted
- Date: 2026-11-25
- Affects: `tools/gltf-to-dko.py`, `tests/test_gltf_to_dko.py`, `tests/dko_probe.cpp`, `tests/flag_animation.py`, `tests/e2e/e2e.py` (`ctf_flag_animation`); ROBO-247; amends [0070](0070-gltf-converts-to-dko.md)

## Context
- BV2's flags (10 frames), flag pole (25), knives (10), shield and shield magnet (20) are vertex-animated `.DKO`s: TIME_INFO's third short is the frame count, each material group stores one vertex and one normal array per frame, each dummy one position and one matrix per frame (`engine/dko/src/CdkoMesh.cpp`, `CdkoModel.cpp`). The engine interpolates between frames and plays the last frame back into frame 0.
- The game drives frames by time, not by the model: `game/src/Map.cpp` plays the flags at 10 frames a second over 10 frames.
- ADR 0070's converter refused animations, skins and morph targets.

## Decision
- One animation becomes the DKO's frames. A model with several needs `--animation <name|index>`.
- The animation is sampled every 1/10 s by default (`--fps`), or into exactly `--frames N` frames over its length. A loop by default: the last sample is one step before the end, since the engine wraps to frame 0; `--no-loop` adds the end pose.
- TIME_INFO is (first frame, last frame, count), the first frame being the animation's start time in frames.
- Each frame bakes everything into world-space vertices, as static models already are: node translation, rotation (slerp) and scale, morph target weights (positions and normals) and skins (linear blend of up to 4 joints, `JOINTS_0`/`WEIGHTS_0`). Every node without a mesh stays a dummy, with a position and matrix per frame, so joints and moving markers like the flag pole's `Dummy01` come through.
- STEP, LINEAR and CUBICSPLINE keys are read as glTF defines them. An animated node with a `matrix` fails, as glTF forbids it.
- Skins are supported, not refused (the item left this open): the same per-frame sampling bakes them, and Blender rigs export as skins.

## Consequences
- A flag must be authored as a 1 s loop (10 frames at the default rate) to play like BV2's; other lengths play cut or stretched, because the game, not the model, sets the frame rate.
- `test_gltf_to_dko` checks TIME_INFO, frame counts and interpolated frames (morph LINEAR, STEP, CUBICSPLINE, `--frames`, `--fps`, `--no-loop`, `--animation`, a skinned strip), and `dko_probe` reads the frame count and a dummy at a frame through `engine/dko`.
- e2e `ctf_flag_animation` runs a converted flag in game on placeholder data: a client spawned over it takes two frames 0.3 s apart, and the animated flag changes far more pixels than the same flag converted as one frame.
- No engine or game change: the converter writes the layout BV2's animated models already use.
