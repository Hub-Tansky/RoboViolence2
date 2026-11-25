#!/usr/bin/env python3
"""ROBO-239: tools/gltf-to-dko.py turns a small glTF (as .gltf and as .glb) into a .DKO with the expected meshes,
materials, material groups, vertices and dummy; engine/dko loads the result (through dko_probe, when given) and
reports the same bounding box and dummy position; unsupported input fails with exit 1 instead of a broken file.
ROBO-247: an animated flag (morph target weights, a moving mesh node and a moving dummy, LINEAR, STEP and CUBICSPLINE
keys, a late start) and a skinned strip convert to DKO frames with the expected TIME_INFO and interpolated vertices,
and engine/dko reads every frame; two animations fail.

Usage: test_gltf_to_dko.py <repo root> <scratch dir> [<dko_probe executable>]
"""
import base64
import importlib.util
import json
import math
import os
import shutil
import struct
import subprocess
import sys
import zlib

ROOT, SCRATCH = sys.argv[1], os.path.join(sys.argv[2], "gltf-to-dko")
PROBE = sys.argv[3] if len(sys.argv) > 3 else None
spec = importlib.util.spec_from_file_location("gltf_to_dko", os.path.join(ROOT, "tools", "gltf-to-dko.py"))
conv = importlib.util.module_from_spec(spec)
spec.loader.exec_module(conv)

failures = []


def check(ok, what):
    if not ok:
        failures.append(what)
        print(f"FAIL: {what}")


def close(a, b, eps=1e-4):
    return len(a) == len(b) and all(abs(x - y) < eps for x, y in zip(a, b))


# --- a small model: "body" (cube in Red + quad in Steel), child "barrel" (triangle in Steel), its child "flash1"
CUBE = [(x, y, z) for x in (-1, 1) for y in (-1, 1) for z in (-1, 1)]
CUBE_IDX = [0, 1, 3, 0, 3, 2, 4, 6, 7, 4, 7, 5, 0, 4, 5, 0, 5, 1, 2, 3, 7, 2, 7, 6, 0, 2, 6, 0, 6, 4, 1, 5, 7, 1, 7, 3]
QUAD = [(2, 0, 0), (3, 0, 0), (3, 1, 0), (2, 0, 0), (3, 1, 0), (2, 1, 0)]
QUAD_N = [(0, 0, 1)] * 6
QUAD_UV = [(0, 0), (1, 0), (1, 1), (0, 0), (1, 1), (0, 1)]
TRI = [(0, 0, 0), (0.5, 0, 0), (0, 0.5, 0)]
BARREL_T, FLASH_T = (1, 2, 3), (0, 0, 0.5)


