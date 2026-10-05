# 0005: Keep the `main/` data root; the runtime dir assembles it

- Status: Accepted
- Date: 2026-10-03
- Affects: [step 2 §2.0, §2.6](../refactoring/phase-a-modern-portable-build/step2-cmake-build.md)

## Context
- Game code opens files as `main/...` (`game/src/GameVar.cpp`, `Map.cpp`, `Console.cpp:655`). Hundreds of literals, plus `main/bv2.cfg` and `main/LaunchScript/`.
- `content/` holds only `languages/` and `LaunchScript/`; the original assets are gone and nothing may be generated into `content/` (step 2 §2.6).
- Placeholder assets and the example config must sit next to the tracked content at runtime.

## Decision
Don't rewrite the `main/` prefix. The runtime directory (`<build>/runtime/`) contains a real `main/` directory: `content/` entries are linked in (symlink, junction on Windows without symlink rights), `bv2.cfg` is copied from `config/bv2.example.cfg`, and `tools/gen-placeholder-content.py` writes placeholder assets beside them.

## Consequences
- No source edits for paths; the working directory must be the runtime dir. Step 4 §4.4 makes the data root configurable (`BV2_DATA_DIR`).
- A developer's own original data replaces the generated parts by pointing `main/` elsewhere, without touching the repo.
- Installed layouts (`share/bv2/main/`, `.app/Contents/Resources/main/`) assemble the same directory with `install()`.
- Rejected: mounting `content/` itself as `main/` (generated files would land in the repo tree); search-and-replace of the prefix (large, risky diff before Step 4 touches the same lines).
