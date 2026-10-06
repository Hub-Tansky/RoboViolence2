# 0011: Use gettext PO for translations, keep .lang loading, default to the OS language

- Status: Accepted
- Date: 2026-10-06
- Affects: [Phase B step 13](../roadmap/phase-b-security-infrastructure-anti-cheat/step13-localisation.md), [PNS-21](../roadmap/possible-new-scope.md#pns-21-localisation-beyond-the-foundations)

## Context
- Strings live in `content/languages/en.lang` (`key<TAB>text`), loaded from `languageFile` (`game/src/GameVar.cpp:271`); English is the only language.
- Translators' tools (Poedit, Weblate, Crowdin) speak gettext PO, not `.lang`.
- BV2 `.lang` files must keep loading (ADR 0010).
- SDL3 reports the OS language on all three platforms (`SDL_GetPreferredLocales`).

## Decision
Translations are gettext PO files: `msgctxt` = key, `msgid` = English, `msgstr` = translation, named `<ll>[_<CC>].po`, with `en.pot` generated from `en.lang`. The runtime loads `<locale>.po`, then `<locale>.lang`, with per-key English fallback. An empty `languageFile` means "use the OS language", falling back to English.

## Consequences
- Needs a small PO parser in the client and a `tools/` generator and checker.
- A saved `languageFile` overrides detection; there is no picker until an approved **[GUI]** change.
- Rejected: build-time PO → `.lang` conversion (shipped files differ from translators' files); JSON/i18next (weaker translator tooling); keeping `.lang` only (no standard tooling).
