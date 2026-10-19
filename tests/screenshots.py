#!/usr/bin/env python3
"""Screenshot regression (Phase T7): run the autotest script tests/golden/<name>.txt, which takes `screenshot <name>`,
and compare the frame with tests/golden/<name>.png within a tolerance.

Usage: screenshots.py <bv2> <scratch dir> <name> [--update]
Passes and deletes the client's pref dir (screenshots included) when at most MAX_CHANGED of the pixels differ by more
than CHANNEL_TOLERANCE in a channel. On a failure it keeps <scratch>/<name>-actual.png and <name>-diff.png (changed
pixels red) for the CI artifact. --update writes the golden instead.
"""
import os
import shutil
import struct
import subprocess
import sys
import zlib

from autotest_client import client_command, start_xvfb

CHANNEL_TOLERANCE = 24  # per channel, 0-255: absorbs rasterizer rounding
MAX_CHANGED = 0.005  # fraction of pixels


def read_bmp(path):
    """(width, height, rows of RGB bytes, top row first) from the client's bottom-up 32 bpp BGRA BMP"""
    data = open(path, "rb").read()
    offset, = struct.unpack_from("<I", data, 10)
    width, height = struct.unpack_from("<ii", data, 18)
    rows = []
    for y in range(height - 1, -1, -1):
        bgra = data[offset + y * width * 4:offset + (y + 1) * width * 4]
        rgb = bytearray(width * 3)
        rgb[0::3], rgb[1::3], rgb[2::3] = bgra[2::4], bgra[1::4], bgra[0::4]
        rows.append(bytes(rgb))
    return width, height, rows


def write_png(path, width, height, rows):
    """8-bit RGB PNG, filter 0 on every row"""
    def chunk(kind, body):
        return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body))
    header = struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)
    pixels = zlib.compress(b"".join(b"\0" + row for row in rows), 9)
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", header) + chunk(b"IDAT", pixels) + chunk(b"IEND", b""))


def read_png(path):
    """Reads what write_png writes (8-bit RGB, filter 0)"""
    data = open(path, "rb").read()
    pos, idat = 8, b""
    while pos < len(data):
        length, = struct.unpack_from(">I", data, pos)
        kind, body = data[pos + 4:pos + 8], data[pos + 8:pos + 8 + length]
        if kind == b"IHDR":
            width, height = struct.unpack_from(">II", body)
        elif kind == b"IDAT":
            idat += body
        pos += length + 12
    raw, stride = zlib.decompress(idat), width * 3 + 1
    return width, height, [raw[y * stride + 1:(y + 1) * stride] for y in range(height)]


def compare(actual, golden):
    """(fraction of changed pixels, diff rows: changed pixels red, the rest the golden dimmed)"""
    (width, height, rows), (gw, gh, grows) = actual, golden
    if (width, height) != (gw, gh):
        return 1.0, None
    changed, diff = 0, []
    for row, grow in zip(rows, grows):
        out = bytearray(len(row))
        for x in range(0, len(row), 3):
            if max(abs(row[x] - grow[x]), abs(row[x + 1] - grow[x + 1]), abs(row[x + 2] - grow[x + 2])) > CHANNEL_TOLERANCE:
                changed += 1
                out[x] = 255
            else:
                out[x:x + 3] = bytes(c // 3 for c in grow[x:x + 3])
        diff.append(bytes(out))
    return changed / (width * height), diff


def main():
    exe, scratch, name = sys.argv[1], sys.argv[2], sys.argv[3]
    here = os.path.dirname(os.path.abspath(__file__))
    script, golden = os.path.join(here, "golden", name + ".txt"), os.path.join(here, "golden", name + ".png")
    pref = os.path.join(scratch, name)
    shutil.rmtree(pref, ignore_errors=True)
    os.makedirs(pref)
    cmd, env = client_command(exe, pref, script, "r_showStats false\n")  # no FPS counter
    xvfb = None
    if sys.platform.startswith("linux") and not os.environ.get("DISPLAY"):
        xvfb, env["DISPLAY"] = start_xvfb()
    try:
        r = subprocess.run(cmd, env=env, cwd=os.path.dirname(os.path.abspath(exe)), stdout=subprocess.PIPE,
                           stderr=subprocess.STDOUT, timeout=180)
    finally:
        if xvfb:
            xvfb.terminate()
            xvfb.wait()
    out = r.stdout.decode(errors="replace")
    shot = os.path.join(pref, "screenshots", name + ".bmp")
    if r.returncode != 0 or not os.path.exists(shot):
        print(out[-4000:])
        print(f"FAIL: exit code {r.returncode}, screenshot {'saved' if os.path.exists(shot) else 'missing'}")
        return 1
    actual = read_bmp(shot)
    if "--update" in sys.argv:
        write_png(golden, *actual)
        print(f"wrote {golden}")
        return 0
    if os.path.exists(golden):
        fraction, diff = compare(actual, read_png(golden))
        if fraction <= MAX_CHANGED:
            shutil.rmtree(pref)
            print(f"ok: {name}: {fraction:.4%} of the pixels changed")
            return 0
        print(f"FAIL: {name}: {fraction:.4%} of the pixels changed (limit {MAX_CHANGED:.2%})")
        if diff:
            write_png(os.path.join(scratch, name + "-diff.png"), actual[0], actual[1], diff)
    else:
        print(f"FAIL: no golden {golden}; run with --update to create it")
    write_png(os.path.join(scratch, name + "-actual.png"), *actual)
    print(f"kept {scratch}/{name}-actual.png" + (" and -diff.png" if os.path.exists(golden) else ""))
    return 1


if __name__ == "__main__":
    sys.exit(main())
