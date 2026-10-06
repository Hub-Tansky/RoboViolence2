# Possible new scope

Knowledge gained while working on a step that affects later phases or work outside any planned step. Each entry starts as **Proposed**. The owner moves it into a phase plan (**Accepted**, with the target) or drops it (**Rejected**, with the reason).

Entry format: ID, where it was found, an extract of the source, why it matters, suggested home, status.

## From Phase A

### PNS-1: Phase A manual test never run; several runtime paths unverified

- **Found in:** PR #13, PR #7, `ARCHITECTURE.md` Open items.
- **Extract:** "The manual test has not been run yet on any OS." (PR #13); "Not verified: Windows `/WX` on game targets (first CI run), OGG playback, HiDPI, audio, a real two-machine game." (PR #7)
- **Why:** The README's "Phase A done when" requires DM, TDM and CTF rounds with sound, input and map download on all three OSes. The Results table in [phase-a-manual-test.md](phase-a-modern-portable-build/phase-a-manual-test.md) is empty. Phase B changes (§A packet hygiene, §C.5 validator) need this baseline to detect regressions.
- **Suggested home:** run before Phase B §A starts; record results in the table.
- **Status:** Proposed

### PNS-2: Font loader is single-byte, codes 33–159 only

- **Found in:** PR #12 (placeholder font atlas).
- **Extract:** "`CFont::loadTGAFile` (`engine/zeven/src/CFont.cpp`) finds characters 33–159 by scanning `main/fonts/babo.tga` in 64-pixel rows" (`engine/zeven/src/CFont.cpp:156`). "Rows are written bottom-up because `dkt.cpp` ignores the TGA origin bit."
- **Why:** UTF-8 `.po` strings (§I.2) and Latin Extended glyphs (§H.6) need more than new font art: the loader and text renderer must decode UTF-8 and index more than 127 glyphs, without breaking `babo.tga` (ADR 0010). Replacement TGAs (§H) must be bottom-up.
- **Suggested home:** §I (UTF-8 decoding, glyph lookup) and §H.6 (font coverage and atlas layout).
- **Status:** Proposed

### PNS-3: Headless server still links GLU

- **Found in:** `ARCHITECTURE.md` Open items, PR #3.
- **Extract:** "GLU: still the system library (headers and link) for `dko`, `zeven_client` and the game, so the headless server needs GLU dev packages; `dkt` is stubbed there."
- **Why:** The §C.1 sim core must have no rendering dependency, and §D container images should not need GL packages.
- **Suggested home:** §C.1 (split `dko` geometry loading from GL upload) or §F.1 (replace GLU).
- **Status:** Proposed

### PNS-4: Master server client class is reconstructed

- **Found in:** PR #2, `masterserver/README.md`.
- **Extract:** "The master's client class is missing from the source release; `MasterClient` is reconstructed from its usage and its 60 s timeout is an assumption."
- **Why:** Unverified protocol behaviour in a network-facing service; it should be covered before §A.3 (SQL) and §D (hosting).
- **Suggested home:** §A (tests for master register and list), §D.
- **Status:** Proposed

### PNS-5: Pre-existing UBSan defects

- **Found in:** `ARCHITECTURE.md` Open items, PR #2, PR #5.
- **Extract:** "UBSan reports pre-existing defects (out-of-range `bool` loads, NaN casts in `CUserLogin.cpp`)."
- **Why:** Undefined behaviour in shipped code, not yet in the R1–R15 register.
- **Suggested home:** §A.5; add them to `docs/analysis/KEY_QUESTIONS.md`.
- **Status:** Proposed

### PNS-6: Lifecycle null-pointer crashes are a pattern

- **Found in:** PR #10, PR #5, PR #2.
- **Extract:** "`~Scene()` calls `Scene::disconnect()`, which sets `menuManager.root->visible` without a null check" (PR #10); "Launch-script `set sv_*` before `dedicate` called into a null `Server`" (PR #5).
- **Why:** Three separate null dereferences in start-up and teardown. More are likely, and §A fuzzing will hit them.
- **Suggested home:** §A.2 (audit start-up and teardown paths under ASan).
- **Status:** Proposed

### PNS-7: Static initialisation order is fragile

- **Found in:** PR #2, PR #9.
- **Extract:** "with static linking the `dksvar` registry was constructed after `GameVar`, so every `set` said 'Unknown variable'" (PR #2); "`gameVar` is a static … Its constructor loaded `main/languages/en.lang` relative to the working directory before `main()` ran `pathsInit()`" (PR #9).
- **Why:** The §C.1 sim core and its tests need an explicit init order, not global constructors.
- **Suggested home:** §C.1 or §G.
- **Status:** Proposed

### PNS-8: Dead code and warning debt

- **Found in:** `ARCHITECTURE.md` Open items, PR #5, PR #7.
- **Extract:** "16 old-menu `.cpp` files in `game/src` are not built"; "about 4800 string-literal-to-`char*` warnings are suppressed"; "Cosmetic classes … are off for `game` and `masterserver` (`bv2_target_legacy_strict`)"; "clang-tidy runs in CI as a non-required job".
- **Why:** Leftover headers mislead plans (e.g. `game/src/OptionMenu.h` declares a language picker that doesn't exist). Warning suppressions hide new defects.
- **Suggested home:** §G: delete the unbuilt files, burn down the suppressed warning classes, then make clang-tidy required.
- **Status:** Proposed

### PNS-9: `std::byte` is disabled on Windows

- **Found in:** PR #5, `CMakeLists.txt:52`.
- **Extract:** "`_HAS_STD_BYTE=0`) # `using namespace std;` and windows.h both define `byte` otherwise"
- **Why:** Blocks `std::byte` in new code (e.g. §E serialisation buffers) until `using namespace std;` is removed from headers.
- **Suggested home:** §G, before §E.
- **Status:** Proposed

### PNS-10: File-format tests are a spec for exporters

- **Found in:** PR #7.
- **Extract:** "`tests/test_fileio.cpp`: pins `.bvm` field widths; `.DKO` loader audited (explicit `short`/`float`/32-bit fields)."
- **Why:** The §H.4 Blender/glTF exporter and new maps must write exactly these layouts (ADR 0010). The tests can validate exporter output.
- **Suggested home:** §H.4 (run exporter output through these tests).
- **Status:** Proposed

### PNS-11: Stale facts in `ARCHITECTURE.md`

- **Found in:** closing Phase A (2026-10-06).
- **Extract:** "State after Step 3"; "Protocol `GAME_VERSION_SV/CL` = 21100"; "Linux and Windows are untested". The code has `GAME_VERSION_SV 22000` (`game/src/Server.h:32`), and CI builds all three OSes.
- **Why:** Agents treat `ARCHITECTURE.md` as the source of truth.
- **Suggested home:** next PR that touches `ARCHITECTURE.md`.
- **Status:** Proposed
