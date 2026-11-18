# 0068: New models are authored as glTF and converted to .DKO offline

- Status: Accepted
- Date: 2026-11-18
- Affects: `tools/gltf-to-dko.py`, `tests/test_gltf_to_dko.py`, `tests/dko_probe.cpp`; ROBO-239 (refactor Phase 1 A3; [ADR 0062](0062-asset-packs-mode-modules-client-independent-server.md) asset seam, owner decision D2: glTF)

## Context
- The owner makes the new art, and D2 chose glTF 2.0 as its format. The game draws models through `engine/dko`, which reads the BV2 `.DKO` chunk format (static or vertex-animated triangle lists, materials, named dummies such as `flash1` and `eject1`) and loads only `.DKO` and `.3ds`.
- ROBO-239 asked for a prototype that settles one of two ways: a runtime glTF loader beside the DKO one, or a converter to `.DKO`.

## Decision
New models are authored as glTF and converted offline by `tools/gltf-to-dko.py` (Python 3, standard library only) into a `.DKO` that the unchanged `engine/dko` loads:
- one DKO material per glTF material (name, base colour, alpha as transparency, double-sided, base colour texture as an image file beside the `.DKO`), one mesh per mesh node with one material group per triangle primitive, de-indexed and in world space; one dummy per node without a mesh;
- glTF's Y-up axes become the game's Z-up ones, `(x, y, z) -> (x, -z, y)`, which undoes Blender's glTF export: a model built in Blender keeps its Blender axes, as the BV2 3ds Max models did (Z up, a weapon's muzzle along +Y, held at about x 0.165 m, z 0.25 m);
- 1 m is 200 DKO units by default (`--scale`), because the game draws models at 0.005, so 1 m is one map tile;
- texture v is flipped (glTF rows run top-down, the game's textures bottom-up).

Static models only: animation, skins, morph targets, sparse accessors and non-triangle primitives fail with exit 1 and no output.

## Consequences
- No new code or dependency in the client or the server, and one draw path: a converted model goes through the same loader, culling and ray tests as the BV2 models, and the BV2 data loads as before.
- The converted `.DKO` ships in the pack; the `.glb` stays the source. The pack manifest (ADR 0065) lists the `.DKO`.
- `test_gltf_to_dko` builds a small glTF (as `.gltf` and `.glb`), converts it and checks meshes, materials, groups, vertices, uv, normals and the dummy with its own reader, then loads the result through `engine/dko` (`dko_probe`) and compares the bounding box, radius and dummy position.
- Prototype check (2026-11-18, not automated): a procedural `.glb` weapon converted to `models/SMG.DKO` in a copy of the BV2 data drew in the player's hand where the BV2 SMG draws.
- Animated models (the BV2 vertex-animated DKOs) need a later extension: glTF morph targets or sampled frames written as DKO frames.
- Rejected: a runtime glTF loader. It adds a parser (cgltf or tinygltf) to client and server, a second draw path for the same scene code, and a second set of ray and sphere tests for collisions, for no gain at runtime; the game would still need every DKO feature (dummies, transparency, two-sided) mapped from glTF either way.
