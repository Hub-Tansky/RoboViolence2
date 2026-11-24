#!/usr/bin/env python3
"""Fixture art pack (ROBO-264): stands in for the real art pack (ROBO-260), which is never in this repository.

Usage: art_pack_fixture.py <base manifest> <out dir>
Writes <out dir>/main/pack.tsv and one flat-colour PNG per texture id of the base pack, the size of the base texture and
with alpha where it is 32-bit, under licence LicenseRef-RV2-Art. Stacked over the base pack (BV2_DATA_DIR, ADR 0069), it
makes the game draw PNGs from a pack with its own manifest, as the art will. Fonts stay with the base pack (ROBO-265).
Then it runs tools/check-pack.py --base on it; exit 0 when the pack passes.
"""
import os
import re
import struct
import subprocess
import sys
import zlib


def png(path, width, height, rgba, alpha):
    """A flat-colour 8-bit PNG, RGBA when alpha else RGB"""
    def chunk(kind, body):
        return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body))
    pixel = bytes(rgba if alpha else rgba[:3])
    pixels = zlib.compress((b"\0" + pixel * width) * height, 9)
    header = struct.pack(">IIBBBBB", width, height, 8, 6 if alpha else 2, 0, 0, 0)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", header) + chunk(b"IDAT", pixels) + chunk(b"IEND", b""))


def colour(texture_id):
    """A stable, saturated colour per id, so a frame shows which textures came from the fixture"""
    h = zlib.crc32(texture_id.encode())
    return [64 + (h & 0xBF), 64 + ((h >> 8) & 0xBF), 64 + ((h >> 16) & 0xBF), 255]


def make(base, out):
    rows = ["# Fixture art pack (tests/art_pack_fixture.py, ROBO-264): flat colours, no real art",
            "id\tfile\ttype\tformat\tauthor\ttool\tlicence\tsource"]
    for line in open(base, encoding="utf-8"):
        cols = line.rstrip("\n").split("\t")
        size = re.match(r"TGA (\d+)×(\d+)×(24|32)$", cols[3]) if len(cols) >= 4 and cols[2] == "texture" else None
        if line.startswith("#") or not size:
            continue
        file = os.path.splitext(cols[1])[0] + ".png"
        png(os.path.join(out, "main", file), int(size[1]), int(size[2]), colour(cols[0]), size[3] == "32")
        rows.append("\t".join([cols[0], file, "texture", f"PNG {size[1]}×{size[2]}×{size[3]}", "RV2 tests",
                               "tests/art_pack_fixture.py", "LicenseRef-RV2-Art", "generated fixture"]))
    with open(os.path.join(out, "main", "pack.tsv"), "w", encoding="utf-8") as f:
        f.write("\n".join(rows) + "\n")
    return len(rows) - 2


def main():
    base, out = sys.argv[1], sys.argv[2]
    count = make(base, out)
    check = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "tools", "check-pack.py")
    r = subprocess.run([sys.executable, check, os.path.join(out, "main"), "--base", base])
    print(f"{'ok' if r.returncode == 0 else 'FAIL'}: fixture art pack with {count} textures in {out}")
    return r.returncode


if __name__ == "__main__":
    sys.exit(main())