def build(edit=None, mode=None, sparse=False):
    """(glTF json, binary buffer) of the small model."""
    blob, views, accessors = bytearray(), [], []

    def add(values, fmt, kind, ctype, target=None):
        while len(blob) % 4:
            blob.append(0)
        flat = [x for v in values for x in (v if isinstance(v, tuple) else (v,))]
        data = struct.pack(f"<{len(flat)}{fmt}", *flat)
        views.append({"buffer": 0, "byteOffset": len(blob), "byteLength": len(data)})
        if target:
            views[-1]["target"] = target
        blob.extend(data)
        acc = {"bufferView": len(views) - 1, "componentType": ctype, "count": len(values), "type": kind}
        if kind == "VEC3" and fmt == "f":
            acc["min"] = [min(v[i] for v in values) for i in range(3)]
            acc["max"] = [max(v[i] for v in values) for i in range(3)]
        accessors.append(acc)
        return len(accessors) - 1

    cube = add(CUBE, "f", "VEC3", 5126)
    cube_idx = add(CUBE_IDX, "H", "SCALAR", 5123)
    quad, quad_n, quad_uv = add(QUAD, "f", "VEC3", 5126), add(QUAD_N, "f", "VEC3", 5126), add(QUAD_UV, "f", "VEC2", 5126)
    tri = add(TRI, "f", "VEC3", 5126)
    if sparse:
        accessors[tri]["sparse"] = {"count": 1}
    body = {"primitives": [{"attributes": {"POSITION": cube}, "indices": cube_idx, "material": 0},
                           {"attributes": {"POSITION": quad, "NORMAL": quad_n, "TEXCOORD_0": quad_uv}, "material": 1}]}
    barrel = {"primitives": [{"attributes": {"POSITION": tri}, "material": 1}]}
    if mode is not None:
        barrel["primitives"][0]["mode"] = mode
    materials = [{"name": "Red", "pbrMetallicRoughness": {"baseColorFactor": [1, 0, 0, 1]}},
                 {"name": "Steel", "doubleSided": True, "pbrMetallicRoughness": {"baseColorFactor": [0.5, 0.5, 0.5, 0.8]}}]
    doc = {"asset": {"version": "2.0"}, "scene": 0, "scenes": [{"nodes": [0]}],
           "nodes": [{"name": "body", "mesh": 0, "children": [1]},
                     {"name": "barrel", "mesh": 1, "translation": list(BARREL_T), "children": [2]},
                     {"name": "flash1", "translation": list(FLASH_T)}],
           "meshes": [body, barrel], "materials": materials, "accessors": accessors, "bufferViews": views,
           "buffers": [{"byteLength": len(blob)}]}
    if edit:
        edit(doc)
    return doc, bytes(blob)


def write_gltf(path, doc, blob):
    doc = json.loads(json.dumps(doc))
    doc["buffers"][0]["uri"] = "data:application/octet-stream;base64," + base64.b64encode(blob).decode()
    with open(path, "w") as f:
        json.dump(doc, f)


def write_glb(path, doc, blob):
    js = json.dumps(doc).encode()
    js += b" " * (-len(js) % 4)
    blob += b"\0" * (-len(blob) % 4)
    body = struct.pack("<II", len(js), 0x4E4F534A) + js + struct.pack("<II", len(blob), 0x004E4942) + blob
    with open(path, "wb") as f:
        f.write(struct.pack("<III", 0x46546C67, 2, 12 + len(body)) + body)


def png_1x1():
    raw = b"\0\xff\0\0"
    def part(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data))
    return (b"\x89PNG\r\n\x1a\n" + part(b"IHDR", struct.pack(">IIBBBBB", 1, 1, 8, 2, 0, 0, 0))
            + part(b"IDAT", zlib.compress(raw)) + part(b"IEND", b""))


# --- an independent reader that follows engine/dko's CDkoModel::loadFromFile
class Reader:
    def __init__(self, data):
        self.data, self.pos = data, 0

    def take(self, fmt):
        v = struct.unpack_from("<" + fmt, self.data, self.pos)
        self.pos += struct.calcsize("<" + fmt)
        return v

    def chunk(self):
        return self.take("h")[0]

    def string(self):
        end = self.data.index(b"\0", self.pos)
        s = self.data[self.pos:end].decode("latin-1")
        self.pos = end + 1
        return s


