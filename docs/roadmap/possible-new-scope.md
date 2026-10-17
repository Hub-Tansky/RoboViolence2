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

### PNS-5: Pre-existing UBSan defects

- **Found in:** `ARCHITECTURE.md` Open items, PR #2, PR #5.
- **Extract:** "UBSan reports pre-existing defects (out-of-range `bool` loads, NaN casts in `CUserLogin.cpp`)."
- **Why:** Undefined behaviour in shipped code, not yet in the R1–R15 register.
- **Suggested home:** §A.5; add them to `docs/analysis/KEY_QUESTIONS.md`.
- **Status:** Done in Phase B step 5: the smoke job's two `bool` loads (`Console.cpp`, `CMaster.cpp`) fixed; the `CUserLogin.cpp` NaN casts no longer appear in the smoke job

### PNS-6: Lifecycle null-pointer crashes are a pattern

- **Found in:** PR #10, PR #5, PR #2.
- **Extract:** "`~Scene()` calls `Scene::disconnect()`, which sets `menuManager.root->visible` without a null check" (PR #10); "Launch-script `set sv_*` before `dedicate` called into a null `Server`" (PR #5).
- **Why:** Three separate null dereferences in start-up and teardown. More are likely, and §A fuzzing will hit them.
- **Suggested home:** §A.2 (audit start-up and teardown paths under ASan).
- **Status:** Done in Phase B step 5 for the server (11 start/quit sequences under ASan) and the client teardown the smoke job runs; quitting at each client scene is left to play-tests

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

## Deferred from Phase B planning (2026-10-07)

Items of the former Phase B plan (`future-phases.md`, deleted) that are not in a [Phase B step](phase-b-security-infrastructure-anti-cheat/README.md). Extracts quote that plan.

### PNS-12: Headless sim core, player commands, seeded RNG

- **Source:** former §C.1, §C.2, §C.4; absorbs PNS-3 (GLU in the server) and PNS-7 (static init order).
- **Extract:** "Extract the game rules with no rendering, audio, UI or networking: `Game`, `Player` physics, `Map` collision and ray march, `Weapon`, `Projectile`, game modes, spawning." "A `PlayerCommand` struct: tick number, move vector, aim direction, fire …" "Replace global `rand()` with a per-`Game` seeded RNG."
- **Why deferred:** Owner decision (2026-10-07). Large refactor of every `#ifndef CONSOLE` block; needed only for replays, server-authoritative movement and information hiding (PNS-13). Phase B's `ServerValidator` (step 8) is the layer these plug into later.
- **Suggested home:** first steps of the phase after B; needs the Phase B manual test as a baseline.
- **Status:** Accepted → Phase D (not yet planned)

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

### PNS-16: Unsigned installers

- **Source:** former §D distribution; split by the owner (2026-10-14) into this entry, PNS-49 (signing), PNS-50 (stores) and PNS-52 (Flatpak).
- **Extract:** "Windows 11: signed installer or zip; macOS: codesign + notarisation, DMG; Linux: AppImage and/or Flatpak (+ `.deb`); optionally Steam or itch.io."
- **Scope (owner, 2026-10-14):** Windows 10 and 11: `.exe` installer; macOS: `.dmg`; a source tarball. None signed (macOS keeps the ad-hoc signature, `packaging/make-package.py`). Built by CI and attached to GitHub releases. Linux keeps the CI `.tar.gz` package until Flatpak (PNS-52). AppImage and `.deb` dropped: Flatpak covers distro package managers (e.g. Mint's).
- **Original assets:** not shipped. Packages already carry generated placeholders and read the player's data through `BV2_DATA_DIR` (`game/src/Paths.cpp:144`); PNS-51 removes the env var and keeps packaged content loading.
- **Why it matters:** friends install without building or unpacking archives.
- **Suggested home:** after PNS-51; the PNS-23 ADR with it.
- **Status:** Done in Phase C [step 3](phase-c-installers-and-fixes/step3-unsigned-installers.md)

### PNS-17: Networking rework

- **Source:** former §E; absorbs PNS-9 (`std::byte` disabled on Windows).
- **Extract:** "Move coord frames and, later, C.2 commands to UDP … only **after** auditing babonet's UDP / `cPeer2Peer` path" "Replace raw `memcpy` structs with explicit, versioned serialisation" "Tick-stamped snapshots and delta compression"
- **Why deferred:** Big protocol change; prerequisite of server-authoritative movement and information hiding (PNS-13). Phase B hardens the current TCP protocol instead (steps 4, 10).
- **Suggested home:** with PNS-12.
- **Status:** Accepted → Phase D (not yet planned)

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
- **Suggested home:** its own phase, in parallel from now on.
- **Owner decision (2026-10-14):** engine-only releases (code, generated placeholders, the player supplies the BV2 data) may ship publicly before H.1 (Phase C, PNS-16). Flathub (PNS-52), Steam and itch.io (PNS-50) wait for H.1.
- **Status:** Proposed (owner-deferred, 2026-10-14)

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
- **Status:** Done in Phase C [step 4](phase-c-installers-and-fixes/step4-housekeeping.md)

### PNS-23: ADR for the play-test package format

- **Found in:** Phase B step 1 fresh-context review (2026-10-07), finding 6.
- **Extract:** "Step task 1.2 asks for `.sh` and `.ps1` run scripts; `.cmd` was delivered … The new distribution format (archive layout, unarchived artifact upload, duplicate bundle `main/`, ad-hoc re-sign) has no ADR either."
- **Why it matters:** A later agent might "fix" `.cmd` back to `.ps1`, drop the second macOS `main/`, or re-zip the artifacts without knowing why they are as they are (`packaging/make-package.py`, step 1 task 1.2).
- **Suggested home:** a short ADR in the next PR that touches packaging, or with PNS-16 (signed distribution). The owner asked to check it further before writing one.
- **Status:** Done in Phase C [step 3](phase-c-installers-and-fixes/step3-unsigned-installers.md)

### PNS-24: Remove the unused runtime `bv2.db`

- **Found in:** Phase B step 1 fresh-context review (2026-10-07), finding 10.
- **Extract:** "the build still generates `runtime/bv2.db` (`tools/CMakeLists.txt:11-25` …). The game never reads it: `openClientDb` uses the pref dir (`game/src/Paths.cpp:275`)."
- **Why it matters:** Dead build output that the docs have to explain, and a second copy of client-database seed data.
- **Suggested home:** any step touching `tools/CMakeLists.txt`; check first that nothing seeds the pref-dir `bv2.db` from it.
- **Status:** Done in Phase C [step 4](phase-c-installers-and-fixes/step4-housekeeping.md)

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
- **Status:** Done in Phase C [step 3](phase-c-installers-and-fixes/step3-unsigned-installers.md)

### PNS-27: Pin the Windows CI runner image

- **Found in:** Phase B step 2a fresh-context review (2026-10-10), finding 1.
- **Extract:** "The CI log of run 38002113508 shows `Operating System: Microsoft Windows Server 2025`, `Image: windows-2025-vs2026` … Optionally pin `windows-2025` instead of `windows-latest` so the fact can't drift silently."
- **Why it matters:** The docs and ADR 0012 name the Windows version CI tests on; `windows-latest` can change under them. `.github/workflows/` is outside step 2a's Allowed paths.
- **Suggested home:** the next step that touches `.github/workflows/build.yml` (step 3 adds a fuzz job there).
- **Status:** Done in Phase C [step 3](phase-c-installers-and-fixes/step3-unsigned-installers.md)

### PNS-29: Shareable debug-capable Windows packages

- **Found in:** owner decision on step 2b (2026-10-10): CI ships Release packages only.
- **Extract:** "investigation if i can change software stack to be able to distribute debug packages (replacement of Windows Debug package)".
- **Why it matters:** a Windows Debug build links Microsoft's debug C++ runtime, which may not be redistributed, so testers can't get a crash-friendly build.
- **Suggested home:** a small investigation step. Candidates: RelWithDebInfo packages (release runtime plus `.pdb` symbols, redistributable); a toolchain whose debug runtime is redistributable (clang/LLVM-MinGW with libc++). Compare crash-report value, size and CI time.
- **Status:** Proposed

### PNS-30: "High detail menus" looks worse than off

- **Found in:** owner play-test on macOS and Linux, Phase B step 2 (2026-10-10).
- **Extract:** Options → "High detail menus" (`r_highDetailMenu`, default `true`, `game/src/GameVar.cpp:677`) drives `renderMenuQuad` (`game/src/Helper.cpp:248`), upstream code. Off (`game/src/Helper.cpp:342`): one smooth translucent vertical gradient. On (`game/src/Helper.cpp:296`): a two-part gradient with a hard step at mid-height, and four full-size black quads at alpha 0.25 meant as a 1 px shadow, which darken the whole panel by about 68%.
- **Why it matters:** the default setting gives the worse-looking, less translucent menus.
- **Change (owner-approved [GUI], 2026-10-14):** default `r_highDetailMenu` to `false` (one line). The drawing fix is PNS-48.
- **Status:** Accepted → [Phase C](phase-c-installers-and-fixes/README.md)

### PNS-31: Finish the step 2 play-test checks

- **Found in:** Phase B step 2 close (2026-10-10); the owner merges step 2 with these checks open.
- **Extract:** owner: "task 2.2, windows client play-test have to be checked later, re-test mouse-wheel i will do on next linux build check."
- **Open checks:**
  - Task 2.2 cross-OS game (Linux servers, macOS and Windows clients).
  - Windows client run on a PC with an OpenGL 2.1 driver.
  - ~~Task 2.5 wheel fix re-test on Linux~~: passed (owner, 2026-10-12).
  - Results rows in `phase-a-manual-test.md` (task 2.6).
- **Suggested home:** the next Linux build check (wheel); the cross-OS game and Windows client run when a suitable PC is available; the Results rows before step 4 (regression baseline, PNS-1).
- **Status:** Accepted (owner-deferred)

### PNS-32: In-game menu sits left of centre in fullscreen

- **Found in:** owner play-test of the step 2b macOS Release package, fullscreen (2026-10-10).
- **Extract:** "menu during play is shifted a little to the left. It is not centered." Screenshot (1703 px wide): the team and weapon buttons span x 46–1598, leaving 46 px left and 105 px right.
- **Why it matters:** cosmetic; every player sees it in fullscreen. Cause not analysed (likely the menu's fixed layout on a wide or HiDPI screen).
- **Suggested home:** a **[GUI]** change (owner approval), with HiDPI and ultrawide scaling (PNS-18).
- **Status:** Proposed

### PNS-33: Build `bv2dedicated` from `bv2server_lib`

- **Found in:** Phase B step 3 fresh-context review (2026-10-10), finding 8.
- **Extract:** "The whole server is compiled twice: `bv2dedicated` and `bv2server_lib` share the same `BV2_SERVER_SOURCES` minus main.cpp … The bigger simplification is `bv2dedicated = main.cpp + bv2server_lib`."
- **Why it matters:** every test build compiles the server twice, and the two targets can drift apart in flags.
- **Suggested home:** a build-only change to `game/CMakeLists.txt` in any later step that touches it; check the harness still links its fake `bb_*` first.
- **Status:** Done in Phase C [step 4](phase-c-installers-and-fixes/step4-housekeeping.md)

### PNS-34: Make `CString`'s format constructor explicit

- **Found in:** Phase B step 4 fresh-context review (2026-10-11), finding 4.
- **Extract:** "`console->svChange(svChange.svChange)` … `TimedMessage(chat.message)` … Neither is a visible `CString(` call … Making the format constructor `explicit` would close the whole class of bugs."
- **Why it matters:** `CString(char* fmt, ...)` is implicit, so passing wire text anywhere a `CString` is expected runs it as a printf format (R16, R21–R23 and the two client cases). Step 4 fixed the sites it found; an `explicit` constructor would make the compiler find the rest.
- **Suggested home:** step 7 (medium/low defects), or a step that touches `engine/zeven/include/CString.h`; expect many call-site changes.
- **Status:** Accepted → [Phase C](phase-c-installers-and-fixes/README.md)

### PNS-36: Update docs that still describe R3 as current

- **Found in:** Phase B step 5 fresh-context review (2026-10-11), finding 4.
- **Extract:** "`AGENTS.md:75` still lists the `sv_serverType = 1` assignment as a load-bearing known defect. `docs/analysis/README.md:27` and `ALGORITHM_03` W1 (`:230`) describe the bug as current … `AGENTS.md:57` says 'the non-Pro ruleset … removed'."
- **Why it matters:** an agent reading them could undo ADR 0013.
- **Suggested home:** the next step allowed to edit `AGENTS.md` and `docs/analysis/`; point each at ADR 0013.
- **Also (step 6):** `masterserver/README.md` still says `cMasterServer.cpp` builds SQL with `sprintf`; it binds since Phase B step 6.
- **Status:** Done in Phase C [step 4](phase-c-installers-and-fixes/step4-housekeeping.md)

### PNS-37: One read primitive for `FileIO` and `MemIO`

- **Found in:** Phase B step 5 fresh-context review (2026-10-11), findings 6 and 9.
- **Extract:** "Make `read(void*, size_t)` the one protected virtual primitive in `FileIO` … delete about 20 overrides from `MemIO`." "Extract `static map_cell * readCells(FileIO &, CVector2i & size)` … use it in all 8 places."
- **Why it matters:** every getter (and the R16 `getString` bound) exists twice; the map loader repeats the size check and cell loop eight times.
- **Suggested home:** step 7 or a refactoring step that touches `FileIO`/`MemIO`/`Map.cpp`.
- **Status:** Proposed

### PNS-38: Open defects after step 5 (R28, R29)

- **Found in:** Phase B step 3 (R28) and step 5 (R29).
- **Extract:** KEY_QUESTIONS R28 "On Windows, babonet intermittently crashes … when a TCP connection closes"; R29 "`~_typLayer` calls `glDeleteTextures` when `textureID == 0` … `loadTexture` dereferences a null `ptrLayer`".
- **Why it matters:** R28 can crash the Windows game and master when a player leaves; the master test is off on Windows until it is fixed. R29 leaks every DKT texture.
- **Suggested home:** step 7 (R29 is in `engine/dko/src/ePTexture.h`); R28 needs a Windows debugging session (a crash dump from the owner's PC or a Windows CI run with a debugger).
- **Status:** Accepted → [Phase C](phase-c-installers-and-fixes/README.md)

### PNS-39: Why did the dropped-flag radii differ?

- **Found in:** Phase B step 7, R12 (2026-10-11).
- **Extract:** owner: "0.25 but put it in possible new scope to check why there was a difference and maybe re-enable it". The blue dropped flag used 0.5 map units and the red one 0.25 (`game/src/ServerCTF.cpp`); step 7 set both to 0.25.
- **Why it matters:** the difference may have been a deliberate balance for a map side, or a typo; players who knew the old behaviour may notice.
- **Suggested home:** a gameplay review with the regulars (play-test both values); restoring a difference needs an ADR.
- **Status:** Proposed

### PNS-40: Temp-ban as a validator action

- **Found in:** Phase B step 8 (2026-10-11).
- **Extract:** step 8 lists the validator actions "log, warn, kick, temp-ban"; it ships kick only, and step 9 adds warn.
- **Why it matters:** a kicked cheater reconnects at once. A temp-ban needs a ban list on the dedicated server (today only the master bans, by its password, Q-S5).
- **Suggested home:** step 10 (server credentials) or step 12 (remote admin).
- **Status:** Proposed

### PNS-41: The step 8 frame check flags lag stalls

- **Found in:** Phase B step 9 (2026-10-11).
- **Extract:** coord frames held back more than about 0.5 s across a 3 s window's start give `clientFrames > serverFrames + 5 + 10%` (`ServerValidator::onCoordFrame`); three such windows in a row kick (`sv_speedHackKick`).
- **Why it matters:** a player on a link that stalls every few seconds can be kicked as a speed hacker. The rule is unchanged since upstream.
- **Suggested home:** tune it from the step 9 play-test log (time the window by the client frames, or count only the drift beyond a stall).
- **Status:** Accepted → Phase D (not yet planned)

### PNS-42: Admin login attempts and weak passwords

- **Found in:** Phase B step 10 (2026-10-12).
- **Extract:** the join and admin proofs are HMACs keyed by the password over a public nonce (ADR 0014). Someone who records one can test password guesses offline; the server accepts unlimited `ADMIN_REQUEST`s (no budget in step 9's rate limits).
- **Why it matters:** a short `zsv_adminPass` or `sv_password` can be found from one captured login.
- **Suggested home:** step 12 (remote admin): an attempt budget per connection, a minimum password length for `zsv_adminPass`; a PAKE or TLS if passwords must survive capture.
- **Status:** Done in Phase C [step 6](phase-c-installers-and-fixes/step6-server-and-master.md)

### PNS-43: Join-time identity gaps left after step 10

- **Found in:** Phase B step 10 review (2026-10-12).
- **Extract:**
  - The server accepts `PLAYER_INFO` before the password handshake (`Server::checkPacket`): on a passworded server an unauthenticated connection triggers the name broadcast, the account request and the master ban query.
  - Every relayed `PLAYER_INFO` carries each player's IP to all clients.
  - The master's ban and unban messages are authorised by a hard-coded password (`roxbabo`) sent in cleartext.
- **Why it matters:** privacy (IPs), and anyone can add master bans.
- **Suggested home:** step 12 (remote admin) for the ban password; a protocol step for the join order and IPs.
- **Status:** Master ban password done in Phase C [step 6](phase-c-installers-and-fixes/step6-server-and-master.md) (wire format: PNS-66); join order and relayed IPs → Phase D with PNS-17

### PNS-44: The master lists a server at its connection's address

- **Found in:** Phase B step 11 (2026-10-12).
- **Extract:** `bv2Row.ip` is left empty (`game/src/CMaster.cpp`, `sendGameInfo`); the master records the address the heartbeat came from. In the compose example that is the server container's internal address.
- **Why it matters:** remote clients can't join a co-located server from the browser; they must `connect` by hand.
- **Suggested home:** an advertised address (`sv_publicAddress` or `BV2_PUBLIC_ADDRESS`) sent in the heartbeat and trusted by the master only when it matches a configured server.
- **Status:** Done in Phase C [step 6](phase-c-installers-and-fixes/step6-server-and-master.md)

### PNS-45: AGENTS.md and the CI list miss the containers workflow

- **Found in:** Phase B step 11 review (2026-10-12).
- **Extract:** AGENTS.md lists the required CI checks; `containers` (step 11) is not there, and whether it should be required is undecided.
- **Why it matters:** a broken image can merge unnoticed unless the check is required.
- **Suggested home:** the next step that may edit AGENTS.md; the owner decides on branch protection.
- **Status:** Done in Phase C [step 4](phase-c-installers-and-fixes/step4-housekeeping.md)

### PNS-46: Remote admin leftovers

- **Found in:** Phase B step 12 review (2026-10-12).
- **Extract:**
  - With `RA_LOGIN` refused, the rest of the legacy UDP remote admin is unreachable but still compiled: the `RA_*` cases in `game/src/CMaster.cpp` (one writes files), the peers' `remoteAdmin` state, and the `RA_ConsoleBroadcast`, `RA_NewPlayer` and `RA_PositionBroadcast` calls on every console line.
  - `BV2_ADMIN_API_PORT`, `BV2_ADMIN_API_BIND` and `BV2_ADMIN_API_PROXY` are not in the environment lists of AGENTS.md and config/README.md.
- **Why it matters:** dead network code, and operators reading the config docs miss the admin API.
- **Suggested home:** a cleanup step that may edit `game/src/CMaster.*`, AGENTS.md and config/README.md.
- **Status:** Done in Phase C [step 6](phase-c-installers-and-fixes/step6-server-and-master.md)

### PNS-47: Old saved `languageFile` pins English

- **Found in:** Phase B step 13 Linux check (2026-10-13).
- **Extract:** builds before step 13 saved `languageFile "main/languages/en.lang"` in the user's `bv2.cfg`; a saved value wins over the OS language (`game/src/GameVar.cpp` `reloadLanguage`), so upgraded players never get the OS default. `content/languages/README.md` only documents the workaround.
- **Why it matters:** translations stay invisible to existing players until they edit their config.
- **Suggested home:** PNS-21 or the next step touching `GameVar.cpp`: treat the old default value as empty once, on load.
- **Status:** Accepted → [Phase C](phase-c-installers-and-fixes/README.md)

### PNS-48: Fix the "High detail menus" drawing

- **Found in:** split from PNS-30 (owner, 2026-10-14).
- **Extract:** On (`game/src/Helper.cpp:296`): a two-part gradient with a hard step at mid-height, and four full-size black quads at alpha 0.25 meant as a 1 px shadow.
- **Why it matters:** players who turn the option on get darker menus with a visible step.
- **Proposed change:** draw the shadow as a 1 px border only and remove the mid-height step, so "on" is a refined version of "off".
- **Suggested home:** a **[GUI]** change (owner approval), with menu work or the rebrand (PNS-20).
- **Status:** Proposed

### PNS-49: Code signing and notarisation

- **Found in:** split from PNS-16 (owner, 2026-10-14).
- **Extract:** "Windows 11: signed installer …; macOS: codesign + notarisation".
- **Why it matters:** removes SmartScreen and Gatekeeper warnings. Needs an Apple Developer account, a Windows code-signing certificate and CI secrets.
- **Suggested home:** before a wider public release (after PNS-20 H.1).
- **Status:** Proposed

### PNS-50: Steam and itch.io

- **Found in:** split from PNS-16 (owner, 2026-10-14).
- **Extract:** "optionally Steam or itch.io."
- **Why it matters:** store reach. Stores need the game to ship its own assets, so it waits for PNS-20 H.1 and the rebrand.
- **Suggested home:** after PNS-20 and PNS-49.
- **Status:** Proposed (owner-deferred)

### PNS-51: Installed game finds the player's BV2 data

- **Found in:** Phase C planning (2026-10-14), checking PNS-16 against `game/src/Paths.cpp`.
- **Extract:** `pathsInit` picks one data root: `BV2_DATA_DIR`, else the folder beside the executable (`game/src/Paths.cpp:139–180`). With `BV2_DATA_DIR` set, the package's own `main/` is ignored, so its launch scripts and translations don't load (`content/languages/README.md:23`).
- **Why it matters:** setting a permanent environment variable is hard for players (and impossible inside a Flatpak, PNS-52), and translations shipped in the package stay invisible.
- **Proposed change:** layer two roots: the player's BV2 data for assets, the package's `main/` for content the repo owns (launch scripts, language files). Then locate the player's data without an env var.
- **Options for locating the data** (owner decides; ADR):

  | Option | How the player does it | Notes |
  |---|---|---|
  | A. Known folder | Copies BV2's `main/` into a fixed folder in the pref dir | No UI; the folder is hidden on macOS and Windows, so it needs written instructions |
  | B. Saved path | Sets a path once (config line or `--data-dir`); the game remembers it | No UI; editing a config file is as hard as an env var |
  | C. Auto-detect | Nothing, if BV2 is installed in a usual place (e.g. `C:\Program Files\BaboViolent 2`) | Works only for installed copies; a first try before D |
  | D. First-run picker **[GUI]** | When no data is found, a native message box explains what is needed, then the OS folder dialog (`SDL_ShowOpenFolderDialog`); the choice is validated and saved (as B) | Native OS dialogs, not BV2 menus; English-only text at first |
  | E. Installer page | The Windows installer asks for the folder | Windows only; the `.dmg` and the Linux package can't ask |
  | F. Import | After D, copy the assets into the pref dir | The original folder can move; costs disk space |

- **Decision (owner, 2026-10-14; D is owner-approved [GUI]):** lookup order `BV2_DATA_DIR` → saved path (B) → auto-detect (C) → first-run picker (D) that saves to B. Cancelling D starts with placeholders and says so. A, E and F dropped.
- **Suggested home:** Phase C, before PNS-16.
- **Status:** Done in Phase C [step 1](phase-c-installers-and-fixes/step1-player-data-dir.md)

### PNS-52: Flatpak on Flathub

- **Found in:** split from PNS-16 (owner, 2026-10-14).
- **Extract:** "Linux: AppImage and/or Flatpak (+ `.deb`)".
- **Scope:** delivered through Flathub, so it shows in distro software managers (e.g. Linux Mint's). Choose the permanent app ID in an ADR; the sandbox must reach the player's data (PNS-51) or ship its own.
- **Why it matters:** the standard way to install on any Linux distro.
- **Blocked by:** PNS-20 (owner, 2026-10-14).
- **Suggested home:** Phase D.
- **Status:** Accepted → Phase D (not yet planned)

### PNS-53: In-game name and credits

- **Found in:** owner review of Phase C (2026-10-14).
- **Extract:** [ADR 0008](../decisions/0008-display-name-robo-violence-2.md): "§H must rebrand `en.lang` (`lang_gameName`), credits and window titles before any public release"; never imply endorsement by the original authors.
- **Visible today:**
  - `content/languages/en.lang:1` `lang_gameName` "Babo Violent 2": the window title (`game/src/main.cpp:460`).
  - `game/src/CCredit.cpp:38` title "BaboViolent 2 | Team"; no mention of the fork.
  - `game/src/GameVar.cpp:372` and `config/bv2.example.cfg:38` `sv_gameName` "Babo Violent 2 - Server": the name in server browsers; `game/src/main.cpp:300` dedicated-server banner.
  - `game/src/CUserLogin.cpp:52`: "register on the official forum www.rndlabs.ca" (dead site, implies an official channel).
  - `game/src/IntroScreen.cpp:31`: RndLabs logo splash at start-up (from the player's data).
- **Change (owner-approved [GUI], 2026-10-14):**
  - Game name Robo Violence 2 in the window title, server name and dedicated-server banner.
  - Credits, in order: title "Robo Violence 2"; "Unofficial fork of BaboViolent 2 by bitHeads / RndLabs; not endorsed by them."; "Thanks to everyone for this game!"; header "Original credits below"; the original team list unchanged.
  - Remove the forum sentence from the login note.
  - Replace the RndLabs logo splash with a new splash showing "Loading" in a big font.
  - Remove dead code: the survey (`game/src/CSurvey.*`, its `Scene` and `CMaster` hooks, the `DidSurvey` seed row in `content-seed/bv2.sql`, the master's `SURVEY_*` handling in `masterserver/src/cNetManager.cpp`; message IDs stay as gaps) and the unused `lang_RndLabsTeam` (`GameVar.*`, `en.lang`).
  - Regenerate `en.pot`.
- **Unchanged:** source copyright notices; in-game terms ("Babo", "Unnamed Babo", "Counter Baborist"); binaries, `BV2_*`, pref path `BaboViolent2`, master game ID (PNS-20).
- **Depends on:** PNS-51, otherwise the player's original `en.lang` overrides the name.
- **Suggested home:** Phase C, before the first release; an ADR amending 0008 and 0010.
- **Status:** Done in Phase C [step 2](phase-c-installers-and-fixes/step2-name-and-credits.md)

### PNS-54: miniaudio 0.11.25 has CVE-2026-32837

- **Found in:** Phase C step 4 (2026-10-14), first run of `tools/osv-vcpkg.py`.
- **Extract:** OSV: "heap out-of-bounds read … in the WAV BEXT metadata parser … crafted WAV files"; fixed upstream in miniaudio commit `1df46ae`; CVSS 4.0 `AV:L … VA:L`.
- **Why it matters:** the client decodes `.wav` sounds from the player's data with miniaudio. Exposure is local files only (sounds are not downloaded from servers), so a crash at worst.
- **Suggested home:** the next vcpkg baseline bump once the `miniaudio` port ships the fix (check `osv-vcpkg`), or an overlay port; any step allowed to edit `vcpkg.json`.
- **Status:** Proposed

### PNS-55: Two copies of the client database schema

- **Found in:** Phase C step 4 review (2026-10-14), finding 1.
- **Extract:** "`content-seed/bv2.sql` and `game/src/Paths.cpp` (openClientDb) are now two copies of the same client schema. Nothing consumes bv2.sql any more."
- **Why it matters:** the two will drift; step 2 already has to edit both (the `DidSurvey` row, PNS-53).
- **Suggested home:** step 2 (PNS-53 touches both): delete `content-seed/bv2.sql` and make `openClientDb` the schema, or embed the file and run it.
- **Status:** Proposed

### PNS-56: Server browser ping is always `???`

- **Found in:** Phase B manual test, macOS (2026-10-14).
- **Extract:** with master, server and client on one Mac, the browser lists the server with ping `???` (1000 ms or more). `CMaster::update` pings each row over a babonet UDP peer (`game/src/CPing.cpp`, `bb_peerSend` to the game port); no answer arrives.
- **Why it matters:** players can't tell near servers from far ones; the browser sorts by ping.
- **Suggested home:** Phase C step 6 (server and master) or a client fix after the manual test.
- **Status:** Proposed

### PNS-57: Packaged master and server listen on all interfaces

- **Found in:** Phase T step 1 prototype (2026-10-14).
- **Extract:** `lsof` on the packaged `bv2master` during a local test: `TCP *:10207 (LISTEN)`; the game server likewise on 3333.
- **Why it matters:** the playtest agent's disposable local server is reachable from the LAN for the test's duration (ADR 0017), and a first run of an unsigned build may trigger the macOS firewall prompt. A bind-address option (like `zsv_adminApiBind`) would keep local tests on loopback.
- **Suggested home:** Phase C step 6 (server and master) or a later networking step.
- **Status:** Proposed

### PNS-58: Copied helpers in the playtest and staging agents

- **Found in:** Phase T step 2 review (2026-10-14), structure note.
- **Extract:** "`deploy.sh` copies about 8 helpers from `tools/playtest/agent.sh` … a fix to one copy (e.g. step 1's lock fixes #24, #32, #39) must be ported by hand to the other."
- **Why it matters:** the main-only run filter and the lock are security-relevant; the copies can drift. A shared lib was rejected in step 1 (#23) because each agent is installed as one file.
- **Suggested home:** the next Phase T step that changes either agent: a shared `tools/lib/agent-common.sh` with a one-line install for both, or a test that checks the two `pick_run` filters are identical.
- **Status:** Proposed

### PNS-59: Leftovers of the RndLabs splash and the loading frame

- **Found in:** Phase C step 2 review (2026-10-14), findings 5 and 9.
- **Extract:** `tools/placeholder-manifest.tsv:44,56` still generate placeholder `HeadGames.tga` and `RnDLabs.tga`, which no code loads now. `game/src/Map.cpp:85-91` repeats the "LOADING" frame that `IntroScreen::renderLoading` now draws.
- **Why it matters:** dead placeholder output, and a third copy of the loading frame.
- **Suggested home:** the next step that touches `tools/` or `Map.cpp`; keep the `tools/original-assets.sha256` entries.
- **Status:** Proposed

### PNS-60: Rename `bv2.example.cfg` to `robo2.defaults.cfg`

- **Found in:** Phase B manual test, Linux (2026-10-16); owner request.
- **Extract:** `config/bv2.example.cfg` is the original BV2 `bv2.cfg` with personal values blanked, and the first config layer (ADR 0007): its values, not `GameVar.cpp`, set the defaults players get (52 differ, among them `sv_serverType 1`, `sv_timeToSpawn 1` and the weapon damages). "Example" hides that; its `cl_playerName "Daivuk"` went unnoticed until fixed in PR #60.
- **Why it matters:** contributors edit the code defaults expecting a change in game and see none.
- **Suggested home:** with the rebrand of config and pref paths (PNS-20; roadmap ground rule 3 holds config paths until then). Rename the file and its copies in `CMakeLists.txt`, `packaging/` and `game/src/Paths.cpp`, keep reading the old name once, and update ADR 0007 and the docs that name it.
- **Status:** Proposed

### PNS-61: Windows CI link step fails on a locked file

- **Found in:** Phase B step 12 follow-ups (2026-10-13, 2026-10-14): `build (win-x64-msvc)` failed three times linking a test executable (`test_master.exe`, `test_babonet.exe`) with "The process cannot access the file because it is being used by another process"; a rerun passed.
- **Why it matters:** red `main` builds and PRs that need a manual rerun.
- **Suggested home:** Phase C step 3 (pins the Windows runner, PNS-27) or step 4: retry the build step once, or exclude the build folder from Defender on the runner.
- **Status:** Proposed

### PNS-62: The vcpkg cache key ignores the overlay triplets

- **Found in:** Phase C step 3 review (2026-10-14), finding 10.
- **Extract:** `.github/actions/setup-vcpkg/action.yml` keys `actions/cache` on `hashFiles('vcpkg.json')` only; `triplets/arm64-osx.cmake` changes every macOS port's ABI, so the old cache is restored, all ports rebuild, and an exact key hit skips saving the new binaries.
- **Why it matters:** macOS CI rebuilds SDL3 and the other ports on every run until `vcpkg.json` changes.
- **Suggested home:** the next step allowed to edit `.github/actions/`: add `triplets/**` to `hashFiles`. A baseline bump (PNS-54) also refreshes the key.
- **Status:** Proposed

### PNS-63: A licence file for the vendored glad loader

- **Found in:** Phase C step 3 review round 2 (2026-10-14), finding 4.
- **Extract:** `packaging/make-package.py` builds `licenses/glad.txt` from a hard-coded "glad 2.0.8 …: WTFPL OR CC0-1.0" and the first comment of `KHR/khrplatform.h`, repeating `engine/zeven/third_party/glad/README.md:3`.
- **Why it matters:** regenerating glad or changing the header's comment silently changes or breaks the notice.
- **Suggested home:** a step allowed to edit `engine/zeven/third_party/glad/`: commit `LICENSE.txt` there and copy it.
- **Status:** Proposed

### PNS-66: Master bans still travel as a 7-character cleartext password

- **Found in:** Phase C step 6 (2026-10-17), task 6.2.
- **Extract:** `stCacheBan.Pass[8]` / `stCacheUnban.Pass[10]` (`masterserver/src/cMSstruct.h`); the game's `cacheban` sends the first 7 characters typed. Step 6 replaced `roxbabo` with `BV2_MASTER_BAN_PASS` but kept the wire format.
- **Why it matters:** anyone on the path to the master reads the ban password; 7 characters are guessable.
- **Suggested home:** with accounts or a master protocol step (PNS-14, PNS-17): an HMAC over a master nonce, as ADR 0014 does for servers.
- **Status:** Proposed

### PNS-67: The master's game list keeps dead priority logic and two update paths

- **Found in:** Phase C step 6 review round 2 (2026-10-17), findings 1 and 2.
- **Extract:** `Priority` is always 0 since step 6, so `GAME_TIMEOUT * 5` (`masterserver/src/cBV2game.cpp`) and `lastHighPriority` in `cMasterServer::CreateGame` take one path; `CreateGame` has two identical "update existing game" blocks (the loop and the tail).
- **Why it matters:** a fix must be made twice (step 6 did); new games are inserted before the tail for no reason.
- **Suggested home:** the next step touching `masterserver/src/cMasterServer.cpp`: one loop over all games, append at the tail, plain timeout.
- **Status:** Proposed
