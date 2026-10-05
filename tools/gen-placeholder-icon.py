#!/usr/bin/env python3
"""Write the placeholder Windows application icon (.ico, 16/32/48 px, 32-bit BGRA).

A grey ball with a red visor on a dark rounded square. Generated at build time so no icon binary is committed.
Usage: gen-placeholder-icon.py <out.ico>
Python 3, stdlib only.
"""
import os
import struct
import sys

BG = (40, 44, 52)
BALL = (170, 176, 186)
VISOR = (220, 40, 40)


def pixel(x, y, n):
    # Coordinates in 0..1, sampled at the pixel centre
    u, v = (x + 0.5) / n, (y + 0.5) / n
    r = 0.18  # corner radius of the background square
    cu, cv = min(max(u, r), 1 - r), min(max(v, r), 1 - r)
    if (u - cu) ** 2 + (v - cv) ** 2 > r * r:
        return None  # transparent corner
    if (u - 0.5) ** 2 + (v - 0.54) ** 2 <= 0.34 ** 2:
        if 0.40 <= v <= 0.52 and 0.28 <= u <= 0.72:
            return VISOR
        return BALL
    return BG


def image(n):
    # BMP rows are bottom-up; ICO stores height doubled (XOR image + AND mask)
    header = struct.pack("<IiiHHIIiiII", 40, n, n * 2, 1, 32, 0, 0, 0, 0, 0, 0)
    xor = bytearray()
    for y in reversed(range(n)):
        for x in range(n):
            c = pixel(x, y, n)
            xor += bytes([0, 0, 0, 0]) if c is None else bytes([c[2], c[1], c[0], 255])
    stride = ((n + 31) // 32) * 4
    mask = bytes(stride * n)  # alpha channel decides transparency
    return header + bytes(xor) + mask


def ico(sizes=(16, 32, 48)):
    images = [image(n) for n in sizes]
    out = struct.pack("<HHH", 0, 1, len(sizes))
    offset = 6 + 16 * len(sizes)
    for n, data in zip(sizes, images):
        out += struct.pack("<BBBBHHII", n, n, 0, 0, 1, 32, len(data), offset)
        offset += len(data)
    return out + b"".join(images)


def main():
    if len(sys.argv) != 2:
        print(__doc__.strip().splitlines()[-2], file=sys.stderr)
        return 2
    out = sys.argv[1]
    os.makedirs(os.path.dirname(os.path.abspath(out)), exist_ok=True)
    with open(out, "wb") as f:
        f.write(ico())
    return 0


if __name__ == "__main__":
    sys.exit(main())