def read_dko(path):
    r = Reader(open(path, "rb").read())
    model = {"materials": [], "meshes": [], "dummies": [], "frames": None}
    c = conv.CHUNK
    while True:
        cid = r.chunk()
        if cid == c["END"]:
            break
        if cid == c["VERSION"]:
            model["version"] = r.take("h")[0]
        elif cid == c["TIME_INFO"]:
            model["time_info"] = r.take("hhh")
            model["frames"] = model["time_info"][2]
        elif cid == c["PROPERTIES"]:
            while (sub := r.chunk()) != c["END"]:
                {c["NAME"]: r.string, c["POSITION"]: lambda: r.take("3f"), c["MATRIX"]: lambda: r.take("9f")}[sub]()
        elif cid == c["MATLIST"]:
            n = r.take("h")[0]
            mats = [None] * n
            for i in range(n):
                m = {}
                while (sub := r.chunk()) != c["END"]:
                    if sub == c["MATNAME"]:
                        m["name"] = r.string()
                    elif sub == c["TEX_DIFFUSE"]:
                        m["texture"] = r.string()
                    elif sub in (c["AMBIENT"], c["DIFFUSE"], c["SPECULAR"], c["EMISSIVE"]):
                        m[sub] = r.take("4f")
                    elif sub == c["SHININESS"]:
                        r.take("h")
                    elif sub == c["TRANSPARENCY"]:
                        m["transparency"] = r.take("f")[0]
                    elif sub == c["TWO_SIDED"]:
                        m["two_sided"] = r.take("B")[0]
                    else:
                        raise ValueError(f"material chunk {sub:#x}")
                mats[n - i - 1] = m
            model["materials"] = mats
        elif cid == c["TRI_MESH"]:
            mesh = {"groups": []}
            while (sub := r.chunk()) != c["END"]:
                if sub == c["NAME"]:
                    mesh["name"] = r.string()
                elif sub == c["POSITION"]:
                    r.take("3f")
                elif sub == c["MATRIX"]:
                    r.take("9f")
                elif sub == c["NB_MAT_GROUP"]:
                    for _ in range(r.take("h")[0]):
                        g = {}
                        while (gc := r.chunk()) != c["END"]:
                            if gc == c["MAT_ID"]:
                                g["material"] = r.take("h")[0]
                            elif gc == c["NB_VERTEX"]:
                                g["n"] = r.take("i")[0]
                            elif gc in (c["VERTEX_ARRAY"], c["NORMAL_ARRAY"]):
                                # one array per frame, back to back (CdkoMesh::loadMatGroup)
                                per = []
                                for _ in range(model["frames"]):
                                    f = r.take(f"{g['n'] * 3}f")
                                    per.append([f[i:i + 3] for i in range(0, len(f), 3)])
                                key = "vertices" if gc == c["VERTEX_ARRAY"] else "normals"
                                g[key], g[key + "_frames"] = per[0], per
                            elif gc == c["TEXCOORD_ARRAY"]:
                                f = r.take(f"{g['n'] * 2}f")
                                g["uvs"] = [f[i:i + 2] for i in range(0, len(f), 2)]
                            else:
                                raise ValueError(f"group chunk {gc:#x}")
                        mesh["groups"].append(g)
                else:
                    raise ValueError(f"mesh chunk {sub:#x}")
            model["meshes"].append(mesh)
        elif cid == c["DUMMY"]:
            d = {}
            while (sub := r.chunk()) != c["END"]:
                if sub == c["NAME"]:
                    d["name"] = r.string()
                elif sub == c["POSITION"]:
                    d["positions"] = [r.take("3f") for _ in range(model["frames"])]
                    d["position"] = d["positions"][0]
                elif sub == c["MATRIX"]:
                    d["matrix"] = [r.take("9f") for _ in range(model["frames"])][0]
            model["dummies"].append(d)
        else:
            raise ValueError(f"top chunk {cid:#x}")
    check(r.pos == len(r.data), f"{path}: {len(r.data) - r.pos} bytes after the END chunk")
    return model


def game(v):
    """glTF axes to game axes, as the converter documents: (x, y, z) -> (x, -z, y)."""
    return (v[0], -v[2], v[1])


