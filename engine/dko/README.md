# dko

Loader and renderer for the `.DKO` model format (chunked binary, exported from 3ds Max) with ray and sphere intersection against an octree.

- **API:** `include/dko.h`, prefix `dko*` (`dkoLoadFile`, `dkoRender`, `dkoGetDummyPosition`, ray/sphere tests).
- **Format:** `src/dko-chunk-info.txt`; `src/CHUNKINF.H` is Autodesk's 3DS chunk list (3DS import).
- **Depends on:** `zeven_core` (types, `dkt` textures), OpenGL.
- **Used by:** `bv2`, `bv2dedicated` (the server loads models for collision and dummies).

## Gotchas

- Models are indexed in a fixed array of 1024; `dkoLoadFile` returns an ID, 0 on failure (`dkoGetLastError`).
- An empty-but-valid model (version, time info, end chunk) loads, so placeholder assets work (`tools/gen-placeholder-content.py`).
- The original `.DKO` files are removed; see [../../docs/assets/ASSET-INVENTORY.md](../../docs/assets/ASSET-INVENTORY.md).
