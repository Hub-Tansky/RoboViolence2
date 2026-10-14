SKILL: Launching skill: thermo-nuclear-code-quality-review

Scope: `git diff 7da808d..HEAD` (0d35c71..0ec8643, 6 commits, 29 files, +120/-924), checked against the step 2 file, REVIEW.md §2, and the step 1 and step 4 records. I did not build, run ctest or modify anything. Acceptance grep: `git grep -i survey` finds only the "1016-1018 … don't reuse" comments in both `cMSstruct.h` and some docs. That passes in spirit, but the step's literal `grep -rni survey game/src masterserver/src engine` will hit `engine/babonet/include/cMSstruct.h:34` and `masterserver/src/cMSstruct.h:36` (see 3).

## Findings

1. **Medium: commits 2.2 and 2.3 don't build, and the survey deletion is in the wrong commit.** 22637e2 ("step2.2: credits … login note") deletes `game/src/CSurvey.cpp` and `game/src/CSurvey.h` (509 lines). But `game/CMakeLists.txt` still lists `src/CSurvey.cpp`, and `game/src/Scene.h` still has `#include "CSurvey.h"`, until e80696c (2.4). So 22637e2 and 88ce73c fail to configure or compile, which breaks bisect, and the 2.2 commit message hides a deletion that belongs to 2.4. This is not the "two tasks in one commit" pattern rejected in step 1 #15 / step 4 r2 #1: here the history doesn't build. Fix: before merge, move the two file deletions from 22637e2 into e80696c (`git rebase` with an edit, not `-i`: e.g. `git checkout 22637e2 -- …` reorder), or record it in `reviews/step2.md` and accept it explicitly.

2. **Medium (verify on screen): the credits frame may now overflow.** `game/src/CCredit.cpp:38-50`: the frame is a fixed 736x506 (`CCredit.cpp:33`) with no scrolling (`animY`/`velY` are set but never used). Labels stack with `CONTROL_SNAP_BOTTOM` (25 px + spacer, default 5, `CControl.h:172`). The new block adds 3 rows (+115 px), minus 15 from the Game Designer spacer, so the whole original list moves down about 100 px. By my count, the original stack already reached about 850 px from the frame top. So the tail ("Sunita", "Whole www.baboviolent.net community") is the part to check for clipping or spilling over the menu. Fix if clipped: on the owner's screen check, shrink the new rows (one combined line, smaller spacers), or cut the old 30 px section spacers to 15.

3. **Low: the acceptance grep conflicts with the new comments.** `engine/babonet/include/cMSstruct.h:34` and `masterserver/src/cMSstruct.h:36` contain "survey", so the step's grep literally "finds" something. Fix: reword them ("1016-1018: removed in Phase C step 2 (ADR 0017); don't reuse"), or change the acceptance check to exclude that comment.

4. **Low: dead code left in the block the diff touched.** `game/src/Scene.cpp:178` keeps `/* if (gameVar.s_inGameMusic) dksPlayMusic("main/sounds/Menu.ogg", -1);*/`, a commented duplicate of line 175, which the diff itself moved. Lines 170-171 keep `// menu->isReady` / `// ZEVEN_SAFE_DELETE(menuManager.root)`. Fix: delete line 178 (and the two stale comments) so the intro-to-menu transition reads in four lines.

5. **Low (code judo): three copies of the "LOADING" frame.** `game/src/Scene.cpp:66-79`, `game/src/Map.cpp:85-91` and now `game/src/IntroScreen.cpp:52-59` each repeat ortho 800x600, blend, bind font and `printCenterText(400, 268, 64, "LOADING")`. Fix: one helper, e.g. `IntroScreen::renderLoading(font, brightness)` or a `printLoading()` in `Helper.cpp`, called by Scene's loading frame and `render()`. `Map.cpp` and `Helper.cpp` are outside Allowed paths, so either do Scene + IntroScreen now and note Map in possible-new-scope, or note the whole thing.

6. **Low (product semantics): the splash shows "LOADING" after loading has finished.** `IntroScreen` runs for 3 s after `Scene::Scene()` has already loaded everything. The scope file requires this (2.3, owner-approved), so it is not a defect, but the owner should know when checking on screen. The bigger simplification would be deleting `IntroScreen` and its `introScreen` gating in `Scene::update`/`render`. That needs owner approval and is not a fix for this step.

7. **Low: stale doc.** `docs/analysis/01_SYSTEM_OVERVIEW.md:146` still says "Intro screen / survey gating". `docs/analysis/**` is in Allowed paths. Fix: "Intro screen gating".

8. **Low: ARCHITECTURE.md inconsistencies.** The new `0017` row (`ARCHITECTURE.md:250`) sits above the `0016` row, out of order. The `game/src/IntroScreen.h` row still says "Intro screen" while `IntroScreen.cpp` now says "LOADING" splash. Fix: put 0017 after 0016 and align the `.h` description.

9. **Low (out of scope, note only): placeholders for textures nothing loads.** `tools/placeholder-manifest.tsv:44,56` still generate placeholder `HeadGames.tga` and `RnDLabs.tga`, which no code loads now. `tools/` is outside Allowed paths, so add a line to `docs/roadmap/possible-new-scope.md`. The `tools/original-assets.sha256` entries must stay.

10. **Low: no automated test for the new behaviour.** The name strings are covered only indirectly (`lang_check` covers `en.lang`/`en.pot` consistency). Rendering (credits, splash) has no test and is waived to the owner's on-screen check. The survey removal is covered by the build and the grep. Fix: record the waiver and the owner check in `reviews/step2.md`, as step 1 did with "Not done".

11. **Nit: leftover blank lines.** Removed members leave double or triple blank lines: `game/src/CMaster.h:42-43`, `game/src/Scene.h:67-72`, `game/src/MenuSetup.cpp:44-46`. `IntroScreen(unsigned int)` could be `explicit`. Optional.

## Checked, no issue

- Survey removal is complete: client hooks, `CMaster` `SURVEY_CONFIRM`, master `SURVEY_SUBMIT`/`NEVER`, both `SSurvey` and their `static_assert`s, the `DidSurvey` seed row and the `openClientDb` row. No other wire struct changed, so no `GAME_VERSION` bump is needed. IDs 1016-1018 are left as gaps, nothing renumbered. No runbook, Docker or seed references to `survey.db` remain.
- The text still renders with GL_TEXTURE_2D disabled before `introScreen->render()`, because `CFont` enables it itself (`engine/zeven/src/CFont.cpp:437-439`).
- The fade matches the old curve. The server banner keeps its width.
- ADR 0017 exists, 0008 and 0010 Status lines are amended, and the README index is updated.
- [GUI] changes match PNS-53 and the owner's approval.
- No mechanical rewrite is mixed into a functional commit.
- No file passes 1000 lines; all touched files shrank.

## Verdict

Good design overall: a net deletion of about 800 lines, and nothing structural is wrong. Before closing, fix #1 (non-building intermediate commits; fix or explicitly accept in the record) and get the owner's on-screen check for #2. #3, #4, #7 and #8 are quick fixes. #5 and #9 are optional now or go to possible-new-scope; #6 is for the owner to decide. #10 and #11 need only a note in the record or are optional.
