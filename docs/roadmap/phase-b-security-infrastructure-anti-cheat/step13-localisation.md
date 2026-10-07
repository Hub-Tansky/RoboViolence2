# Step 13: Localisation foundations

**Status:** TODO

**Depends on:** [Step 2](step2-cross-os-playtest.md). **Next:** none (last Phase B step). **Index:** [README.md](README.md)

## Scope

| Field | Value |
|---|---|
| Goal | The client picks the OS language when no `languageFile` is set, loads gettext PO files with `.lang` fallback, and contributors can add a language from a template and a guide |
| In scope | Tasks 13.1–13.4 below |
| Out of scope | An in-game language picker, moving hard-coded strings, non-ASCII rendering (all PNS-21); new fonts (PNS-20); shipping translations other than English |
| Allowed paths | `game/src/GameVar.{h,cpp}`, `game/src/main.cpp`, `game/src/Lang*.{h,cpp}` (new), `game/CMakeLists.txt`, `content/languages/**`, `tools/lang-*`, `tools/check-hygiene.py`, `tests/**`, `ARCHITECTURE.md`, `docs/roadmap/**` |
| Inputs | `AGENTS.md`, `ARCHITECTURE.md`, [ADR 0011](../../decisions/0011-gettext-po-translations.md), [ADR 0010](../../decisions/0010-bv2-asset-compat-and-gui-freeze.md), PNS-2 |
| Deliverables | Locale detection; PO loader; `en.pot` generator; translation checker in `hygiene`; `content/languages/README.md` |
| Definition of done | See "Acceptance checks". All must pass. `ARCHITECTURE.md` is updated |

## Context

- `languageFile` defaults to `main/languages/en.lang` (`game/src/GameVar.cpp:271`) and is set only through config; `GameVar::loadLanguage` (`GameVar.cpp:1223`) reads `key<TAB>text`. There is no in-game picker (`game/src/OptionMenu.h` is a leftover header).
- `CFont` maps single bytes 33–159 (`engine/zeven/src/CFont.cpp:156`, PNS-2). Languages needing other glyphs can't render until PNS-20/21.
- ADR 0011: PO with `msgctxt` = key, `msgid` = English, `msgstr` = translation; `<ll>[_<CC>].po`; per-key English fallback. Original `.lang` files keep loading (ADR 0010).

## Tasks

### 13.1 OS language default

- An empty `languageFile` (the new default) means auto. `SDL_GetPreferredLocales()` → exact `ll_CC`, then `ll`, else English. A saved `languageFile` always wins.
- Auto-detection skips a language whose strings use characters the loaded font lacks; setting `languageFile` by hand still loads it.

### 13.2 PO loader

- Minimal PO parser (msgctxt, msgid, msgstr, escapes, multi-line). Loads `<locale>.po`, then `<locale>.lang`; missing keys fall back to English.

### 13.3 Template and checker

- `tools/lang-to-pot.py` generates `content/languages/en.pot` from `en.lang`.
- `tools/check-translations.py`, run by the `hygiene` job: valid PO, no unknown keys, `en.pot` in sync, missing keys reported per language.

### 13.4 Translator guide

- `content/languages/README.md`: copy `en.pot` to `<ll>.po`, edit in Poedit or Weblate, naming, the glyph limit until new fonts.

## Critical files

- `game/src/GameVar.cpp`, `game/src/main.cpp`, `content/languages/en.lang`

## Acceptance checks

```bash
ctest --test-dir build/linux-x64 --output-on-failure -R lang
```

- Passes: the PO parser, the fallback order, the locale matching and the glyph skip are covered.
- With `LANG=pl_PL.UTF-8` and a test `pl.po` containing only ASCII, the Linux client shows Polish strings; with no PO file it shows English.
- With original BV2 data, `en.lang` loads and the menus look unchanged (owner).
- `python3 tools/check-hygiene.py` runs the translation check.
- [REVIEW.md](../../../REVIEW.md) checklist and `/anthropic-skills:thermo-nuclear-code-quality-review` done on the step diff; every finding fixed or confirmed with the owner.
- `ARCHITECTURE.md` lists every file added, moved or removed in this step.
- The secret scan (`gitleaks git --staged` / CI) is clean.