def check_model(path, label):
    m = read_dko(path)
    check(m.get("version") == 2 and m["frames"] == 1, f"{label}: version 2, one frame")
    check([x["name"] for x in m["materials"]] == ["Red", "Steel"], f"{label}: materials Red, Steel in order")
    check(close(m["materials"][0][conv.CHUNK["DIFFUSE"]], (1, 0, 0, 1)), f"{label}: Red diffuse")
    check(abs(m["materials"][1]["transparency"] - 0.2) < 1e-6 and m["materials"][1]["two_sided"] == 1,
          f"{label}: Steel transparency 0.2, two-sided")
    check([x.get("name") for x in m["meshes"]] == ["body", "barrel"], f"{label}: meshes body, barrel")
    groups = [[(g["material"], g["n"]) for g in x["groups"]] for x in m["meshes"]]
    check(groups == [[(0, 36), (1, 6)], [(1, 3)]], f"{label}: material groups {groups}")
    body = m["meshes"][0]["groups"]
    check(close([x for v in body[0]["vertices"] for x in v],
                [x for i in CUBE_IDX for x in game(CUBE[i])]), f"{label}: cube de-indexed in game axes")
    check(all(close(n, (0, -1, 0)) for n in body[1]["normals"]), f"{label}: quad normal +Z becomes -Y")
    check(close([x for uv in body[1]["uvs"] for x in uv], [x for u, v in QUAD_UV for x in (u, 1 - v)]),
          f"{label}: uv v flipped")
    barrel = m["meshes"][1]["groups"][0]["vertices"]
    want = [game(tuple(p + t for p, t in zip(v, BARREL_T))) for v in TRI]
    check(close([x for v in barrel for x in v], [x for v in want for x in v]), f"{label}: barrel at its node")
    flash = game(tuple(a + b for a, b in zip(BARREL_T, FLASH_T)))
    dummies = [(d["name"], d["position"]) for d in m["dummies"]]
    check(len(dummies) == 1 and dummies[0][0] == "flash1" and close(dummies[0][1], flash),
          f"{label}: dummy flash1 at {flash}, got {dummies}")
    return m, flash


def probe(path, model, flash, label):
    out = subprocess.run([PROBE, path, "flash1"], capture_output=True, text=True)
    check(out.returncode == 0, f"{label}: dko_probe exit {out.returncode}: {out.stdout}{out.stderr}")
    if out.returncode:
        return
    got = {line.split()[1]: [float(x) for x in line.split()[2:]] for line in out.stdout.splitlines()
           if line.startswith("probe ")}
    verts = [v for x in model["meshes"] for g in x["groups"] for v in g["vertices"]]
    lo = [min(v[i] for v in verts) for i in range(3)]
    hi = [max(v[i] for v in verts) for i in range(3)]
    box = [(lo[i] + hi[i]) / 2 for i in range(3)] + [(hi[i] - lo[i]) / 2 for i in range(3)]
    check(close(got.get("oabb", []), box), f"{label}: engine bounding box {got.get('oabb')} != {box}")
    radius = max(sum(x * x for x in v) ** 0.5 for v in verts)
    check(close(got.get("radius", []), [radius]), f"{label}: engine radius {got.get('radius')} != {radius}")
    check(close(got.get("dummy", []), flash), f"{label}: engine dummy {got.get('dummy')} != {flash}")


def run(src, dst, *extra):
    return conv.main([src, dst, "--scale", "1", *extra])


shutil.rmtree(SCRATCH, ignore_errors=True)
os.makedirs(SCRATCH)
doc, blob = build()
for ext, writer in ((".gltf", write_gltf), (".glb", write_glb)):
    src, dst = os.path.join(SCRATCH, "weapon" + ext), os.path.join(SCRATCH, f"weapon{ext[1:]}.DKO")
    writer(src, doc, blob)
    check(run(src, dst) == 0, f"{ext}: converter exit 0")
    if os.path.exists(dst):
        model, flash = check_model(dst, ext)
        if PROBE:
            probe(dst, model, flash, ext)

# without --scale, 1 m is 200 DKO units (the game draws models at 0.005)
dst = os.path.join(SCRATCH, "default-scale.DKO")
check(conv.main([os.path.join(SCRATCH, "weapon.glb"), dst]) == 0, "default scale: exit 0")
if os.path.exists(dst):
    got = read_dko(dst)["dummies"][0]["position"]
    want = tuple(200 * x for x in game(tuple(a + b for a, b in zip(BARREL_T, FLASH_T))))
    check(close(got, want, 1e-2), f"default scale: dummy at {want}, got {got}")

