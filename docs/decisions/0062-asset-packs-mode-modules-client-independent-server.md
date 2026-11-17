# 0062: Asset packs, mode modules and a client-independent server are the long-term direction

- Status: Accepted
- Date: 2026-11-17
- Affects: [AGENTS.md](../../AGENTS.md) "Ground rules" 5 to 9, [REVIEW.md](../../REVIEW.md) section 2, [game/CONTEXT.md](../../game/CONTEXT.md); amends the direction of [ADR 0003](0003-keep-opengl-2.1-then-sdl-gpu.md); ROBO-230 (refactor direction Phase 0, owner started 2026-11-17)

## Context
- Owner goals: (1) a new look from replacement assets, released about 2027-02-11 (ROBO-020); (2) assets reusable across mini games, mini games easy to make, later by players; (3) the server usable by other clients on more platforms.
- Today an asset is a path literal in C++ (199 loader calls in `game/src`), a mode is a `GAME_TYPE_*` branch in 16 files (`game/src/Game.h:38`), rules and drawing share classes split by 320 `CONSOLE` preprocessor lines (counts from the handoff at `3d5b3b4`), and the protocol is raw structs with one exact version number. Every new asset path or mode branch is debt against goals 2 and 3.
- Plan and measurements: rv2-docs `30-dev/refactor-direction-handoff.md` (2026-11-11), owner decisions D1 to D4 answered.

## Decision
The code moves toward three seams, each landing with its test: an **asset seam** (code asks for an asset id; a pack manifest maps ids to files with author and licence; packs stack), a **rules seam** (a headless simulation; each mode a module behind one interface; the client draws what the simulation reports) and a **front seam** (a written, versioned protocol with a conformance corpus); from now on new code follows rules R1 to R8 below, and old code moves only in items of kind `refactor`, as a strangler on `main`, never a rewrite or a long-lived branch.

| # | Rule | Check |
|---|---|---|
| R1 | No new asset path literal in game code; a new asset gets an id in the manifest | grep `"main/` in the diff |
| R2 | No new `#ifdef CONSOLE` or `#ifndef CONSOLE`; rules code includes no GL, audio or input header | grep in the diff |
| R3 | No new `gameType ==` branch outside mode code | grep in the diff |
| R4 | Tunable numbers go in data files or cvars, not constructor calls | review |
| R5 | A new or changed network message gets explicit field-by-field read and write, a corpus sample and a line in the protocol doc (the `ARCHITECTURE.md` Network row until `docs/protocol.md` exists, Phase 2); messages carry ids, never file names or paths | corpus test |
| R6 | A new format is added beside the old one, never instead (ADR 0010) | review |
| R7 | Every seam lands with its test: goldens for look, corpus for protocol, harness test for rules | CI |
| R8 | Player-made content is data; player-made code runs only on the server, sandboxed, never natively on other players' machines | design review |

## Consequences
- R1 to R5 are ground rules 5 to 9 in AGENTS.md and Standards blockers in REVIEW.md; R6 to R8 are already covered (ground rule 1, the test blocker) or apply only from the mini game kit on.
- No mode code exists until the mode interface (Phase 2); until then a branch an item cannot avoid is named under Not done / unsure in its review record, as debt for that phase.
- Phase 1 (ROBO-231 to ROBO-234: manifest, registry, PNG/OGG/glTF/TTF loaders, pack order) runs before the release; the sim core (ROBO-012), protocol rework (ROBO-017) and renderer (ROBO-018) wait until after it.
- ADR 0003's SDL_GPU target is re-checked when ROBO-018 starts: if the web is a target platform the renderer needs a web backend.
- Rejected: a rewrite on another engine (loses the BV2 feel, the tested server and the netcode hardening); waiting with all of it until after the release (every asset added without an id is touched twice).
