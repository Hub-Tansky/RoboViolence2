#!/usr/bin/env python3
"""Write placeholder game data for development and CI: no original assets.

Reads the `required` rows of docs/assets/ASSET-INVENTORY.md and writes, under <out>/:
  - a solid-colour TGA (same size and depth) for every required texture and font,
  - a short silent WAV (same format) for every required sound,
  - an empty but valid .DKO model for every required model,
  - four tiny maps (DM, TDM, CTF, SND) in .bvm v20202.
Music (.ogg) is not generated: only the client plays it, and no encoder is available here.
Usage: gen-placeholder-content.py <out-dir> [--inventory FILE]
Python 3, stdlib only.
"""
import os
import re
import struct
import sys
import zlib

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEFAULT_INVENTORY = os.path.join(ROOT, "docs", "assets", "ASSET-INVENTORY.md")


def colour(name):
    h = zlib.crc32(name.encode()) & 0xFFFFFF
    return 64 + (h & 0x7F), 64 + ((h >> 8) & 0x7F), 64 + ((h >> 16) & 0x7F)


def tga(name, w, h, bpp):
    r, g, b = colour(name)
    px = bytes([b, g, r]) if bpp == 24 else bytes([b, g, r, 255])
    header = struct.pack("<BBBHHBHHHHBB", 0, 0, 2, 0, 0, 0, 0, 0, w, h, bpp, 0 if bpp == 24 else 8)
    return header + px * (w * h)


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


def required_rows(inventory):
    folder = None
    for line in open(inventory, encoding="utf-8"):
        m = re.match(r"## `(?:BaboViolent2/Content/)?(main/[^`]*)`", line)
        if m:
            folder = m.group(1)
            continue
        cells = [c.strip() for c in line.strip().strip("|").split("|")]
        if folder and len(cells) == 6 and cells[0].startswith("`") and cells[4] == "required":
            yield folder, cells[0].strip("`"), cells[1]


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
    inv = DEFAULT_INVENTORY
    if "--inventory" in sys.argv:
        inv = sys.argv[sys.argv.index("--inventory") + 1]
    # content/ holds only languages and scripts; main/ in the runtime dir is the game's data root
    counts, skipped = {}, []
    for folder, rel, typ in required_rows(inv):
        # folder is "main/<kind>"; the runtime main/ dir is `out` itself
        dest = os.path.join(out, folder.split("/", 1)[1], rel)
        ext = os.path.splitext(rel)[1].lower()
        if ext == ".tga":
            m = re.match(r"TGA (\d+)×(\d+)×(\d+)", typ)
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