# an embedded PNG on Red is written beside the .DKO and named by TEX_DIFFUSE
def textured(d):
    d["images"] = [{"name": "skin", "uri": "data:image/png;base64," + base64.b64encode(png_1x1()).decode()}]
    d["textures"] = [{"source": 0}]
    d["materials"][0]["pbrMetallicRoughness"]["baseColorTexture"] = {"index": 0}


tex_dir = os.path.join(SCRATCH, "tex")
os.makedirs(tex_dir)
write_gltf(os.path.join(SCRATCH, "textured.gltf"), *build(textured))
check(run(os.path.join(SCRATCH, "textured.gltf"), os.path.join(tex_dir, "textured.DKO")) == 0, "textured: exit 0")
check(open(os.path.join(tex_dir, "skin.png"), "rb").read() == png_1x1(), "textured: skin.png written beside the DKO")
if os.path.exists(os.path.join(tex_dir, "textured.DKO")):
    mats = read_dko(os.path.join(tex_dir, "textured.DKO"))["materials"]
    check(mats[0].get("texture") == "skin.png" and "texture" not in mats[1], "textured: Red has TEX_DIFFUSE skin.png")
    if PROBE:
        out = subprocess.run([PROBE, os.path.join(tex_dir, "textured.DKO"), "flash1"], capture_output=True, text=True)
        check(out.returncode == 0, f"textured: dko_probe exit {out.returncode}: {out.stdout}")

# an external image file is copied beside the .DKO
def external(d):
    d["images"] = [{"uri": "skin2.png"}]
    d["textures"] = [{"source": 0}]
    d["materials"][1]["pbrMetallicRoughness"]["baseColorTexture"] = {"index": 0}


ext_dir = os.path.join(SCRATCH, "ext")
os.makedirs(ext_dir)
with open(os.path.join(SCRATCH, "skin2.png"), "wb") as f:
    f.write(png_1x1())
write_gltf(os.path.join(SCRATCH, "external.gltf"), *build(external))
check(run(os.path.join(SCRATCH, "external.gltf"), os.path.join(ext_dir, "external.DKO")) == 0, "external: exit 0")
check(os.path.exists(os.path.join(ext_dir, "skin2.png")), "external: skin2.png copied beside the DKO")

# unsupported input: exit 1 and no output file
def no_keys(d):
    d["animations"] = [{"channels": [], "samplers": []}]


def skin_without_weights(d):
    d["skins"] = [{"joints": [2]}]
    d["nodes"][1]["skin"] = 0


def matrix_animated(d):
    d["nodes"][2]["matrix"] = [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]
    del d["nodes"][2]["translation"]
    d["accessors"].append({"bufferView": 0, "componentType": 5126, "count": 2, "type": "SCALAR"})
    d["accessors"].append({"bufferView": 0, "componentType": 5126, "count": 2, "type": "VEC3"})
    n = len(d["accessors"])
    d["animations"] = [{"samplers": [{"input": n - 2, "output": n - 1}],
                        "channels": [{"sampler": 0, "target": {"node": 2, "path": "translation"}}]}]


for label, kwargs in (("lines", {"mode": 1}), ("sparse", {"sparse": True}), ("no-keys", {"edit": no_keys}),
                      ("skin-no-weights", {"edit": skin_without_weights}),
                      ("matrix-animated", {"edit": matrix_animated})):
    src, dst = os.path.join(SCRATCH, label + ".gltf"), os.path.join(SCRATCH, label + ".DKO")
    write_gltf(src, *build(**kwargs))
    check(run(src, dst) == 1 and not os.path.exists(dst), f"{label}: exit 1, no .DKO written")


