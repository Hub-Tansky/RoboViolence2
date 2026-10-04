#!/usr/bin/env python3
"""Write placeholder game data for development and CI: no original assets.

Reads tools/placeholder-manifest.tsv (the files the game loads at startup) and writes, under <out>/:
  - a solid-colour TGA (same size and depth) for every required texture,
  - a font atlas for main/fonts/babo.tga (5×7 bitmap glyphs, ASCII 33–159),
  - a short silent WAV (same format) for every required sound,
  - an empty but valid .DKO model for every required model,
  - four tiny maps (DM, TDM, CTF, SND) in .bvm v20202.
Music (.ogg) is not generated: only the client plays it, and no encoder is available here.
Usage: gen-placeholder-content.py <out-dir> [--manifest FILE]
Python 3, stdlib only.
"""
import os
import re
import struct
import sys
import zlib

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEFAULT_MANIFEST = os.path.join(ROOT, "tools", "placeholder-manifest.tsv")


def colour(name):
    h = zlib.crc32(name.encode()) & 0xFFFFFF
    return 64 + (h & 0x7F), 64 + ((h >> 8) & 0x7F), 64 + ((h >> 16) & 0x7F)


def tga(name, w, h, bpp):
    r, g, b = colour(name)
    px = bytes([b, g, r]) if bpp == 24 else bytes([b, g, r, 255])
    header = struct.pack("<BBBHHBHHHHBB", 0, 0, 2, 0, 0, 0, 0, 0, w, h, bpp, 0 if bpp == 24 else 8)
    return header + px * (w * h)


# 5×7 glyphs for ASCII 33..126: 7 rows of 5 bits (MSB = left), 2 hex digits per row
FONT_5X7 = (
    "04040404040004 0a0a0a00000000 0a0a1f0a1f0a0a 040f140e051e04 18190204081303 0c12140815120d "
    "04040800000000 02040808080402 08040202020408 0004150e150400 0004041f040400 000000000c0408 "
    "0000001f000000 00000000000c0c 00010204081000 0e11131519110e 040c040404040e 0e11010204081f "
    "1f02040201110e 02060a121f0202 1f101e0101110e 0608101e11110e 1f010204080808 0e11110e11110e "
    "0e11110f01020c 000c0c000c0c00 000c0c000c0408 02040810080402 00001f001f0000 08040201020408 "
    "0e110102040004 0e11010d15150e 0e1111111f1111 1e11111e11111e 0e11101010110e 1c12111111121c "
    "1f10101e10101f 1f10101e101010 0e11101711110f 1111111f111111 0e04040404040e 0702020202120c "
    "11121418141211 1010101010101f 111b1515111111 11111915131111 0e11111111110e 1e11111e101010 "
    "0e11111115120d 1e11111e141211 0f10100e01011e 1f040404040404 1111111111110e 11111111110a04 "
    "1111111515150a 11110a040a1111 1111110a040404 1f01020408101f 0e08080808080e 00100804020100 "
    "0e02020202020e 040a1100000000 0000000000001f 08040200000000 00000e010f110f 1010161911111e "
    "00000e1010110e 01010d1311110f 00000e111f100e 0609081c080808 000f11110f010e 10101619111111 "
    "04000c0404040e 0200060202120c 10101214181412 0c04040404040e 00001a15151111 00001619111111 "
    "00000e1111110e 00001e111e1010 00000d130f0101 00001619101010 00000e100e011e 08081c08080906 "
    "0000111111130d 00001111110a04 0000111115150a 0000110a040a11 000011110f010e 00001f0204081f "
    "02040408040402 04040404040404 08040402040408 00000815020000 "
).split()
# Codes 128..135 are accented letters (GameVar::loadLanguage remaps them); draw the base letter
FONT_ACCENTS = {128: "C", 129: "u", 130: "e", 131: "a", 132: "a", 133: "a", 134: "a", 135: "c"}


def font_atlas(size=512, cell_h=64, scale=5, gap=3, top=12):
    """512×512 RGBA atlas in the layout CFont::loadTGAFile scans (engine/zeven/src/CFont.cpp).

    The loader reads characters 33..159 in order, row by row (cell_h px high), each one a run of
    columns with alpha > 0 ended by a fully transparent column. An alpha-1 underline joins each
    glyph's columns (so '"' stays one character) and is invisible on screen.
    """
    alpha = bytearray(size * size)  # row 0 = top of the atlas
    x, y = 0, 0
    w = 5 * scale
    for c in range(33, 160):
        if x + w > size:
            x, y = 0, y + cell_h
        if c <= 126:
            rows = [int(FONT_5X7[c - 33][i:i + 2], 16) for i in range(0, 14, 2)]
        elif c in FONT_ACCENTS:
            rows = [int(FONT_5X7[ord(FONT_ACCENTS[c]) - 33][i:i + 2], 16) for i in range(0, 14, 2)]
        else:
            rows = [0x1F, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1F]  # box for unused codes
        for r, bits in enumerate(rows):
            for col in range(5):
                if bits & (0x10 >> col):
                    for dy in range(scale):
                        start = (y + top + r * scale + dy) * size + x + col * scale
                        alpha[start:start + scale] = b"\xff" * scale
        under = (y + top + 7 * scale) * size + x
        alpha[under:under + w] = b"\x01" * w
        x += w + gap
    # The TGA loader (engine/zeven/src/dkt.cpp) ignores the origin bit: file row 0 is the atlas bottom
    pixels = bytearray()
    for row in range(size - 1, -1, -1):
        for a in alpha[row * size:(row + 1) * size]:
            pixels += bytes((255, 255, 255, a))
    return struct.pack("<BBBHHBHHHHBB", 0, 0, 2, 0, 0, 0, 0, 0, size, size, 32, 8) + bytes(pixels)


