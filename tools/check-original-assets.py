#!/usr/bin/env python3
"""Fail if a tracked file matches the SHA-256 of a removed original asset.

The hash list is tools/original-assets.sha256.
Usage: tools/check-original-assets.py [--staged]   (--staged: check the index only)
Python 3, stdlib only.
"""
import hashlib
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHA = "tools/original-assets.sha256"


def main():
    staged = "--staged" in sys.argv
    hashes, paths = {}, []
    with open(os.path.join(ROOT, SHA)) as f:
        for line in f:
            h, p = line.rstrip("\n").split("  ", 1)
            hashes[h] = p
            paths.append(p)
    errors = []

    cmd = ["git", "diff", "--cached", "--name-only", "--diff-filter=AM"] if staged else ["git", "ls-files"]
    files = subprocess.run(cmd, cwd=ROOT, check=True, capture_output=True).stdout.decode().splitlines()
    for p in files:
        full = os.path.join(ROOT, p)
        if not os.path.isfile(full) or os.path.islink(full):
            continue
        with open(full, "rb") as f:
            h = hashlib.sha256(f.read()).hexdigest()
        if h in hashes:
            errors.append(f"{p}: matches removed original asset {hashes[h]}")

    for e in errors:
        print("FAIL", e)
    if not errors:
        print(f"ok: no tracked file matches {len(hashes)} original-asset hashes")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
