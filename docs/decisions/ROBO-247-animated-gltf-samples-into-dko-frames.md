# ROBO-247: An animated glTF is sampled into DKO frames at 10 per second

- Status: Accepted
- Date: 2026-11-25
- Affects: `tools/gltf-to-dko.py`, `tests/test_gltf_to_dko.py`, `tests/dko_probe.cpp`, `tests/flag_animation.py`, `tests/e2e/e2e.py` (`ctf_flag_animation`); ROBO-247; amends [0070](0070-gltf-converts-to-dko.md)

## Context
- BV2's flags (10 frames), flag pole (25), knives (10), shield and shield magnet (20) are vertex-animated `.DKO`s: TIME_INFO's third short is the frame count, each material group stores one vertex and one normal array per frame, each dummy one position and one matrix per frame (`engine/dko/src/CdkoMesh.cpp`, `CdkoModel.cpp`). The engine interpolates between frames and plays the last frame back into frame 0.
- The game sets the frame rate, not the model: `game/src/Map.cpp` plays the flags at 10 frames a second over 10 frames.
- ADR 0070's converter refused animations, skins and morph targets.

## Decision
`tools/gltf-to-dko.py` samples a model's one animation every 1/10 s from its first key into DKO frames, baking node transforms, morph target weights and skins into each frame's vertices, normals and dummies.

## Consequences
- The last sample is one step before the end (a loop), since the engine wraps to frame 0: a flag is authored as a 1 s loop for its 10 frames. Other lengths play cut or stretched, because the game sets the rate.
- TIME_INFO is (first frame, last frame, count), the first frame being the first key's time at 10 per second; `engine/dko` reads only the count.
- Keys are read as glTF defines them (STEP, LINEAR with slerp for rotations, CUBICSPLINE); skins blend up to 4 joints (`JOINTS_0`, `WEIGHTS_0`); every node without a mesh stays a dummy, so joints and markers like the flag pole's `Dummy01` move per frame.
- More than one animation, and an animated node with a `matrix` (glTF forbids it), fail with exit 1.
- `test_gltf_to_dko` checks TIME_INFO, frame counts and interpolated frames; `dko_probe` reads the frame count and a dummy at a frame through `engine/dko`. e2e `ctf_flag_animation` plays a converted flag in a CTF game on placeholder data and checks it moves between two frames, where the same flag without its animation does not.
- No engine or game change: the converter writes the layout BV2's animated models already use.
- Rejected: refusing skins (the item left it open): the same sampling bakes them, and Blender rigs export as skins.
- Rejected: options for the rate, the frame count, a non-looping end or picking one of several animations: no model needs them yet, and the game fixes the rate.
- Rejected: storing a frame rate in the model for the game to read: a game change, and BV2's models have none.