def wav(ch, rate, bits):
    frames = max(1, rate // 20)
    silence = (b"\x80" if bits == 8 else b"\x00" * (bits // 8)) * (frames * ch)
    fmt = struct.pack("<HHIIHH", 1, ch, rate, rate * ch * bits // 8, ch * bits // 8, bits)
    body = b"WAVE" + b"fmt " + struct.pack("<I", len(fmt)) + fmt + b"data" + struct.pack("<I", len(silence)) + silence
    return b"RIFF" + struct.pack("<I", len(body)) + body


def dko():
    # version chunk (0x0000, short 2), time info (0x0001, 3 shorts), end (0x0900)
    return struct.pack("<hh", 0x0000, 2) + struct.pack("<hhhh", 0x0001, 0, 0, 1) + struct.pack("<h", 0x0900)


def vec(x, y, z):
    return struct.pack("<fff", x, y, z)


def bvm(name, size=16):
    """Walled arena with spawns for every mode (format: docs/analysis/02_DATA_STRUCTURES.md 4.3)."""
    out = struct.pack("<I", 20202)
    out += name.encode()[:24].ljust(25, b"\0")  # author field
    out += struct.pack("<hh", 0, 0)  # theme, weather (int16)
    out += struct.pack("<hh", size, size)
    for y in range(size):
        for x in range(size):
            wall = x in (0, size - 1) or y in (0, size - 1)
            out += bytes([(0 if wall else 128) | (3 if wall else 0), 0])
    c = size / 2.0
    dm = [vec(3, 3, 0), vec(size - 3, 3, 0), vec(3, size - 3, 0), vec(size - 3, size - 3, 0)]
    out += struct.pack("<h", len(dm)) + b"".join(dm)
    for gt in range(4):  # GAME_TYPE_COUNT = 4: DM, TDM, CTF, SND
        out += struct.pack("<h", gt)
        if gt == 2:
            out += vec(3, c, 0) + vec(size - 3, c, 0)
        elif gt == 3:
            out += vec(c, 3, 0) + vec(c, size - 3, 0)
            blue = [vec(3, c, 0), vec(3, c + 1, 0)]
            red = [vec(size - 3, c, 0), vec(size - 3, c + 1, 0)]
            out += struct.pack("<h", len(blue)) + b"".join(blue)
            out += struct.pack("<h", len(red)) + b"".join(red)
    return out


def required_rows(manifest):
    for line in open(manifest, encoding="utf-8"):
        if line.startswith("#") or not line.strip():
            continue
        folder, name, fmt = line.rstrip("\n").split("\t")
        yield folder, name, fmt


def write(path, data):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as f:
        f.write(data)


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    if not args:
        print(__doc__)
        return 2
    out = args[0]
    manifest = DEFAULT_MANIFEST
    if "--manifest" in sys.argv:
        manifest = sys.argv[sys.argv.index("--manifest") + 1]
    # content/ holds only languages and scripts; main/ in the runtime dir is the game's data root
    counts, skipped = {}, []
    for folder, rel, typ in required_rows(manifest):
        # folder is "main/<kind>"; the runtime main/ dir is `out` itself
        dest = os.path.join(out, folder.split("/", 1)[1], rel)
        ext = os.path.splitext(rel)[1].lower()
        if ext == ".tga":
            m = re.match(r"TGA (\d+)×(\d+)×(\d+)", typ)
            if folder == "main/fonts" and (int(m[1]), int(m[2]), int(m[3])) == (512, 512, 32):
                data = font_atlas()
            else:
                data = tga(rel, int(m[1]), int(m[2]), int(m[3]))
        elif ext == ".wav":
            m = re.match(r"WAV (\d+)ch (\d+) Hz (\d+)-bit", typ)
            data = wav(int(m[1]), int(m[2]), int(m[3]))
        elif ext == ".dko":
            data = dko()
        else:
            skipped.append(rel)
            continue
        write(dest, data)
        counts[ext] = counts.get(ext, 0) + 1
    for name in ("DM-Placeholder", "TDM-Placeholder", "CTF-Placeholder", "SND-Placeholder"):
        write(os.path.join(out, "maps", name + ".bvm"), bvm(name))
        counts[".bvm"] = counts.get(".bvm", 0) + 1
    print("placeholder content:", counts, "skipped:", skipped)
    return 0


if __name__ == "__main__":
    sys.exit(main())
