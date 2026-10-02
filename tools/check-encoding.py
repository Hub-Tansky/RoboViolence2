#!/usr/bin/env python3
"""Fail on any tracked text file that is not UTF-8, has a BOM, or contains CR.

Files with a NUL byte are treated as binary and skipped.
Usage: tools/check-encoding.py [--staged]   (--staged: only files added or modified in the index)
Python 3, stdlib only.
"""
import os
import subprocess
import sys

# Removed in step 2 (after its source lists are copied into CMake); not converted.
LEGACY = ("BaboViolent2/Code/BaboViolent2.vcxproj", "BaboViolent2/Code/BaboViolent2.vcxproj.filters")
CRLF_OK = (".ps1", ".bat", ".cmd")  # stored LF, checked out CRLF by .gitattributes

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def main():
    cmd = ["git", "diff", "--cached", "--name-only", "--diff-filter=AM"] if "--staged" in sys.argv else ["git", "ls-files"]
    files = subprocess.run(cmd, cwd=ROOT, check=True, capture_output=True).stdout.decode().splitlines()
    bad = 0
    for p in files:
        full = os.path.join(ROOT, p)
        if p in LEGACY or not os.path.isfile(full) or os.path.islink(full):
            continue
        with open(full, "rb") as f:
            b = f.read()
        if b"\0" in b:
            continue
        problems = []
        if b.startswith(b"\xef\xbb\xbf"):
            problems.append("BOM")
        if b"\r" in b and not p.lower().endswith(CRLF_OK):
            problems.append("CR")
        try:
            b.decode("utf-8")
        except UnicodeDecodeError:
            problems.append("not UTF-8")
        if problems:
            bad += 1
            print(f"FAIL {p}: {', '.join(problems)}")
    if not bad:
        print("ok: encoding")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
