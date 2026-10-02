#!/usr/bin/env python3
"""Fail if a tracked file matches the SHA-256 of a removed original asset.

Also checks docs/assets/ASSET-INVENTORY.md against original-assets.sha256:
one row per hash, and no `required` row with Purpose `TODO`.
Usage: tools/check-original-assets.py [--staged]   (--staged: check the index only)
Python 3, stdlib only.
"""
import hashlib
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHA = "docs/assets/original-assets.sha256"
INV = "docs/assets/ASSET-INVENTORY.md"


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

    if os.path.exists(os.path.join(ROOT, INV)):
        rows = []
        with open(os.path.join(ROOT, INV), encoding="utf-8") as f:
            for line in f:
                cells = [c.strip() for c in line.strip().strip("|").split("|")]
                if len(cells) == 6 and cells[0].startswith("`") and cells[4] in ("required", "content", "source", "drop"):
                    rows.append(cells)
        if len(rows) != len(paths):
            errors.append(f"{INV}: {len(rows)} rows, {SHA}: {len(paths)} hashes")
        for c in rows:
            if c[4] == "required" and c[3] == "TODO":
                errors.append(f"{INV}: required row {c[0]} has Purpose TODO")

    for e in errors:
        print("FAIL", e)
    if not errors:
        print(f"ok: no tracked file matches {len(hashes)} original-asset hashes")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
