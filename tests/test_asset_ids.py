#!/usr/bin/env python3
"""ROBO-232: every literal asset id the game asks for (bv2::assetPath("...") and the ids passed to new Weapon(...)) is a
row of content/base-pack.tsv, so asking for an id the manifest lacks fails here, not at runtime.

Usage: test_asset_ids.py [<repo root>]
"""
import os
import re
import sys

ROOT = sys.argv[1] if len(sys.argv) > 1 else os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ASSET_CALL = re.compile(r'assetPath\(\s*"([^"%]+)"\s*\)')
WEAPON_IDS = re.compile(r'new Weapon\(\s*"([^"]*)"\s*,\s*"([^"]*)"')


def manifest_ids(path):
    ids, header = set(), None
    for line in open(path, encoding="utf-8"):
        if line.startswith("#") or not line.strip():
            continue
        cols = line.rstrip("\n").split("\t")
        if header is None:
            header = cols
            continue
        row = dict(zip(header, cols))
        for col in ("id", "file"):
            if re.search(r'[;"\\]', row[col]):  # game/CMakeLists.txt compiles these into BasePackIds.cpp
                print(f"FAIL: content/base-pack.tsv {col} {row[col]!r} has ; \" or \\")
                ids.add(None)
        ids.add(row["id"])
    return ids


def built_sources(root):
    """game/src/*.cpp that game/CMakeLists.txt builds (the old menu files are not built)"""
    cmake = open(os.path.join(root, "game", "CMakeLists.txt"), encoding="utf-8").read()
    return sorted(set(re.findall(r"\b(src/[\w/]+\.(?:cpp|h))\b", cmake)))


def asked_ids(text):
    for m in ASSET_CALL.finditer(text):
        yield m.group(1)
    for m in WEAPON_IDS.finditer(text):
        yield from (i for i in m.groups() if i)


def main():
    ids = manifest_ids(os.path.join(ROOT, "content", "base-pack.tsv"))
    bad, seen = ([] if None not in ids else ["content/base-pack.tsv: bad character in an id or file"]), 0
    for rel in built_sources(ROOT) + ["src/Weapon.h"]:
        path = os.path.join(ROOT, "game", rel)
        if not os.path.exists(path):
            continue
        text = open(path, encoding="utf-8", errors="replace").read()
        # commented-out loads ask for nothing; block comments keep their newlines so line numbers still match
        text = re.sub(r"/\*.*?\*/", lambda m: "\n" * m.group(0).count("\n"), text, flags=re.S)
        text = re.sub(r"//[^\n]*", "", text)
        for asset_id in asked_ids(text):
            seen += 1
            if asset_id not in ids:
                line = text[: text.find('"' + asset_id + '"')].count("\n") + 1
                bad.append(f"game/{rel}:{line}: asset id \"{asset_id}\" is not in content/base-pack.tsv")
    for b in bad:
        print("FAIL:", b)
    if seen == 0:
        print("FAIL: no asset ids found; did the call pattern change?")
        return 1
    if not bad:
        print(f"ok: {seen} literal asset ids, all in content/base-pack.tsv")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
