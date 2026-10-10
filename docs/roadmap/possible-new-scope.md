# Possible new scope

Knowledge gained while working on a step that affects later phases or work outside any planned step. Each entry starts as **Proposed**. The owner moves it into a phase plan (**Accepted**, with the target), drops it (**Rejected**, with the reason), or folds it into another entry (**Merged into PNS-n**).

Entry format: ID, where it was found, an extract of the source, why it matters, suggested home, status.

## From Phase A

### PNS-1: Phase A manual test never run; several runtime paths unverified

- **Found in:** PR #13, PR #7, `ARCHITECTURE.md` Open items.
- **Extract:** "The manual test has not been run yet on any OS." (PR #13); "Not verified: Windows `/WX` on game targets (first CI run), OGG playback, HiDPI, audio, a real two-machine game." (PR #7)
- **Why:** The README's "Phase A done when" requires DM, TDM and CTF rounds with sound, input and map download on all three OSes. The Results table in [phase-a-manual-test.md](phase-a-modern-portable-build/phase-a-manual-test.md) is empty. Phase B changes (§A packet hygiene, §C.5 validator) need this baseline to detect regressions.
- **Suggested home:** run before Phase B §A starts; record results in the table.
- **Status:** Accepted → Phase B [step 1](phase-b-security-infrastructure-anti-cheat/step1-playtest-builds-and-build-guides.md) and [step 2](phase-b-security-infrastructure-anti-cheat/step2-cross-os-playtest.md); the rest → PNS-31

### PNS-2: Font loader is single-byte, codes 33–159 only

- **Found in:** PR #12 (placeholder font atlas).
- **Extract:** "`CFont::loadTGAFile` (`engine/zeven/src/CFont.cpp`) finds characters 33–159 by scanning `main/fonts/babo.tga` in 64-pixel rows" (`engine/zeven/src/CFont.cpp:156`). "Rows are written bottom-up because `dkt.cpp` ignores the TGA origin bit."
- **Why:** UTF-8 `.po` strings (§I.2) and Latin Extended glyphs (§H.6) need more than new font art: the loader and text renderer must decode UTF-8 and index more than 127 glyphs, without breaking `babo.tga` (ADR 0010). Replacement TGAs (§H) must be bottom-up.
- **Suggested home:** §I (UTF-8 decoding, glyph lookup) and §H.6 (font coverage and atlas layout).
- **Status:** Merged into PNS-20 (font art) and PNS-21 (UTF-8 rendering); Phase B step 13 skips languages the font can't draw

### PNS-3: Headless server still links GLU

- **Found in:** `ARCHITECTURE.md` Open items, PR #3.
- **Extract:** "GLU: still the system library (headers and link) for `dko`, `zeven_client` and the game, so the headless server needs GLU dev packages; `dkt` is stubbed there."
- **Why:** The §C.1 sim core must have no rendering dependency, and §D container images should not need GL packages.
- **Suggested home:** §C.1 (split `dko` geometry loading from GL upload) or §F.1 (replace GLU).
- **Status:** Merged into PNS-12; Phase B step 11 ships GLU in the server image meanwhile

### PNS-4: Master server client class is reconstructed

- **Found in:** PR #2, `masterserver/README.md`.
- **Extract:** "The master's client class is missing from the source release; `MasterClient` is reconstructed from its usage and its 60 s timeout is an assumption."
- **Why:** Unverified protocol behaviour in a network-facing service; it should be covered before §A.3 (SQL) and §D (hosting).
- **Suggested home:** §A (tests for master register and list), §D.
- **Status:** Accepted → Phase B [step 3](phase-b-security-infrastructure-anti-cheat/step3-test-harness.md)

### PNS-5: Pre-existing UBSan defects

- **Found in:** `ARCHITECTURE.md` Open items, PR #2, PR #5.
- **Extract:** "UBSan reports pre-existing defects (out-of-range `bool` loads, NaN casts in `CUserLogin.cpp`)."
- **Why:** Undefined behaviour in shipped code, not yet in the R1–R15 register.
- **Suggested home:** §A.5; add them to `docs/analysis/KEY_QUESTIONS.md`.
- **Status:** Accepted → Phase B [step 5](phase-b-security-infrastructure-anti-cheat/step5-crash-fixes.md)

### PNS-6: Lifecycle null-pointer crashes are a pattern

- **Found in:** PR #10, PR #5, PR #2.
- **Extract:** "`~Scene()` calls `Scene::disconnect()`, which sets `menuManager.root->visible` without a null check" (PR #10); "Launch-script `set sv_*` before `dedicate` called into a null `Server`" (PR #5).
- **Why:** Three separate null dereferences in start-up and teardown. More are likely, and §A fuzzing will hit them.
- **Suggested home:** §A.2 (audit start-up and teardown paths under ASan).
- **Status:** Accepted → Phase B [step 5](phase-b-security-infrastructure-anti-cheat/step5-crash-fixes.md)

### PNS-7: Static initialisation order is fragile

- **Found in:** PR #2, PR #9.
- **Extract:** "with static linking the `dksvar` registry was constructed after `GameVar`, so every `set` said 'Unknown variable'" (PR #2); "`gameVar` is a static … Its constructor loaded `main/languages/en.lang` relative to the working directory before `main()` ran `pathsInit()`" (PR #9).
- **Why:** The §C.1 sim core and its tests need an explicit init order, not global constructors.
- **Suggested home:** §C.1 or §G.
- **Status:** Merged into PNS-12

### PNS-8: Dead code and warning debt

- **Found in:** `ARCHITECTURE.md` Open items, PR #5, PR #7.
- **Extract:** "16 old-menu `.cpp` files in `game/src` are not built"; "about 4800 string-literal-to-`char*` warnings are suppressed"; "Cosmetic classes … are off for `game` and `masterserver` (`bv2_target_legacy_strict`)"; "clang-tidy runs in CI as a non-required job".
- **Why:** Leftover headers mislead plans (e.g. `game/src/OptionMenu.h` declares a language picker that doesn't exist). Warning suppressions hide new defects.
- **Suggested home:** §G: delete the unbuilt files, burn down the suppressed warning classes, then make clang-tidy required.
- **Status:** Merged into PNS-19

### PNS-9: `std::byte` is disabled on Windows

- **Found in:** PR #5, `CMakeLists.txt:52`.
- **Extract:** "`_HAS_STD_BYTE=0`) # `using namespace std;` and windows.h both define `byte` otherwise"
- **Why:** Blocks `std::byte` in new code (e.g. §E serialisation buffers) until `using namespace std;` is removed from headers.
- **Suggested home:** §G, before §E.
- **Status:** Merged into PNS-17

### PNS-10: File-format tests are a spec for exporters

- **Found in:** PR #7.
- **Extract:** "`tests/test_fileio.cpp`: pins `.bvm` field widths; `.DKO` loader audited (explicit `short`/`float`/32-bit fields)."
- **Why:** The §H.4 Blender/glTF exporter and new maps must write exactly these layouts (ADR 0010). The tests can validate exporter output.
- **Suggested home:** §H.4 (run exporter output through these tests).
- **Status:** Merged into PNS-20

### PNS-11: Stale facts in `ARCHITECTURE.md`

- **Found in:** closing Phase A (2026-10-06).
- **Extract:** "State after Step 3"; "Protocol `GAME_VERSION_SV/CL` = 21100"; "Linux and Windows are untested". The code has `GAME_VERSION_SV 22000` (`game/src/Server.h:32`), and CI builds all three OSes.
- **Why:** Agents treat `ARCHITECTURE.md` as the source of truth.
- **Suggested home:** next PR that touches `ARCHITECTURE.md`.
- **Status:** Accepted → Phase B [step 0](phase-b-security-infrastructure-anti-cheat/step0-supply-chain-security.md)

## Deferred from Phase B planning (2026-10-07)

Items of the former Phase B plan (`future-phases.md`, deleted) that are not in a [Phase B step](phase-b-security-infrastructure-anti-cheat/README.md). Extracts quote that plan.

### PNS-12: Headless sim core, player commands, seeded RNG

- **Source:** former §C.1, §C.2, §C.4; absorbs PNS-3 (GLU in the server) and PNS-7 (static init order).
- **Extract:** "Extract the game rules with no rendering, audio, UI or networking: `Game`, `Player` physics, `Map` collision and ray march, `Weapon`, `Projectile`, game modes, spawning." "A `PlayerCommand` struct: tick number, move vector, aim direction, fire …" "Replace global `rand()` with a per-`Game` seeded RNG."
- **Why deferred:** Owner decision (2026-10-07). Large refactor of every `#ifndef CONSOLE` block; needed only for replays, server-authoritative movement and information hiding (PNS-13). Phase B's `ServerValidator` (step 8) is the layer these plug into later.
- **Suggested home:** first steps of the phase after B; needs the Phase B manual test as a baseline.
- **Status:** Proposed

### PNS-13: Advanced anti-cheat

- **Source:** former §C.8–§C.12.
- **Extract:** "A per-match event stream (JSON lines or SQLite): shots, hits, damage, accuracy, time-to-target and aim-snap angles …" "Record inbound commands and periodic snapshots per tick" "Clients send C.2 commands, the server simulates with C.1, the client predicts and reconciles." "The server sends positions only for players the recipient could perceive" "Bans by account, with expiry … An in-game `/report` command … is **[GUI]**."
- **Why deferred:** Needs PNS-12 (sim core, commands, RNG) and PNS-17 (UDP, snapshots); C.12 also needs accounts (PNS-14).
- **Suggested home:** after PNS-12 and PNS-17. Telemetry (C.8) can come first: it only needs the step 8 validator.
- **Status:** Proposed

### PNS-14: Account tokens and modern KDF

- **Source:** former §B.
- **Extract:** "game servers must never see reusable account secrets. Use short-lived signed session tokens issued by the account service and verified by the game server." "Replace MD5 with a modern KDF (Argon2id / scrypt) on the account side."
- **Why deferred:** Blocked on the account/ladder decision (PNS-15). Phase B step 10 covers server and admin passwords and drops MAC identity.
- **Suggested home:** with PNS-15.
- **Status:** Proposed

### PNS-15: Account and ladder system, service endpoints

- **Source:** former §D (endpoints, account and ladder).
- **Extract:** "`CUserLogin`, `CRegisterClan`: keep and host it, replace it, or remove it. This is a product decision; removing its menus is **[GUI]**." "host your own account, master and report services over HTTPS. Their addresses are supplied only through local config or env, **never committed**."
- **Why deferred:** Product decision by the owner; any HTTP service turns `BV2_WITH_HTTP` back on or replaces `CCurl` (ADR 0002).
- **Suggested home:** decide before the phase that takes PNS-13 and PNS-14.
- **Status:** Proposed

### PNS-16: Signed distribution

- **Source:** former §D distribution.
- **Extract:** "Windows 11: signed installer or zip; macOS: codesign + notarisation, DMG; Linux: AppImage and/or Flatpak (+ `.deb`); optionally Steam or itch.io."
- **Why deferred:** Needs certificates, developer accounts and costs. Phase B step 1 ships unsigned test packages.
- **Suggested home:** before any public release (after PNS-20 H.1).
- **Status:** Proposed

### PNS-17: Networking rework

- **Source:** former §E; absorbs PNS-9 (`std::byte` disabled on Windows).
- **Extract:** "Move coord frames and, later, C.2 commands to UDP … only **after** auditing babonet's UDP / `cPeer2Peer` path" "Replace raw `memcpy` structs with explicit, versioned serialisation" "Tick-stamped snapshots and delta compression"
- **Why deferred:** Big protocol change; prerequisite of server-authoritative movement and information hiding (PNS-13). Phase B hardens the current TCP protocol instead (steps 4, 10).
- **Suggested home:** with PNS-12.
- **Status:** Proposed

### PNS-18: Renderer isolation and replacement

- **Source:** former §F.
- **Extract:** "gather the 144 `glBegin` blocks, display lists and GLU calls into one batched sprite/mesh renderer, still on GL 2.1" "move that renderer to SDL_GPU (Metal, Vulkan, D3D12)" "**[GUI]** HiDPI and ultrawide UI scaling, gamepad menus."
- **Why deferred:** ADR 0003 triggers (first playable match, then Apple's GL removal or regular players) are not met. HiDPI and gamepad menus are **[GUI]**. First analyse rendering, menus, the editor and `CMeshBuilder` with the `codebase-analysis` skill.
- **Suggested home:** after Phase B's cross-OS play-test (step 2) shows the GL path holding up.
- **Play-test note (2026-10-08, macOS arm64):** Apple's GL layer logs `FALLBACK (log once): Fallback to SW fragment processing (outputInfo.polygonModeMismatch)`, i.e. some draws run in software (`glPolygonMode` use: `game/src/Player.cpp:539, 553, 581, 588`, `game/src/CUserLogin.cpp:411`). The game still ran smoothly; recheck frame rate when isolating the renderer.
- **Status:** Proposed

### PNS-19: Code modernisation (continuous)

- **Source:** former §G; absorbs PNS-8 (dead code, warning debt, clang-tidy).
- **Extract:** "Replace raw `sprintf`/`strcpy`/`strcat` (90 call sites) with `snprintf` or `std::string` as files are touched." "raw `new`/`delete` of `Player`, `Projectile` and `CControl` → `std::unique_ptr` where lifetimes are clear." "Internal `bv2` → `roboviolence2` renames"
- **Why deferred:** Continuous rule, not a step: done in files a step already touches ([roadmap README](README.md) ground rules). A dedicated cleanup step (delete the 16 unbuilt menu files, burn down suppressed warnings, make clang-tidy required) can be scheduled at any time.
- **Suggested home:** a cleanup step in any phase; the rule applies throughout.
- **Owner note (2026-10-08):** remove the debug print `printf("nbVertex: …")` in `engine/dko/src/CdkoMesh.cpp:125`; it floods the client's terminal on every model load.
- **Status:** Proposed

### PNS-20: Replacement assets and rebrand

- **Source:** former §H.1–§H.8; absorbs PNS-10 (file-format tests as exporter spec) and the art side of PNS-2.
- **Extract:** "**Minimum playable set:** every `required` row. Textures, skins, fonts, models …, effects and one map per game mode. Use CC0/CC-BY or original work only." "**Toolchain:** … Write a Blender exporter, or convert from glTF in `tools/`" "**Fonts:** the replacement font covers Latin Extended at least" "**Rebrand** **[GUI]**: window title, menus, credits, binary names (`bv2*`), `BV2_*` env vars, config/pref-path names, master-server game ID. Keep a 'based on BaboViolent 2 by bitHeads / RndLabs' credit."
- **Also covers:** H.2 record every replacement (author, licence, source) and keep the hash blocklist on; H.3 recreate maps with the editor under new names; H.5 new effects and music; H.8 record any rights the original authors grant (re-adding a granted original needs an ADR).
- **Why deferred:** A separate art and asset track; ADR 0010 keeps BV2 compatibility until H.1 ships. Rebranding is **[GUI]**.
- **Suggested home:** its own phase, in parallel from now on; nothing ships publicly before H.1.
- **Status:** Proposed

### PNS-21: Localisation beyond the foundations

- **Source:** former §I.5; absorbs the code side of PNS-2.
- **Extract:** "**[GUI]** An in-game language picker, and moving hard-coded English strings (e.g. `game/src/GameShowStats.cpp:298`) into the language file where that changes the text shown."
- **Why deferred:** **[GUI]** changes need the owner's approval. Non-ASCII languages also need UTF-8 decoding in `CFont` (`engine/zeven/src/CFont.cpp:156`) and new fonts (PNS-20).
- **Suggested home:** after Phase B step 13 and PNS-20 fonts.
- **Status:** Proposed

## From Phase B

### PNS-22: No vulnerability alerts for vcpkg ports

- **Found in:** Phase B step 0 (2026-10-07), local OSV-Scanner 2.6.0 run.
- **Extract:** OSV-Scanner on `build/macos-arm64/vcpkg_installed`: "Filtered 6 local/unscannable package/s from the scan." vcpkg's SPDX gives `pkg:vcpkg/sqlite3@3.53.4?port_version=2&...`.
- **Why it matters:** Neither Dependabot (no C/C++ ecosystem in the GitHub Advisory Database) nor OSV checks sqlite3, SDL3, miniaudio or stb. The only defence is the manual monthly baseline bump.
- **Suggested home:** re-check when OSV or GitHub adds a vcpkg ecosystem, or generate an SBOM that maps ports to upstream `pkg:github/...` purls with versions OSV can match (OSS-Fuzz entries for sqlite and SDL exist). Any phase; small.
- **Status:** Proposed

### PNS-23: ADR for the play-test package format

- **Found in:** Phase B step 1 fresh-context review (2026-10-07), finding 6.
- **Extract:** "Step task 1.2 asks for `.sh` and `.ps1` run scripts; `.cmd` was delivered … The new distribution format (archive layout, unarchived artifact upload, duplicate bundle `main/`, ad-hoc re-sign) has no ADR either."
- **Why it matters:** A later agent might "fix" `.cmd` back to `.ps1`, drop the second macOS `main/`, or re-zip the artifacts without knowing why they are as they are (`packaging/make-package.py`, step 1 task 1.2).
- **Suggested home:** a short ADR in the next PR that touches packaging, or with PNS-16 (signed distribution). The owner asked to check it further before writing one.
- **Status:** Proposed

### PNS-24: Remove the unused runtime `bv2.db`

- **Found in:** Phase B step 1 fresh-context review (2026-10-07), finding 10.
- **Extract:** "the build still generates `runtime/bv2.db` (`tools/CMakeLists.txt:11-25` …). The game never reads it: `openClientDb` uses the pref dir (`game/src/Paths.cpp:275`)."
- **Why it matters:** Dead build output that the docs have to explain, and a second copy of client-database seed data.
- **Suggested home:** any step touching `tools/CMakeLists.txt`; check first that nothing seeds the pref-dir `bv2.db` from it.
- **Status:** Proposed

### PNS-25: Hide the "Color depth" option

- **Found in:** owner play-test on macOS, 2026-10-08.
- **Extract:** Options → "Color depth" (16/32) shows no visible change. It sets `r_bitdepth` (`game/src/COption.cpp:146`), passed to `dkwInit` (`game/src/main.cpp:460`), which requests minimum colour-buffer sizes (`engine/zeven/src/dkw.cpp:352–355`: 5/6/5 or 8/8/8/8 bits). The driver may exceed them, and modern systems give 32-bit either way. Like fullscreen, it only applies after a restart (tooltip, `game/src/COption.cpp:142`); not yet tested after a restart.
- **Why it matters:** a control with no effect confuses players.
- **Suggested home:** a **[GUI]** change (owner approval), e.g. with the rebrand (PNS-20) or menu work. Fullscreen and resolution apply after a restart (confirmed); a "restart required" hint would be the same kind of change.
- **Status:** Proposed

### PNS-26: vcpkg ports ignore the macOS 12 deployment target

- **Found in:** Phase B step 2 (2026-10-08), building the client on macOS.
- **Extract:** `ld: warning: object file (…/vcpkg_installed/arm64-osx/debug/lib/libSDL3.a[248](SDL_dummysensor.c.o)) was built for newer 'macOS' version (27.0) than being linked (12.0)`.
- **Why it matters:** `CMakePresets.json:58` sets `CMAKE_OSX_DEPLOYMENT_TARGET` 12.0 for our code, but the `arm64-osx` triplet builds SDL3 and the other ports for the build machine's macOS. The CI package (built on `macos-14`) therefore likely needs macOS 14+, while the docs promise 12+. Friends on older Macs may not be able to start it.
- **Suggested home:** an overlay triplet setting `VCPKG_OSX_DEPLOYMENT_TARGET 12.0` for the macOS presets (touches `CMakePresets.json` and a new `triplets/`), or change the documented minimum. Before the PNS-31 cross-OS game if a tester has macOS < 14.
- **Status:** Proposed

### PNS-27: Pin the Windows CI runner image

- **Found in:** Phase B step 2a fresh-context review (2026-10-10), finding 1.
- **Extract:** "The CI log of run 38002113508 shows `Operating System: Microsoft Windows Server 2025`, `Image: windows-2025-vs2026` … Optionally pin `windows-2025` instead of `windows-latest` so the fact can't drift silently."
- **Why it matters:** The docs and ADR 0012 name the Windows version CI tests on; `windows-latest` can change under them. `.github/workflows/` is outside step 2a's Allowed paths.
- **Suggested home:** the next step that touches `.github/workflows/build.yml` (step 3 adds a fuzz job there).
- **Status:** Proposed

### PNS-28: Start in fullscreen on every OS

- **Found in:** owner request after the Windows 10 play-test (2026-10-10).
- **Extract:** "on all systems game starts in fullscreen mode."
- **Why it matters:** the packages start windowed because they are Debug builds: `r_fullScreen` defaults to false only under `_DEBUG` (`game/src/GameVar.cpp:642–646`), true otherwise (step 2a review round 2, finding 2).
- **Suggested home:** owner-approved (2026-10-10). Release packages give fullscreen without a code change.
- **Status:** Accepted → Phase B [step 2b](phase-b-security-infrastructure-anti-cheat/step2b-release-packages.md)

### PNS-30: "High detail menus" looks worse than off

- **Found in:** owner play-test on macOS and Linux, Phase B step 2 (2026-10-10).
- **Extract:** Options → "High detail menus" (`r_highDetailMenu`, default `true`, `game/src/GameVar.cpp:677`) drives `renderMenuQuad` (`game/src/Helper.cpp:248`), upstream code. Off (`game/src/Helper.cpp:342`): one smooth translucent vertical gradient. On (`game/src/Helper.cpp:296`): a two-part gradient with a hard step at mid-height, and four full-size black quads at alpha 0.25 meant as a 1 px shadow, which darken the whole panel by about 68%.
- **Why it matters:** the default setting gives the worse-looking, less translucent menus.
- **Proposed changes:**
  - Change the default to `false` (one line).
  - When on, draw the shadow as a 1 px border only and remove the mid-height step, so "on" is a refined version of "off".
- **Suggested home:** a **[GUI]** change (owner approval), with menu work or the rebrand (PNS-20).
- **Status:** Proposed

### PNS-31: Finish the step 2 play-test checks

- **Found in:** Phase B step 2 close (2026-10-10); the owner merges step 2 with these checks open.
- **Extract:** owner: "task 2.2, windows client play-test have to be checked later, re-test mouse-wheel i will do on next linux build check."
- **Open checks:**
  - Task 2.2 cross-OS game (Linux servers, macOS and Windows clients).
  - Windows client run on a PC with an OpenGL 2.1 driver.
  - Task 2.5 wheel fix re-test on Linux.
  - Results rows in `phase-a-manual-test.md` (task 2.6).
- **Suggested home:** the next Linux build check (wheel); the cross-OS game and Windows client run when a suitable PC is available; the Results rows before step 4 (regression baseline, PNS-1).
- **Status:** Accepted (owner-deferred)