# --- ROBO-247: animated models
class Blob:
    """Accessors and buffer views of a glTF under construction."""
    def __init__(self):
        self.blob, self.views, self.accessors = bytearray(), [], []

    def add(self, values, kind, fmt="f", ctype=5126):
        while len(self.blob) % 4:
            self.blob.append(0)
        flat = [x for v in values for x in (v if isinstance(v, tuple) else (v,))]
        data = struct.pack(f"<{len(flat)}{fmt}", *flat)
        self.views.append({"buffer": 0, "byteOffset": len(self.blob), "byteLength": len(data)})
        self.blob.extend(data)
        acc = {"bufferView": len(self.views) - 1, "componentType": ctype, "count": len(values), "type": kind}
        if kind == "SCALAR" and fmt == "f":
            acc["min"], acc["max"] = [min(values)], [max(values)]
        if kind == "VEC3" and fmt == "f":
            acc["min"] = [min(v[i] for v in values) for i in range(3)]
            acc["max"] = [max(v[i] for v in values) for i in range(3)]
        self.accessors.append(acc)
        return len(self.accessors) - 1

    def doc(self, nodes, meshes, animations, skins=None):
        d = {"asset": {"version": "2.0"}, "scene": 0, "scenes": [{"nodes": [0]}], "nodes": nodes, "meshes": meshes,
             "materials": [{"name": "Cloth", "doubleSided": True}], "animations": animations,
             "accessors": self.accessors, "bufferViews": self.views, "buffers": [{"byteLength": len(self.blob)}]}
        if skins:
            d["skins"] = skins
        return d, bytes(self.blob)


# a flag: a 2 x 1 cloth of 4 triangles, one morph target that ripples it along glTF z, its weight 0 -> 1 -> 0 over
# 1 s, and "Dummy01" (no mesh) sliding 1 m along glTF x; two samplers share the key times
FLAG = [(0, 0, 0), (1, 0, 0), (1, 1, 0), (0, 0, 0), (1, 1, 0), (0, 1, 0),
        (1, 0, 0), (2, 0, 0), (2, 1, 0), (1, 0, 0), (2, 1, 0), (1, 1, 0)]
RIPPLE = [(0, 0, 0.5 * v[0]) for v in FLAG]
WAVE_T, WAVE_W = [0.0, 0.5, 1.0], [0.0, 1.0, 0.0]
DUMMY_T = [(0, 0, 0), (1, 0, 0), (0, 0, 0)]


def flag(interpolation="LINEAR", extra_animation=False, start=0.0, lift=False):
    """start: the first key's time; lift: the Flag node also rises 2 m along glTF y and back."""
    b = Blob()
    pos, nrm = b.add(FLAG, "VEC3"), b.add([(0, 0, 1)] * len(FLAG), "VEC3")
    ripple = b.add(RIPPLE, "VEC3")
    times = b.add([t + start for t in WAVE_T], "SCALAR")
    if interpolation == "CUBICSPLINE":  # in-tangent, value, out-tangent per key; flat tangents
        weights = b.add([x for w in WAVE_W for x in (0.0, w, 0.0)], "SCALAR")
        slide = b.add([x for v in DUMMY_T for x in ((0, 0, 0), v, (0, 0, 0))], "VEC3")
    else:
        weights, slide = b.add(WAVE_W, "SCALAR"), b.add(DUMMY_T, "VEC3")
    mesh = {"primitives": [{"attributes": {"POSITION": pos, "NORMAL": nrm}, "material": 0,
                            "targets": [{"POSITION": ripple}]}], "weights": [0.0]}
    nodes = [{"name": "Flag", "mesh": 0, "children": [1]}, {"name": "Dummy01", "translation": [0, 0, 0]}]
    anims = [{"name": "wave", "samplers": [{"input": times, "output": weights, "interpolation": interpolation},
                                           {"input": times, "output": slide, "interpolation": interpolation}],
              "channels": [{"sampler": 0, "target": {"node": 0, "path": "weights"}},
                           {"sampler": 1, "target": {"node": 1, "path": "translation"}}]}]
    if lift:
        anims[0]["samplers"].append({"input": times, "output": b.add([(0, 0, 0), (0, 2, 0), (0, 0, 0)], "VEC3")})
        anims[0]["channels"].append({"sampler": 2, "target": {"node": 0, "path": "translation"}})
    if extra_animation:
        anims.append({"name": "still", "samplers": [{"input": times, "output": weights}],
                      "channels": [{"sampler": 0, "target": {"node": 0, "path": "weights"}}]})
    return b.doc(nodes, [mesh], anims)


