#!/usr/bin/env python3
"""The game's own maps (content/maps) are playable as the server reads them (game/src/Map.cpp, version 20202).

Each map: a valid map name (the rule of bv2::isMapName, game/src/Paths.cpp), version 20202, a sane size, author set, every spawn and both flag pods on a
passable cell inside the map, all of them in one connected area, and flag pods at height 0: ServerCTF.cpp picks up a
flag within 0.25 of the pod measured with the player at z 0, so a pod at z 0.25 can never be taken. The CTF, TDM and
FFA launch scripts name only maps that exist here.

Usage: test_content_maps.py <repo root>
"""
import os
import re
import struct
import sys

NAME_CHARS = set("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_- ")
DEVICE_NAMES = {"CON", "PRN", "AUX", "NUL"} | {f"{d}{i}" for d in ("COM", "LPT") for i in range(1, 10)}
root = sys.argv[1]
maps_dir = os.path.join(root, "content", "maps")
failures = []


def fail(msg):
    failures.append(msg)
    print("FAIL " + msg)


def check_map(path):
    name = os.path.basename(path)[:-4]
    data = open(path, "rb").read()
    if not (1 <= len(name) <= 15 and set(name) <= NAME_CHARS and name[0] != " " and name[-1] != " ") \
            or name.upper() in DEVICE_NAMES:
        fail(f"{name}: not a valid map name")
    version, = struct.unpack_from("<I", data, 0)
    if version != 20202:
        return fail(f"{name}: version {version}, want 20202")
    author = data[4:29].split(b"\0")[0].decode("latin1")
    if not author:
        fail(f"{name}: no author")
    w, h = struct.unpack_from("<hh", data, 33)
    if not (8 <= w <= 128 and 8 <= h <= 128):
        return fail(f"{name}: size {w}x{h}")
    o = 37
    passable = [bool(data[o + 2 * i] & 0x80) for i in range(w * h)]
    o += 2 * w * h
    n, = struct.unpack_from("<h", data, o)
    o += 2
    points = [("spawn", struct.unpack_from("<fff", data, o + 12 * i)) for i in range(n)]
    o += 12 * n
    pods = None
    for _ in range(4):  # one section per game type
        sid, = struct.unpack_from("<h", data, o)
        o += 2
        if sid == 2:
            pods = [struct.unpack_from("<fff", data, o), struct.unpack_from("<fff", data, o + 12)]
            o += 24
        elif sid == 3:
            o += 24
            for _team in range(2):
                k, = struct.unpack_from("<h", data, o)
                o += 2 + 12 * k
        elif sid not in (0, 1):
            return fail(f"{name}: unknown game-type section {sid}")
    if o != len(data):
        fail(f"{name}: {len(data) - o} bytes after the last section")
    if n < 8:
        fail(f"{name}: {n} spawns, want at least 8")
    if pods is None:
        return fail(f"{name}: no CTF section")
    for k, pod in enumerate(pods):
        if pod[2] != 0.0:
            fail(f"{name}: flag pod {k} at z {pod[2]}, must be 0 or nobody can take the flag")
    points += [("flag pod", p) for p in pods]
    cells = []
    for kind, (x, y, _z) in points:
        cx, cy = int(x), int(y)
        if not (0 <= cx < w and 0 <= cy < h) or not passable[cy * w + cx]:
            fail(f"{name}: {kind} at {x},{y} is on a wall or outside the map")
        else:
            cells.append(cy * w + cx)
    if not cells:
        return
    seen = {cells[0]}
    todo = [cells[0]]
    while todo:
        c = todo.pop()
        x, y = c % w, c // w
        for a, b in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
            if 0 <= a < w and 0 <= b < h and passable[b * w + a] and b * w + a not in seen:
                seen.add(b * w + a)
                todo.append(b * w + a)
    if any(c not in seen for c in cells):
        fail(f"{name}: spawns and flag pods are not all connected")


names = set()
for f in sorted(os.listdir(maps_dir)):
    if f.endswith(".bvm"):
        names.add(f[:-4])
        check_map(os.path.join(maps_dir, f))
if not names:
    fail("no maps in content/maps")
for script in ("CTF", "TDM", "FFA"):
    text = open(os.path.join(root, "content", "LaunchScript", script + ".cfg")).read()
    for m in re.findall(r"^(?:dedicate|addmap) (.+?)\s*$", text, re.M):
        if m not in names:
            fail(f"{script}.cfg: {m} is not in content/maps")
if failures:
    sys.exit(1)
print(f"ok: {len(names)} maps")
