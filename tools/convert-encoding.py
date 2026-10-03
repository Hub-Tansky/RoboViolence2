#!/usr/bin/env python3
"""Convert tracked text sources to UTF-8 (no BOM) with LF endings.

Per file: pure ASCII keeps its content; valid UTF-8 is NOT re-encoded; anything
else is decoded as CP1252. CRLF/CR become LF and a final newline is ensured.
Usage: tools/convert-encoding.py [--dry-run]
Python 3, stdlib only.
"""
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXTS = (".cpp", ".h", ".c", ".m", ".txt", ".cfg", ".lang", ".md")
SKIP_DIRS = ("maps/", "models/", "sounds/", "textures/", "skins/", "fonts/")


def cp1252(b):
    # CP1252 leaves 0x81 0x8D 0x8F 0x90 0x9D undefined: map them to C1 controls so they are reported.
    return "".join(
        bytes([x]).decode("cp1252") if x not in (0x81, 0x8D, 0x8F, 0x90, 0x9D) else chr(x)
        for x in b
    )


def convert(b):
    if b.startswith(b"\xef\xbb\xbf"):
        b = b[3:]
        kind = "utf-8-bom"
        text = b.decode("utf-8")
    else:
        try:
            b.decode("ascii")
            kind, text = "ascii", b.decode("ascii")
        except UnicodeDecodeError:
            try:
                text = b.decode("utf-8")
                kind = "utf-8"
            except UnicodeDecodeError:
                text = cp1252(b)
                kind = "cp1252"
    # U+FFFD already present in valid UTF-8 input is upstream damage: kept, reported separately.
    bad = [c for c in text if "\x80" <= c <= "\x9f" or (c == "\ufffd" and kind == "cp1252")]
    pre = text.count("\ufffd") if kind == "utf-8" else 0
    text = text.replace("\r\n", "\n").replace("\r", "\n")
    if text and not text.endswith("\n"):
        text += "\n"
    return kind, text.encode("utf-8"), bad, pre


def main():
    dry = "--dry-run" in sys.argv
    files = subprocess.run(["git", "ls-files"], cwd=ROOT, check=True, capture_output=True).stdout.decode().splitlines()
    counts, changed, problems, preexisting = {}, 0, 0, 0
    for p in files:
        if not p.lower().endswith(EXTS) or any(s in p for s in SKIP_DIRS):
            continue
        full = os.path.join(ROOT, p)
        with open(full, "rb") as f:
            old = f.read()
        kind, new, bad, pre = convert(old)
        preexisting += pre
        counts[kind] = counts.get(kind, 0) + 1
        if bad:
            problems += 1
            print(f"PROBLEM {p}: {len(bad)} undecodable/control chars {sorted(set(bad))!r}")
        if new != old:
            changed += 1
            print(f"{'would change' if dry else 'changed'} {p} ({kind})")
            if not dry:
                with open(full, "wb") as f:
                    f.write(new)
    print(f"encodings: {counts}; files changed: {changed}; problems: {problems}; pre-existing U+FFFD kept: {preexisting}")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
