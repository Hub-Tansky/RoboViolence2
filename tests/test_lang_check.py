#!/usr/bin/env python3
"""Phase B step 13: tools/lang-check.py fails on bad translations and passes a good one.

Usage: test_lang_check.py <scratch dir>
"""
import os
import shutil
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CHECK = os.path.join(ROOT, "tools", "lang-check.py")
ENTRY = 'msgctxt "{key}"\nmsgid "x"\nmsgstr "{text}"\n'

# (case, .po content, should fail, text the output must contain)
CASES = [
    ("good", ENTRY.format(key="lang_joinedTheGame", text="%s\\x8 dolaczyl"), False, "ok: translations"),
    ("conversion", ENTRY.format(key="lang_joinedTheGame", text="%i dolaczyl"), True, "changes the conversions"),
    ("unknown key", ENTRY.format(key="lang_nope", text="x"), True, "unknown key lang_nope"),
    ("repeated keyword", ENTRY.format(key="lang_quit", text="a") + 'msgstr "b"\n', True, "twice in one entry"),
    ("tab escape", ENTRY.format(key="lang_quit", text="a\\tb"), True, "bad string"),
    ("raw tab", ENTRY.format(key="lang_quit", text="a\tb"), True, "bad string"),
    ("glyph", ENTRY.format(key="lang_quit", text="Wyjdź"), False, "which the font lacks"),
]


def main():
    lang_dir = os.path.join(sys.argv[1], "lang-check")
    shutil.rmtree(lang_dir, ignore_errors=True)
    os.makedirs(lang_dir)
    for name in ("en.lang", "en.pot"):
        shutil.copy(os.path.join(ROOT, "content", "languages", name), lang_dir)
    failures = 0
    for case, po, should_fail, expect in CASES:
        with open(os.path.join(lang_dir, "pl.po"), "w", encoding="utf-8", newline="\n") as f:
            f.write(po)
        r = subprocess.run([sys.executable, CHECK, lang_dir], capture_output=True, text=True)
        if (r.returncode != 0) != should_fail or expect not in r.stdout:
            print(f"FAIL {case}: exit {r.returncode}\n{r.stdout}")
            failures += 1
    with open(os.path.join(lang_dir, "en.pot"), "a", encoding="utf-8") as f:
        f.write("\n")
    os.remove(os.path.join(lang_dir, "pl.po"))
    r = subprocess.run([sys.executable, CHECK, lang_dir], capture_output=True, text=True)
    if r.returncode == 0 or "en.pot is out of date" not in r.stdout:
        print(f"FAIL stale en.pot: exit {r.returncode}\n{r.stdout}")
        failures += 1
    if not failures:
        print("ok: lang_check")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
