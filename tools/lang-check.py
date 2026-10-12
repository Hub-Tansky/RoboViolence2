#!/usr/bin/env python3
"""Translation checks (run by tools/check-hygiene.py, CI `hygiene` job; ADR 0011).

Fails when en.pot is out of date, a content/languages/*.po file is not valid PO as the game parses it
(game/src/Lang.cpp), has a key en.lang doesn't have, or changes a key's printf conversions. Warns about characters
the font lacks; reports untranslated keys per language.
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

ESCAPES = re.compile(r'\\(?:[n"\\]|x[1-9])')
QUOTED = re.compile(r'"((?:[^"\\]|\\.)*)"$')
KEYWORDS = ("msgctxt", "msgid", "msgstr")
# printf conversions, as formatSpecs() in game/src/Lang.cpp finds them ("%%" is left out)
SPECS = re.compile(r"%%|%[-+ #0-9.hlLqjzt]*[a-zA-Z]?")
# The font's glyphs beyond ASCII: LATIN1 in game/src/Lang.cpp
LATIN1 = set("ÇüéâäàåçêëèïîìÄÅÉæÆôöòûùÿÖÜø£Ø×ª")


def specs(text):
    return [m for m in SPECS.findall(text) if m != "%%"]


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
            elif keyword not in KEYWORDS:
                raise ValueError(f"{path}:{n}: unknown keyword {keyword!r}")
            elif keyword != "msgstr" and "msgstr" in cur:
                flush()
                cur = {}
            if keyword in cur:
                raise ValueError(f"{path}:{n}: {keyword} twice in one entry")
            field = keyword or field
            m = QUOTED.match(rest)
            if not m or field is None or ESCAPES.sub("", m.group(1)).count("\\"):
                raise ValueError(f"{path}:{n}: bad string")
            cur[field] = cur.get(field, "") + m.group(1)
    flush()
    return entries


def main():
    bad = 0
    pot = os.path.join(LANG_DIR, "en.pot")
    with open(pot, encoding="utf-8") as f:
        if f.read() != lang_to_pot.pot_text():
            print("content/languages/en.pot is out of date: run tools/lang-to-pot.py")
            bad += 1
    english = dict(lang_to_pot.read_lang(os.path.join(LANG_DIR, "en.lang")))
    keys = list(english)
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
        for k in sorted(set(entries) & set(keys)):
            if specs(entries[k]) != specs(english[k]):
                print(f"{name}: {k} changes the conversions {specs(english[k])} to {specs(entries[k])}")
                bad += 1
            odd = sorted({c for c in entries[k] if ord(c) > 127 and c not in LATIN1})
            if odd:
                print(f"{name}: warning: {k} uses {''.join(odd)}, which the font lacks: the OS default skips this file")
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
