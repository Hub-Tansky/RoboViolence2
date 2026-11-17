#!/usr/bin/env python3
"""Check an asset pack against its manifest (ADR 0064).

Usage: check-pack.py <pack dir> [--manifest FILE]   (default manifest: <pack dir>/pack.tsv)
Fails when the header lacks a column, a row has an empty id, file, type, author or licence, an unknown type,
a duplicate id (case-insensitive), or a file missing from the pack. A row whose source is `missing` may lack its file;
it is listed as a NOTE. Python 3, stdlib only.
"""
import os
import sys

COLUMNS = ["id", "file", "type", "format", "author", "tool", "licence", "source"]
REQUIRED = ["id", "file", "type", "author", "licence"]
TYPES = {"texture", "sound", "model", "font"}


def check(pack, manifest):
    problems, notes, ids, header = [], [], {}, None
    for n, line in enumerate(open(manifest, encoding="utf-8"), 1):
        if line.startswith("#") or not line.strip():
            continue
        cols = line.rstrip("\n").split("\t")
        if header is None:
            header = cols
            missing = [c for c in COLUMNS if c not in header]
            if missing:
                return [f"{manifest}:{n}: header lacks {', '.join(missing)}"], notes
            continue
        if len(cols) != len(header):
            problems.append(f"{manifest}:{n}: {len(cols)} columns, header has {len(header)}")
            continue
        row = dict(zip(header, cols))
        empty = [c for c in REQUIRED if not row[c].strip()]
        if empty:
            problems.append(f"{manifest}:{n}: {row['id'] or '?'}: empty {', '.join(empty)}")
        if row["type"] and row["type"] not in TYPES:
            problems.append(f"{manifest}:{n}: {row['id']}: unknown type '{row['type']}'")
        key = row["id"].lower()
        if key in ids:
            problems.append(f"{manifest}:{n}: {row['id']}: duplicate id (line {ids[key]})")
        ids.setdefault(key, n)
        if row["file"] and not os.path.isfile(os.path.join(pack, row["file"])):
            if row["source"] == "missing":
                notes.append(f"{row['id']}: no file yet ({row['file']})")
            else:
                problems.append(f"{manifest}:{n}: {row['id']}: {row['file']} is not in the pack")
    if header is None:
        problems.append(f"{manifest}: no header row")
    return problems, notes


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    if len(args) < 1 or ("--manifest" in sys.argv and len(args) < 2):
        print(__doc__)
        return 2
    pack = args[0]
    manifest = args[1] if "--manifest" in sys.argv else os.path.join(pack, "pack.tsv")
    problems, notes = check(pack, manifest)
    for note in notes:
        print("NOTE", note)
    for p in problems:
        print("FAIL", p)
    if not problems:
        print(f"ok: pack {pack} matches {manifest}")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
