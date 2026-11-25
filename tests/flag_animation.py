#!/usr/bin/env python3
"""ROBO-247: helpers for the e2e scenario ctf_flag_animation (tests/e2e/e2e.py): a large flag as glTF, converted by
tools/gltf-to-dko.py into a pack that holds models/BlueFlag.DKO and models/RedFlag.DKO, stacked over the placeholders
through BV2_DATA_DIR (ADR 0069).

The flag is a 6 x 4 m cloth lying flat 0.5 m above the floor, so the camera above sees it whole, and its one morph
target shears it 3 m along x; the weight goes 0 -> 1 -> 0 over 1 s.
"""
import importlib.util
import json
import os
import shutil
import struct


def flag_glb(path, animated=True):
    """The flag as .glb: 12 x 4 quads in glTF's XZ plane (the game's XY plane); without its animation when not
    animated."""
    cols, rows, width, height = 12, 4, 6.0, 4.0
    pos, ripple = [], []
    for r in range(rows):
        for c in range(cols):
            corners = [(c, r), (c + 1, r), (c + 1, r + 1), (c, r), (c + 1, r + 1), (c, r + 1)]
            for x, y in corners:
                pos.append((x * width / cols, 0.5, y * height / rows))
                ripple.append((3.0 * y / rows, 0.0, 0.0))
    blob, views, accessors = bytearray(), [], []

    def add(values, kind, count):
        flat = [x for v in values for x in (v if isinstance(v, tuple) else (v,))]
        data = struct.pack(f"<{len(flat)}f", *flat)
        views.append({"buffer": 0, "byteOffset": len(blob), "byteLength": len(data)})
        blob.extend(data)
        acc = {"bufferView": len(views) - 1, "componentType": 5126, "count": count, "type": kind}
        if kind == "VEC3":
            acc["min"] = [min(v[i] for v in values) for i in range(3)]
            acc["max"] = [max(v[i] for v in values) for i in range(3)]
        else:
            acc["min"], acc["max"] = [min(values)], [max(values)]
        accessors.append(acc)
        return len(accessors) - 1

    p, n, t = add(pos, "VEC3", len(pos)), add([(0, 1, 0)] * len(pos), "VEC3", len(pos)), add(ripple, "VEC3", len(pos))
    times, weights = add([0.0, 0.5, 1.0], "SCALAR", 3), add([0.0, 1.0, 0.0], "SCALAR", 3)
    doc = {"asset": {"version": "2.0"}, "scene": 0, "scenes": [{"nodes": [0]}],
           "nodes": [{"name": "Flag", "mesh": 0}],
           "meshes": [{"primitives": [{"attributes": {"POSITION": p, "NORMAL": n}, "material": 0, "targets": [{"POSITION": t}]}],
                       "weights": [0.0]}],
           "materials": [{"name": "Cloth", "doubleSided": True,
                          "pbrMetallicRoughness": {"baseColorFactor": [1.0, 0.9, 0.1, 1.0]}}],
           "animations": [{"name": "wave", "samplers": [{"input": times, "output": weights}],
                           "channels": [{"sampler": 0, "target": {"node": 0, "path": "weights"}}]}],
           "accessors": accessors, "bufferViews": views, "buffers": [{"byteLength": len(blob)}]}
    if not animated:
        del doc["animations"]
    js = json.dumps(doc).encode()
    js += b" " * (-len(js) % 4)
    blob += b"\0" * (-len(blob) % 4)
    body = struct.pack("<II", len(js), 0x4E4F534A) + js + struct.pack("<II", len(blob), 0x004E4942) + bytes(blob)
    with open(path, "wb") as f:
        f.write(struct.pack("<III", 0x46546C67, 2, 12 + len(body)) + body)


def make_pack(root, glb, pack):
    """Writes <pack>/main with both flag models converted from glb and their pack.tsv."""
    spec = importlib.util.spec_from_file_location("gltf_to_dko", os.path.join(root, "tools", "gltf-to-dko.py"))
    conv = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(conv)
    shutil.rmtree(pack, ignore_errors=True)
    os.makedirs(os.path.join(pack, "main", "models"))
    os.makedirs(os.path.join(pack, "main", "textures"))  # the client takes BV2_DATA_DIR as a data root only with it
    rows = ["id\tfile\ttype\tformat\tauthor\ttool\tlicence\tsource"]
    for name in ("BlueFlag", "RedFlag"):
        if conv.main([glb, os.path.join(pack, "main", "models", name + ".DKO")]) != 0:
            raise AssertionError(f"gltf-to-dko failed on {glb}")
        rows.append(f"models/{name}\tmodels/{name}.DKO\tmodel\tDKO v2\tRV2 tests\ttools/gltf-to-dko.py\t"
                    "LicenseRef-RV2-Art\tgenerated fixture")
    with open(os.path.join(pack, "main", "pack.tsv"), "w", encoding="utf-8") as f:
        f.write("\n".join(rows) + "\n")
