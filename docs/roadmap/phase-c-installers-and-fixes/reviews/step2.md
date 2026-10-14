# Step 2 review

Reviewer: fresh-context subagent, /thermo-nuclear-code-quality-review, one round (2026-10-14)
Diff: 7da808d..0ec8643 (history then rebuilt; see #1)
Skill evidence: `SKILL: Launching skill: thermo-nuclear-code-quality-review` (raw report: [step2-report.md](step2-report.md))

## review.sh

`tools/review.sh --full macos-arm64`: build and ctest 21/21 PASS. The scope check lists step 1's files until PR #50 merges into `main`; it is rerun after that merge.

## Findings

| # | Finding | Outcome |
|---|---|---|
| 1 | Commits 2.2 and 2.3 don't build: the `CSurvey` deletion was in 2.2 | fixed: branch rebuilt before any PR; the deletion is in 2.4 (6c89247) and every commit builds |
| 2 | Credits frame may overflow | owner checked on screen (2026-10-14): works; thank-you line reworded by the owner |
| 3 | Acceptance grep hits the ID-gap comments | fixed (comments reworded) |
| 4 | Dead comments in the intro-to-menu switch | fixed |
| 5 | Three copies of the "LOADING" frame | fixed for Scene and IntroScreen (`IntroScreen::renderLoading`); `Map.cpp` deferred: PNS-59 |
| 6 | Splash shows "LOADING" after loading ends | rejected: owner-approved design (PNS-53); removing the splash is the owner's call |
| 7 | Stale system overview line | fixed |
| 8 | ARCHITECTURE row order and `IntroScreen.h` row | fixed |
| 9 | Placeholder `HeadGames.tga`/`RnDLabs.tga` no longer loaded | deferred: PNS-59 |
| 10 | No automated test for rendering | accepted: owner's on-screen check below; `lang_check` covers `en.lang`/`en.pot` |
| 11 | Leftover blank lines; `explicit` constructor | fixed |

## Owner check (macOS, 2026-10-14, original data)

- Window title "Robo Violence 2" (also seen in the window list); "LOADING" splash; Credits show the fork block above the original list. The owner changed the thank-you to "Thanks to everyone for this game!".

## Not done / unsure

- Windows and Linux not checked on screen.
