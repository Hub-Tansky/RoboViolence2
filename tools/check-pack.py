#!/usr/bin/env python3
"""Check an asset pack against its manifest (ADR 0065).

Usage: check-pack.py <pack dir> [--manifest FILE] [--base BASE_MANIFEST]   (default manifest: <pack dir>/pack.tsv)
Fails when the header lacks a column, a row has an empty id, file, type, author or licence, an unknown type,
a duplicate id (case-insensitive), an id or file that leaves the pack root (absolute, `..`, backslash), or a file missing
from the pack. A row whose source is `missing` may lack its file;
it is listed as a NOTE. A row with licence `LicenseRef-RV2-Art` (the art pack, ROBO-260) also needs its tool.
With --base (an art pack over the base pack, ROBO-264): a texture must be a PNG or TGA with power-of-two sides, with alpha
when its base row is 32-bit; the type must match the base row; an id differing from a base id only in case fails; ids the
base lacks are listed as NOTEs. Python 3, stdlib only.
"""
import os
import struct
import sys

COLUMNS = ["id", "file", "type", "format", "author", "tool", "licence", "source"]
REQUIRED = ["id", "file", "type", "author", "licence"]
TYPES = {"texture", "sound", "model", "font"}
ART_LICENCE = "LicenseRef-RV2-Art"


def image_info(path):
    """(width, height, has alpha) of a PNG or TGA file, or None when it is neither"""
    with open(path, "rb") as f:
        data = f.read()
    if data.startswith(b"\x89PNG\r\n\x1a\n") and data[12:16] == b"IHDR":
        width, height, _depth, colour = struct.unpack(">IIBB", data[16:26])
        return width, height, colour in (4, 6) or b"tRNS" in data
    if path.lower().endswith(".tga") and len(data) >= 18:
        width, height, bits = struct.unpack("<HHB", data[12:17])
        return width, height, bits == 32
    return None


def base_rows(manifest):
    """id -> (type, format) of a manifest's rows"""
    rows = {}
    for line in open(manifest, encoding="utf-8"):
        cols = line.rstrip("\n").split("\t")
        if not line.startswith("#") and len(cols) >= 4 and cols[0] != "id":
            rows[cols[0]] = (cols[2], cols[3])
    return rows


def check_against_base(pack, row, base, by_lower):
    """Problems and notes of one art pack row against the base pack (ROBO-264)"""
    problems, notes = [], []
    if row["id"] not in base:
        if row["id"].lower() in by_lower:
            return [f"{row['id']}: differs from base id {by_lower[row['id'].lower()]} only in case"], notes
        notes.append(f"{row['id']}: new id, not in the base pack")
    elif row["type"] != base[row["id"]][0]:
        problems.append(f"{row['id']}: type {row['type']}, base pack has {base[row['id']][0]}")
    path = os.path.join(pack, row["file"])
    if row["type"] != "texture" or not os.path.isfile(path):
        return problems, notes
    info = image_info(path)
    if info is None:
        return problems + [f"{row['id']}: {row['file']} is not a PNG or TGA"], notes
    width, height, alpha = info
    if width & (width - 1) or height & (height - 1) or not width or not height:
        problems.append(f"{row['id']}: {width}x{height} is not a power of two")
    if row["id"] in base and base[row["id"]][1].endswith("32") and not alpha:
        problems.append(f"{row['id']}: no alpha, the base texture is 32-bit")
    return problems, notes


def outside_root(path):
    """True when a manifest path could leave the pack root: absolute, a drive, a backslash or a `..` part."""
    return path.startswith("/") or ":" in path or "\\" in path or ".." in path.split("/")


def check(pack, manifest, base=None):
    problems, notes, ids, header = [], [], {}, None
    by_lower = {k.lower(): k for k in base or {}}
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
        if row["licence"] == ART_LICENCE and not row["tool"].strip():
            problems.append(f"{manifest}:{n}: {row['id']}: empty tool ({ART_LICENCE} rows need author, tool and licence)")
        if row["type"] and row["type"] not in TYPES:
            problems.append(f"{manifest}:{n}: {row['id']}: unknown type '{row['type']}'")
        bad = [c for c in ("id", "file") if outside_root(row[c])]
        if bad:
            problems.append(f"{manifest}:{n}: {row['id']}: {' and '.join(bad)} must stay inside the pack root")
            continue
        key = row["id"].lower()
        if key in ids:
            problems.append(f"{manifest}:{n}: {row['id']}: duplicate id (line {ids[key]})")
        ids.setdefault(key, n)
        if row["file"] and not os.path.isfile(os.path.join(pack, row["file"])):
            if row["source"] == "missing":
                notes.append(f"{row['id']}: no file yet ({row['file']})")
            else:
                problems.append(f"{manifest}:{n}: {row['id']}: {row['file']} is not in the pack")
        if base is not None:
            more, new = check_against_base(pack, row, base, by_lower)
            problems += [f"{manifest}:{n}: {p}" for p in more]
            notes += new
    if header is None:
        problems.append(f"{manifest}: no header row")
    return problems, notes


def main():
    argv = sys.argv[1:]
    base = None
    if "--base" in argv:
        i = argv.index("--base")
        if i + 1 >= len(argv):
            print(__doc__)
            return 2
        base = base_rows(argv.pop(i + 1))
        argv.pop(i)
    args = [a for a in argv if not a.startswith("--")]
    if len(args) < 1 or ("--manifest" in argv and len(args) < 2):
        print(__doc__)
        return 2
    pack = args[0]
    manifest = args[1] if "--manifest" in argv else os.path.join(pack, "pack.tsv")
    problems, notes = check(pack, manifest, base)
    for note in notes:
        print("NOTE", note)
    for p in problems:
        print("FAIL", p)
    if not problems:
        print(f"ok: pack {pack} matches {manifest}")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