def flag_frame(w):
    """The flag's vertices in game axes at morph weight w."""
    return [game(tuple(p + w * r for p, r in zip(v, d))) for v, d in zip(FLAG, RIPPLE)]


def flat(vs):
    return [x for v in vs for x in v]


def weight_at(t, interpolation="LINEAR"):
    k = 0 if t < 0.5 else 1
    u = (t - WAVE_T[k]) / 0.5
    a, b = WAVE_W[k], WAVE_W[k + 1]
    if interpolation == "STEP":
        return a
    if interpolation == "CUBICSPLINE":  # flat tangents: Hermite basis h00, h01
        return (2 * u ** 3 - 3 * u ** 2 + 1) * a + (-2 * u ** 3 + 3 * u ** 2) * b
    return a + (b - a) * u


def convert_flag(label, doc_blob, *extra):
    src, dst = os.path.join(SCRATCH, label + ".glb"), os.path.join(SCRATCH, label + ".DKO")
    write_glb(src, *doc_blob)
    rc = run(src, dst, *extra)
    check(rc == 0, f"{label}: exit 0, got {rc}")
    return read_dko(dst) if rc == 0 and os.path.exists(dst) else None


# default: 10 frames a second, a loop (frame 9 plays back into frame 0), so 1 s gives TIME_INFO (0, 9, 10)
m = convert_flag("flag", flag())
if m:
    check(m["time_info"] == (0, 9, 10), f"flag: TIME_INFO (0, 9, 10), got {m['time_info']}")
    g = m["meshes"][0]["groups"][0]
    check(len(g["vertices_frames"]) == 10 and len(g["normals_frames"]) == 10, "flag: 10 vertex and normal arrays")
    # frame 3 is t = 0.3 s, between the keys at 0 and 0.5: weight 0.6
    check(close(flat(g["vertices_frames"][3]), flat(flag_frame(0.6))), "flag: frame 3 is the ripple at weight 0.6")
    check(close(flat(g["vertices_frames"][5]), flat(flag_frame(1.0))), "flag: frame 5 is the full ripple")
    check(close(flat(g["vertices_frames"][0]), flat(flag_frame(0.0))), "flag: frame 0 is the flat cloth")
    d = m["dummies"][0]
    check(d["name"] == "Dummy01" and len(d["positions"]) == 10, "flag: Dummy01 has 10 positions")
    check(close(d["positions"][3], game((0.6, 0, 0))), f"flag: Dummy01 at frame 3 {d['positions'][3]}")
    if PROBE:
        out = subprocess.run([PROBE, os.path.join(SCRATCH, "flag.DKO"), "Dummy01", "3"], capture_output=True,
                             text=True)
        check(out.returncode == 0, f"flag: dko_probe exit {out.returncode}: {out.stdout}{out.stderr}")
        got = {line.split()[1]: [float(x) for x in line.split()[2:]] for line in out.stdout.splitlines()
               if line.startswith("probe ")}
        check(got.get("frames") == [10], f"flag: engine frame count {got.get('frames')}")
        check(close(got.get("dummy", []), game((0.6, 0, 0))), f"flag: engine Dummy01 at frame 3 {got.get('dummy')}")

# a late first key: frames from 0.5 s, TIME_INFO's first frame is 5
m = convert_flag("flag-late", flag(start=0.5))
if m:
    check(m["time_info"] == (5, 14, 10), f"late start: TIME_INFO (5, 14, 10), got {m['time_info']}")
    v = m["meshes"][0]["groups"][0]["vertices_frames"]
    check(close(flat(v[3]), flat(flag_frame(0.6))), "late start: frame 3 is 0.3 s after the first key")

