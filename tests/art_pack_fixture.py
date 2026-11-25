#!/usr/bin/env python3
"""Fixture art pack (ROBO-264): stands in for the real art pack (ROBO-260), which is never in this repository.

Usage: art_pack_fixture.py <base manifest> <out dir>
Writes <out dir>/main/pack.tsv and one flat-colour PNG per texture id of the base pack, the size of the base texture and
with alpha where it is 32-bit, under licence LicenseRef-RV2-Art. Stacked over the base pack (BV2_DATA_DIR, ADR 0069), it
makes the game draw PNGs from a pack with its own manifest, as the art will. It has no font: the real pack carries fonts/babo as a TTF (Teko, ROBO-271), and text_fit tests that bake.
It also holds one full-colour skin with its team mask, skins/skin10 (ROBO-258): grey above, dark below, a red equator
band the mask covers and a red stripe above it that the mask leaves out, so a frame shows team colour lands on the mask only.
Then it runs tools/check-pack.py --base on it; exit 0 when the pack passes.
"""
import os
import re
import struct
import subprocess
import sys
import zlib


def png_rows(path, width, rows, colour_type):
    """An 8-bit PNG from rows of pixel bytes, top row first; colour type 0 grey, 2 RGB, 6 RGBA"""
    def chunk(kind, body):
        return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body))
    pixels = zlib.compress(b"".join(b"\0" + row for row in rows), 9)
    header = struct.pack(">IIBBBBB", width, len(rows), 8, colour_type, 0, 0, 0)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", header) + chunk(b"IDAT", pixels) + chunk(b"IEND", b""))


def png(path, width, height, rgba, alpha):
    """A flat-colour 8-bit PNG, RGBA when alpha else RGB"""
    pixel = bytes(rgba if alpha else rgba[:3])
    png_rows(path, width, [pixel * width] * height, 6 if alpha else 2)


SKIN_W, SKIN_H = 512, 256
PAINT_RED = bytes([232, 52, 58])  # the red the art paints team parts in (game/src/Skins.h PAINT_RED)


def skin_row(y):
    """Kind of row y of the fixture skin: band (masked red), stripe (red, not masked), top or bottom"""
    if SKIN_H * 7 // 16 <= y < SKIN_H * 9 // 16:
        return "band"
    if SKIN_H * 5 // 16 <= y < SKIN_H * 6 // 16:
        return "stripe"
    return "top" if y < SKIN_H // 2 else "bottom"


def full_colour_skin(out, name):
    """skins/<name> and skins/<name>_mask (ROBO-258); rows only, so the ball looks the same however it spins"""
    fill = {"band": PAINT_RED, "stripe": PAINT_RED, "top": bytes([170, 170, 180]), "bottom": bytes([60, 60, 70])}
    png_rows(os.path.join(out, "main", "skins", name + ".png"), SKIN_W,
             [fill[skin_row(y)] * SKIN_W for y in range(SKIN_H)], 2)
    png_rows(os.path.join(out, "main", "skins", name + "_mask.png"), SKIN_W,
             [bytes([255 if skin_row(y) == "band" else 0]) * SKIN_W for y in range(SKIN_H)], 0)
    return [(f"skins/{name}", f"skins/{name}.png", f"PNG {SKIN_W}×{SKIN_H}×24"),
            (f"skins/{name}_mask", f"skins/{name}_mask.png", f"PNG {SKIN_W}×{SKIN_H}×8")]


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
        # BV2 skins stay with the base pack: the art replaces them with full-colour skins (below), not by id
        if line.startswith("#") or not size or cols[0].startswith("skins/legacy/"):
            continue
        file = os.path.splitext(cols[1])[0] + ".png"
        png(os.path.join(out, "main", file), int(size[1]), int(size[2]), colour(cols[0]), size[3] == "32")
        rows.append("\t".join([cols[0], file, "texture", f"PNG {size[1]}×{size[2]}×{size[3]}", "RV2 tests",
                               "tests/art_pack_fixture.py", "LicenseRef-RV2-Art", "generated fixture"]))
    for skin_id, file, size in full_colour_skin(out, "skin10"):
        rows.append("\t".join([skin_id, file, "texture", size, "RV2 tests", "tests/art_pack_fixture.py", "LicenseRef-RV2-Art",
                               "generated fixture"]))
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
