#!/usr/bin/env python3
"""Repository hygiene checks over tracked files (CI `hygiene` job).

1. No path contains a space.
2. No tracked file matches .gitignore (secrets, binaries, OS junk).
3. No file over 5 MB outside content/ and assets-src/.
4. Encoding: tools/check-encoding.py (UTF-8, no BOM, LF).
Python 3, stdlib only.
"""
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MAX_BYTES = 5 * 1024 * 1024
BIG_OK = ("content/", "assets-src/")


def git(*args):
    out = subprocess.run(["git", *args], cwd=ROOT, check=True, capture_output=True).stdout
    return out.decode("utf-8", "surrogateescape").split("\0")[:-1] if "-z" in args else out.decode().splitlines()


def main():
    bad = 0
    tracked = git("ls-files", "-z")
    for p in tracked:
        if " " in p:
            print(f"space in path: {p}")
            bad += 1
    for p in git("ls-files", "-ci", "--exclude-standard"):
        print(f"tracked but ignored by .gitignore: {p}")
        bad += 1
    for p in tracked:
        full = os.path.join(ROOT, p)
        if p.startswith(BIG_OK) or not os.path.isfile(full) or os.path.islink(full):
            continue
        size = os.path.getsize(full)
        if size > MAX_BYTES:
            print(f"over 5 MB outside content/ and assets-src/: {p} ({size} bytes)")
            bad += 1
    r = subprocess.run([sys.executable, os.path.join(ROOT, "tools", "check-encoding.py")], cwd=ROOT)
    if r.returncode != 0:
        bad += 1
    if bad:
        return 1
    print("ok: hygiene")
    return 0


if __name__ == "__main__":
    sys.exit(main())
