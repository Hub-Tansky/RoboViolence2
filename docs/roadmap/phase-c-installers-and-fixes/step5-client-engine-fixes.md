# Step 5: Client and engine fixes

**Status:** DONE (2026-10-17)

**Depends on:** Phase B DONE (manual tests: macOS and Linux pass; Windows waived by the owner). **Next:** step 6 (server and master). **Index:** [README.md](README.md)

## Scope

| Field | Value |
|---|---|
| Goal | A `char*` can no longer become a `CString` format by accident, DKT textures are freed and a malformed one can't crash the loader, menus default to the plain style, and old configs follow the OS language |
| In scope | Tasks 5.1–5.4 below (PNS-34, PNS-38 R29, PNS-30, PNS-47) |
| Out of scope | The "High detail menus" drawing (PNS-48, [GUI]); R28 (step 7); other `CString` API changes (`operator=`, `set`) |
| Allowed paths | `engine/zeven/include/CString.h`, `engine/zeven/src/CString.cpp`, `engine/zeven/src/dkf.cpp`, `engine/zeven/README.md`, `engine/dko/src/ePTexture.{h,cpp}`, `game/src/**` (`GameVar.cpp`, `Lang.{h,cpp}`, `Server.cpp`), `config/bv2.example.cfg` (`r_highDetailMenu`), `content/languages/README.md`, `tests/**`, `docs/analysis/**`, `docs/decisions/**` (owner 2026-10-17: ADR 0021), `AGENTS.md`, `ARCHITECTURE.md`, `docs/roadmap/**` |
| Inputs | `AGENTS.md`, `ARCHITECTURE.md`, PNS-30, PNS-34, PNS-38, PNS-47, `docs/analysis/KEY_QUESTIONS.md` R29 |
| Deliverables | One-argument text constructor and a formatting template (ADR 0021); R29 fix; new defaults; tests |
| Definition of done | See "Acceptance checks". All must pass. `ARCHITECTURE.md` is updated |

## Context

- `CString(char* fmt, ...)` is implicit (`engine/zeven/include/CString.h:62`): any `char*` passed where a `CString` is expected runs as a printf format (PNS-34). Making it `explicit` alone breaks ~800 call sites, nearly all string literals.
- R29: `~_typLayer` frees the texture only when `textureID == 0` (`engine/dko/src/ePTexture.h:61`); `loadMap` passes a null layer when a texture chunk precedes a known map name.
- `r_highDetailMenu` defaults to true in code and example config (PNS-30, owner-approved [GUI] default change).
- Builds before Phase B step 13 saved `languageFile "main/languages/en.lang"` (PNS-47).

## Tasks

### 5.1 `CString` conversions (PNS-34)

- One argument is text, copied as is; two or more arguments format (ADR 0021). Call sites stay as they are.

### 5.2 R29

- Free a layer's texture only when it has one; read a texture before a known map name into a scratch layer.

### 5.3 Menu default (PNS-30)

- `r_highDetailMenu` defaults to `false` in code and `config/bv2.example.cfg`.

### 5.4 Old `languageFile` (PNS-47)

- A saved `main/languages/en.lang` counts as empty (OS language) and is saved back empty.

## Critical files

- `engine/zeven/include/CString.h`, `engine/dko/src/ePTexture.{h,cpp}`, `game/src/Lang.cpp`, `game/src/GameVar.cpp`

## Acceptance checks

- `ctest --test-dir build/<preset> --output-on-failure` passes, including `cstring`, `crash_fixes` (R29) and `lang` (PNS-47).
- The CI builds (Windows MSVC, macOS, Linux; Debug and Release) compile with the new constructors.
- `tools/review.sh --full <preset>` passes, and the fresh-context review record `reviews/step5.md` and the raw reviewer report `reviews/step5-report.md` (with the skill's load line) exist (`REVIEW.md` section 5).
- `ARCHITECTURE.md` lists every file added, moved or removed in this step.
- The secret scan (`gitleaks git --staged` / CI) is clean.
