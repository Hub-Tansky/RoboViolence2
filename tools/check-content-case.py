#!/usr/bin/env python3
"""Check literal "main/..." paths in the sources against the real file names.

Real names come from content/ (tracked), tools/placeholder-manifest.tsv and tools/original-assets.sha256
(the removed originals). Linux is case-sensitive, so a path that matches a real
file only when case is ignored is an error. Paths that match nothing are only
listed (dynamic names, user files, files from this fork).
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SRC_DIRS = ["game/src", "engine", "masterserver/src"]
LITERAL = re.compile(r'"(main/[^"%\\]*)"')
ORIGINAL_PREFIX = "BaboViolent2/Content/"


def real_names():
    names = set()
    for f in (ROOT / "content").rglob("*"):
        if f.is_file() and not f.name.endswith(".log"):
            names.add("main/" + f.relative_to(ROOT / "content").as_posix())
    for line in (ROOT / "tools/original-assets.sha256").read_text().splitlines():
        path = line.split("  ", 1)[-1]
        if path.startswith(ORIGINAL_PREFIX + "main/"):
            names.add(path[len(ORIGINAL_PREFIX):])
    for line in (ROOT / "tools/placeholder-manifest.tsv").read_text(encoding="utf-8").splitlines():
        if line and not line.startswith("#"):
            folder, name, _ = line.split("\t")
            names.add(f"{folder}/{name}")
    return names


def main():
    names = real_names()
    folded = {}
    for n in names:
        folded.setdefault(n.lower(), set()).add(n)
    dirs = {n.rsplit("/", 1)[0] for n in names}
    errors, unknown = [], []
    for d in SRC_DIRS:
        for f in sorted((ROOT / d).rglob("*")):
            if f.suffix not in {".cpp", ".h", ".c", ".cfg"}:
                continue
            for no, line in enumerate(f.read_text(errors="replace").splitlines(), 1):
                for lit in LITERAL.findall(line):
                    if lit in names or lit.rstrip("/") in dirs:
                        continue
                    where = f"{f.relative_to(ROOT)}:{no}: {lit}"
                    match = folded.get(lit.lower())
                    if match:
                        errors.append(f"{where} (real name: {', '.join(sorted(match))})")
                    elif not lit.endswith("/") and "." in lit.rsplit("/", 1)[-1]:
                        unknown.append(where)
    for u in unknown:
        print("not found:", u)
    for e in errors:
        print("case:", e)
    if errors:
        return 1
    print("ok: no case mismatches in literal main/ paths")
    return 0


if __name__ == "__main__":
    sys.exit(main())
