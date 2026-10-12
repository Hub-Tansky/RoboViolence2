#!/usr/bin/env python3
"""Translation checks (run by tools/check-hygiene.py, CI `hygiene` job; ADR 0011).

Fails when en.pot is out of date, a content/languages/*.po file is not valid PO as the game parses it
(game/src/Lang.cpp), or a PO has a key en.lang doesn't have. Reports untranslated keys per language.
Python 3, stdlib only.
"""
import glob
import importlib.util
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LANG_DIR = os.path.join(ROOT, "content", "languages")
_spec = importlib.util.spec_from_file_location("lang_to_pot", os.path.join(ROOT, "tools", "lang-to-pot.py"))
lang_to_pot = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(lang_to_pot)

ESCAPES = re.compile(r'\\(?:[nt"\\]|x[1-9])')
QUOTED = re.compile(r'"((?:[^"\\]|\\.)*)"$')


def read_po(path):
    """{msgctxt: msgstr} of the translated entries; raises ValueError where the game's parser would fail."""
    entries, cur, field = {}, {}, None

    def flush():
        if cur.get("msgctxt") and cur.get("msgstr"):
            entries[cur["msgctxt"]] = cur["msgstr"]

    with open(path, encoding="utf-8") as f:
        for n, line in enumerate(f, 1):
            line = line.rstrip("\r\n").lstrip(" \t")
            if not line or line.startswith("#"):
                continue
            keyword, _, rest = line.partition(" ")
            if line.startswith('"'):
                keyword, rest = None, line
            elif keyword in ("msgctxt", "msgid"):
                if keyword == "msgctxt" or "msgstr" in cur:
                    flush()
                    cur = {}
                field = keyword
            elif keyword == "msgstr":
                field = keyword
            else:
                raise ValueError(f"{path}:{n}: unknown keyword {keyword!r}")
            m = QUOTED.match(rest)
            if not m or field is None or ESCAPES.sub("", m.group(1)).count("\\"):
                raise ValueError(f"{path}:{n}: bad string")
            cur[field] = cur.get(field, "") + m.group(1) if keyword is None else m.group(1)
    flush()
    return entries


def main():
    bad = 0
    pot = os.path.join(LANG_DIR, "en.pot")
    with open(pot, encoding="utf-8") as f:
        if f.read() != lang_to_pot.pot_text():
            print("content/languages/en.pot is out of date: run tools/lang-to-pot.py")
            bad += 1
    keys = [k for k, _ in lang_to_pot.read_lang(os.path.join(LANG_DIR, "en.lang"))]
    for path in sorted(glob.glob(os.path.join(LANG_DIR, "*.po"))):
        name = os.path.basename(path)
        try:
            entries = read_po(path)
        except ValueError as e:
            print(e)
            bad += 1
            continue
        unknown = sorted(set(entries) - set(keys))
        for k in unknown:
            print(f"{name}: unknown key {k}")
        bad += bool(unknown)
        missing = [k for k in keys if k not in entries]
        if missing:
            print(f"{name}: {len(missing)} of {len(keys)} keys untranslated (English shown): {', '.join(missing[:5])}"
                  + (", ..." if len(missing) > 5 else ""))
    if bad:
        return 1
    print("ok: translations")
    return 0


if __name__ == "__main__":
    sys.exit(main())
