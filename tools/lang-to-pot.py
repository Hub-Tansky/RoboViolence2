#!/usr/bin/env python3
"""Generate content/languages/en.pot (the translators' template) from en.lang (ADR 0011).

msgctxt is the .lang key, msgid the English text. The .lang escapes (\\n, \\x1-\\x9) stay as they are.
Usage: tools/lang-to-pot.py   (tools/lang-check.py fails when en.pot is out of date)
Python 3, stdlib only.
"""
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LANG_DIR = os.path.join(ROOT, "content", "languages")
HEADER = (
    "# Robo Violence 2 strings: copy to <ll>.po or <ll>_<CC>.po (content/languages/README.md).\n"
    "# Generated from en.lang by tools/lang-to-pot.py; don't edit.\n"
    'msgid ""\nmsgstr ""\n"Content-Type: text/plain; charset=UTF-8\\n"\n'
)


def read_lang(path):
    """[(key, text)] of a .lang file up to END; tabs are dropped as the game does."""
    entries = []
    with open(path, encoding="utf-8") as f:
        for line in f:
            parts = line.split(None, 1)
            if not parts:
                continue
            if parts[0] == "END":
                return entries
            text = parts[1].rstrip("\n").replace("\t", "") if len(parts) > 1 else ""
            entries.append((parts[0], text))
    raise ValueError(f"{path}: no END line")


def pot_text(lang_dir=LANG_DIR):
    out = [HEADER]
    for key, text in read_lang(os.path.join(lang_dir, "en.lang")):
        out.append(f'\nmsgctxt "{key}"\nmsgid "{text.replace(chr(34), chr(92) + chr(34))}"\nmsgstr ""\n')
    return "".join(out)


def main():
    with open(os.path.join(LANG_DIR, "en.pot"), "w", encoding="utf-8", newline="\n") as f:
        f.write(pot_text())
    return 0


if __name__ == "__main__":
    sys.exit(main())