# an animated mesh node: its translation moves the vertices (and its child dummy) with the ripple
m = convert_flag("flag-lift", flag(lift=True))
if m:
    v = m["meshes"][0]["groups"][0]["vertices_frames"]
    want = [tuple(a + b for a, b in zip(p, game((0, 1.2, 0)))) for p in flag_frame(0.6)]
    check(close(flat(v[3]), flat(want)), "mesh node translation: frame 3 raised 1.2 m along glTF y")
    check(close(m["dummies"][0]["positions"][3], game((0.6, 1.2, 0))), "mesh node translation: child dummy follows")

# STEP holds a key; CUBICSPLINE with flat tangents eases
for interp in ("STEP", "CUBICSPLINE"):
    m = convert_flag("flag-" + interp.lower(), flag(interp))
    if m:
        v = m["meshes"][0]["groups"][0]["vertices_frames"]
        w = weight_at(0.3, interp)
        check(close(flat(v[3]), flat(flag_frame(w))), f"{interp}: frame 3 at weight {w:.3f}")

# a .DKO holds one animation
src, dst = os.path.join(SCRATCH, "flag-two.glb"), os.path.join(SCRATCH, "flag-two.DKO")
write_glb(src, *flag(extra_animation=True))
check(run(src, dst) == 1 and not os.path.exists(dst), "two animations: exit 1, no .DKO written")


# a skinned strip: two joints, the root still and "tip" turning 90 degrees about glTF z over 1 s; the far vertex
# follows the tip, the middle one is weighted half and half
def skinned_strip():
    b = Blob()
    verts = [(0, 0, 0), (1, 0, 0), (2, 0, 0)]
    pos = b.add(verts, "VEC3")
    joints = b.add([(0, 0, 0, 0), (0, 1, 0, 0), (1, 0, 0, 0)], "VEC4", "B", 5121)
    weights = b.add([(1, 0, 0, 0), (0.5, 0.5, 0, 0), (1, 0, 0, 0)], "VEC4")
    # inverse bind: root at the origin, tip at x = 1
    ibm = b.add([tuple([1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]),
                 tuple([1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, -1, 0, 0, 1])], "MAT4")
    times = b.add([0.0, 1.0], "SCALAR")
    s = math.sin(math.pi / 4)
    rot = b.add([(0, 0, 0, 1), (0, 0, s, s)], "VEC4")
    mesh = {"primitives": [{"attributes": {"POSITION": pos, "JOINTS_0": joints, "WEIGHTS_0": weights},
                            "material": 0}]}
    nodes = [{"name": "rig", "children": [1, 2]}, {"name": "strip", "mesh": 0, "skin": 0},
             {"name": "root", "children": [3]}, {"name": "tip", "translation": [1, 0, 0]}]
    skins = [{"joints": [2, 3], "inverseBindMatrices": ibm}]
    anims = [{"samplers": [{"input": times, "output": rot}],
              "channels": [{"sampler": 0, "target": {"node": 3, "path": "rotation"}}]}]
    return b.doc(nodes, [mesh], anims, skins)


m = convert_flag("skinned", skinned_strip())
if m:
    check(m["frames"] == 10, f"skinned: 10 frames, got {m['frames']}")
    v = m["meshes"][0]["groups"][0]["vertices_frames"]
    check(close(flat(v[0]), flat([game((0, 0, 0)), game((1, 0, 0)), game((2, 0, 0))])), "skinned: bind pose")
    # frame 5 (0.5 s): tip turned 45 degrees about z at x = 1, so (2, 0, 0) -> (1 + c, c, 0); (1, 0, 0) stays (both
    # joints agree there)
    c = math.sqrt(0.5)
    check(close(flat(v[5]), flat([game((0, 0, 0)), game((1, 0, 0)), game((1 + c, c, 0))])),
          f"skinned: frame 5, got {v[5]}")
    dummies = [d["name"] for d in m["dummies"]]
    check(dummies == ["rig", "root", "tip"], f"skinned: joints are dummies, got {dummies}")

print("FAIL" if failures else "OK", f"({len(failures)} failures)")
sys.exit(1 if failures else 0)
