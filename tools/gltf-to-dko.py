#!/usr/bin/env python3
"""Convert a glTF 2.0 model (.glb, or .gltf with its .bin) to a static .DKO the game draws (ROBO-239, ADR 0068).

The art format is glTF (owner decision D2); the game keeps loading .DKO through engine/dko. This converter writes:
  - one DKO material per glTF material: name, diffuse colour (baseColorFactor), diffuse texture (the image file name
    of baseColorTexture, beside the .DKO; embedded images are written there too),
  - one DKO mesh per glTF mesh node, one material group per triangle primitive, de-indexed (DKO stores triangle
    lists), with the node's world transform applied,
  - one DKO dummy per glTF node without a mesh (weapons look up "flash1", "flash2", ... and "eject1", ...),
  - glTF's Y-up axes as the game's Z-up: (x, y, z) -> (x, -z, y), which are Blender's axes again (Blender exports
    (x, y, z) as glTF (x, z, -y)): author in Blender as the BV2 models were built, Z up, a weapon's muzzle along +Y,
  - metres times --scale (default 200): the game draws models at 0.005, so 1 m in Blender is 1 game unit (a map tile).
Static only: no animation (one frame), no skins, no morph targets, no sparse accessors.

Usage: gltf-to-dko.py <in.glb|in.gltf> <out.DKO> [--scale S]  (S DKO units per metre, default 200)
Python 3, stdlib only.
"""
import argparse
import base64
import json
import math
import os
import struct
import sys

GLB_MAGIC, GLB_JSON, GLB_BIN = 0x46546C67, 0x4E4F534A, 0x004E4942

# engine/dko/src/CdkoModel.h
CHUNK = {
    "VERSION": 0x0000, "TIME_INFO": 0x0001, "PROPERTIES": 0x0100, "NAME": 0x0110, "POSITION": 0x0120,
    "MATRIX": 0x0130, "MATLIST": 0x0200, "MATNAME": 0x0210, "AMBIENT": 0x0230, "DIFFUSE": 0x0240,
    "SPECULAR": 0x0250, "EMISSIVE": 0x0260, "SHININESS": 0x0270, "TRANSPARENCY": 0x0280, "TWO_SIDED": 0x0290,
    "TEX_DIFFUSE": 0x02C0, "TRI_MESH": 0x0300, "NB_MAT_GROUP": 0x0340, "MAT_ID": 0x0341, "NB_VERTEX": 0x0342,
    "VERTEX_ARRAY": 0x0343, "NORMAL_ARRAY": 0x0344, "TEXCOORD_ARRAY": 0x0345, "DUMMY": 0x0400, "END": 0x0900,
}
DKO_VERSION = 2
MAX_DUMMIES = 64  # CDkoModel::dummies
DEFAULT_SCALE = 200.0  # DKO units per metre: the game draws models at 0.005
COMPONENTS = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4, "MAT4": 16}
FORMATS = {5120: "b", 5121: "B", 5122: "h", 5123: "H", 5125: "I", 5126: "f"}
TRIANGLES = 4


class GltfError(Exception):
    pass


def load(path):
    """(gltf json, [buffer bytes], folder of the file)."""
    folder = os.path.dirname(os.path.abspath(path))
    data = open(path, "rb").read()
    glb_bin = None
    if data[:4] == struct.pack("<I", GLB_MAGIC):
        _, version, length = struct.unpack_from("<III", data, 0)
        if version != 2:
            raise GltfError(f"GLB version {version}, need 2")
        pos, doc = 12, None
        while pos + 8 <= min(length, len(data)):
            size, kind = struct.unpack_from("<II", data, pos)
            chunk = data[pos + 8:pos + 8 + size]
            if kind == GLB_JSON:
                doc = json.loads(chunk.decode("utf-8"))
            elif kind == GLB_BIN and glb_bin is None:
                glb_bin = chunk
            pos += 8 + size
        if doc is None:
            raise GltfError("GLB has no JSON chunk")
    else:
        doc = json.loads(data.decode("utf-8"))
    if not str(doc.get("asset", {}).get("version", "")).startswith("2"):
        raise GltfError("not glTF 2.0")
    buffers = []
    for i, b in enumerate(doc.get("buffers", [])):
        uri = b.get("uri")
        if uri is None:
            if glb_bin is None:
                raise GltfError(f"buffer {i} has no uri and the file has no GLB binary chunk")
            buffers.append(glb_bin)
        elif uri.startswith("data:"):
            buffers.append(base64.b64decode(uri.split(",", 1)[1]))
        else:
            buffers.append(open(os.path.join(folder, uri), "rb").read())
    return doc, buffers, folder


def accessor(doc, buffers, index):
    """List of tuples (or numbers for SCALAR) of an accessor, normalised integers as floats in 0..1."""
    acc = doc["accessors"][index]
    if "sparse" in acc:
        raise GltfError(f"accessor {index}: sparse accessors are not supported")
    n, comps = acc["count"], COMPONENTS[acc["type"]]
    fmt = FORMATS[acc["componentType"]]
    if "bufferView" not in acc:
        return [(0.0,) * comps if comps > 1 else 0] * n
    view = doc["bufferViews"][acc["bufferView"]]
    buf = buffers[view["buffer"]]
    elem = struct.calcsize("<" + fmt * comps)
    stride = view.get("byteStride") or elem
    start = view.get("byteOffset", 0) + acc.get("byteOffset", 0)
    if start + stride * (n - 1) + elem > len(buf):
        raise GltfError(f"accessor {index} runs past its buffer")
    scale = {"B": 255.0, "H": 65535.0, "b": 127.0, "h": 32767.0}.get(fmt) if acc.get("normalized") else None
    out = []
    for i in range(n):
        v = struct.unpack_from("<" + fmt * comps, buf, start + i * stride)
        if scale:
            v = tuple(max(-1.0, x / scale) for x in v)
        out.append(v if comps > 1 else v[0])
    return out


def mat_mul(a, b):
    return [[sum(a[i][k] * b[k][j] for k in range(4)) for j in range(4)] for i in range(4)]


def node_matrix(node):
    if "matrix" in node:
        m = node["matrix"]  # column-major
        return [[m[c * 4 + r] for c in range(4)] for r in range(4)]
    tx, ty, tz = node.get("translation", (0, 0, 0))
    qx, qy, qz, qw = node.get("rotation", (0, 0, 0, 1))
    sx, sy, sz = node.get("scale", (1, 1, 1))
    r = [[1 - 2 * (qy * qy + qz * qz), 2 * (qx * qy - qz * qw), 2 * (qx * qz + qy * qw)],
         [2 * (qx * qy + qz * qw), 1 - 2 * (qx * qx + qz * qz), 2 * (qy * qz - qx * qw)],
         [2 * (qx * qz - qy * qw), 2 * (qy * qz + qx * qw), 1 - 2 * (qx * qx + qy * qy)]]
    return [[r[0][0] * sx, r[0][1] * sy, r[0][2] * sz, tx],
            [r[1][0] * sx, r[1][1] * sy, r[1][2] * sz, ty],
            [r[2][0] * sx, r[2][1] * sy, r[2][2] * sz, tz],
            [0, 0, 0, 1]]


# Y-up to Z-up: (x, y, z) -> (x, -z, y)
AXES = [[1, 0, 0, 0], [0, 0, -1, 0], [0, 1, 0, 0], [0, 0, 0, 1]]


def apply(m, v, w):
    return tuple(m[i][0] * v[0] + m[i][1] * v[1] + m[i][2] * v[2] + m[i][3] * w for i in range(3))


def normalise(v):
    length = math.sqrt(sum(x * x for x in v)) or 1.0
    return tuple(x / length for x in v)


def walk(doc):
    """(node index, world matrix in game axes) for every node of the default scene, depth first."""
    scenes = doc.get("scenes", [])
    roots = scenes[doc.get("scene", 0)]["nodes"] if scenes else range(len(doc.get("nodes", [])))
    stack = [(i, AXES) for i in reversed(list(roots))]
    while stack:
        i, parent = stack.pop()
        node = doc["nodes"][i]
        world = mat_mul(parent, node_matrix(node))
        yield i, world
        stack.extend((c, world) for c in reversed(node.get("children", [])))


def convert(doc, buffers, folder, out_dir, scale=DEFAULT_SCALE):
    """The DKO's parts: (materials, meshes, dummies); writes embedded images into out_dir."""
    materials = []
    for i, m in enumerate(doc.get("materials", [])):
        pbr = m.get("pbrMetallicRoughness", {})
        colour = list(pbr.get("baseColorFactor", (1, 1, 1, 1)))
        texture = None
        if "baseColorTexture" in pbr:
            image_index = doc["textures"][pbr["baseColorTexture"]["index"]].get("source")
            if image_index is not None:
                texture = image_file(doc, buffers, folder, out_dir, image_index)
        materials.append({"name": m.get("name") or f"material{i}", "diffuse": colour, "texture": texture,
                          "two_sided": bool(m.get("doubleSided"))})
    default_material = None
    meshes, dummies = [], []
    for i, world in walk(doc):
        node = doc["nodes"][i]
        name = node.get("name") or f"node{i}"
        world = [[x * scale for x in row[:3]] + [row[3] * scale] for row in world[:3]] + [world[3]]
        if "mesh" not in node:
            pos = apply(world, (0, 0, 0), 1)
            axes = [normalise(apply(world, a, 0)) for a in ((1, 0, 0), (0, 1, 0), (0, 0, 1))]
            dummies.append({"name": name, "position": pos, "matrix": [x for a in axes for x in a]})
            continue
        groups = []
        for p, prim in enumerate(doc["meshes"][node["mesh"]]["primitives"]):
            if prim.get("mode", TRIANGLES) != TRIANGLES:
                raise GltfError(f"node {name} primitive {p}: only triangle lists are supported")
            attrs = prim["attributes"]
            positions = accessor(doc, buffers, attrs["POSITION"])
            normals = accessor(doc, buffers, attrs["NORMAL"]) if "NORMAL" in attrs else None
            uvs = accessor(doc, buffers, attrs["TEXCOORD_0"]) if "TEXCOORD_0" in attrs else None
            order = accessor(doc, buffers, prim["indices"]) if "indices" in prim else range(len(positions))
            if len(order) % 3:
                raise GltfError(f"node {name} primitive {p}: index count is not a multiple of 3")
            mat = prim.get("material")
            if mat is None:
                if default_material is None:
                    default_material = len(materials)
                    materials.append({"name": "default", "diffuse": [1, 1, 1, 1], "texture": None,
                                      "two_sided": False})
                mat = default_material
            verts, norms, coords = [], [], []
            for k in order:
                verts.append(apply(world, positions[k], 1))
                norms.append(normalise(apply(world, normals[k], 0)) if normals else (0.0, 0.0, 1.0))
                u, v = uvs[k] if uvs else (0.0, 0.0)
                coords.append((u, 1.0 - v))  # glTF v runs down the image, the game's TGA rows run up
            groups.append({"material": mat, "vertices": verts, "normals": norms, "uvs": coords})
        meshes.append({"name": name, "groups": groups})
    if not meshes:
        raise GltfError("the scene has no mesh")
    if len(dummies) > MAX_DUMMIES:
        raise GltfError(f"{len(dummies)} nodes without a mesh, engine/dko keeps at most {MAX_DUMMIES} dummies")
    return materials, meshes, dummies


def image_file(doc, buffers, folder, out_dir, index):
    """File name (beside the .DKO) of image index; embedded images are written out."""
    image = doc["images"][index]
    uri = image.get("uri")
    if uri and not uri.startswith("data:"):
        return os.path.basename(uri)
    if uri:
        head, payload = uri.split(",", 1)
        data, mime = base64.b64decode(payload), head[5:].split(";")[0]
    else:
        view = doc["bufferViews"][image["bufferView"]]
        start = view.get("byteOffset", 0)
        data, mime = buffers[view["buffer"]][start:start + view["byteLength"]], image.get("mimeType", "")
    ext = {"image/png": ".png", "image/jpeg": ".jpg"}.get(mime, ".png")
    name = (image.get("name") or f"image{index}") + ext
    with open(os.path.join(out_dir, name), "wb") as f:
        f.write(data)
    return name


def chunk(name):
    return struct.pack("<h", CHUNK[name])


def string(s):
    return s.encode("latin-1", "replace") + b"\0"


def floats(values):
    return struct.pack(f"<{len(values)}f", *values)


def write_dko(materials, meshes, dummies, name="model"):
    """Bytes of a one-frame DKO v2 in engine/dko's chunk layout."""
    out = chunk("VERSION") + struct.pack("<h", DKO_VERSION)
    out += chunk("TIME_INFO") + struct.pack("<hhh", 0, 0, 1)
    identity = [1, 0, 0, 0, 1, 0, 0, 0, 1]
    out += chunk("PROPERTIES") + chunk("NAME") + string(name) + chunk("POSITION") + floats([0, 0, 0])
    out += chunk("MATRIX") + floats(identity) + chunk("END")
    out += chunk("MATLIST") + struct.pack("<h", len(materials))
    for m in reversed(materials):  # CDkoModel::loadFromFile fills materialArray from the end
        r, g, b, a = m["diffuse"]
        out += chunk("MATNAME") + string(m["name"])
        out += chunk("AMBIENT") + floats([r * 0.5, g * 0.5, b * 0.5, 1])
        out += chunk("DIFFUSE") + floats([r, g, b, 1])
        out += chunk("SPECULAR") + floats([0, 0, 0, 1]) + chunk("EMISSIVE") + floats([0, 0, 0, 1])
        out += chunk("SHININESS") + struct.pack("<h", 0)
        out += chunk("TRANSPARENCY") + floats([1.0 - a])
        out += chunk("TWO_SIDED") + struct.pack("<B", 1 if m["two_sided"] else 0)
        if m["texture"]:
            out += chunk("TEX_DIFFUSE") + string(m["texture"])
        out += chunk("END")
    for mesh in meshes:
        out += chunk("TRI_MESH") + chunk("NAME") + string(mesh["name"])
        out += chunk("POSITION") + floats([0, 0, 0]) + chunk("MATRIX") + floats(identity)
        out += chunk("NB_MAT_GROUP") + struct.pack("<h", len(mesh["groups"]))
        for g in mesh["groups"]:
            out += chunk("MAT_ID") + struct.pack("<h", g["material"])
            out += chunk("NB_VERTEX") + struct.pack("<i", len(g["vertices"]))
            out += chunk("VERTEX_ARRAY") + floats([x for v in g["vertices"] for x in v])
            out += chunk("NORMAL_ARRAY") + floats([x for v in g["normals"] for x in v])
            out += chunk("TEXCOORD_ARRAY") + floats([x for v in g["uvs"] for x in v])
            out += chunk("END")
        out += chunk("END")
    for d in dummies:
        out += chunk("DUMMY") + chunk("NAME") + string(d["name"])
        out += chunk("POSITION") + floats(list(d["position"])) + chunk("MATRIX") + floats(d["matrix"]) + chunk("END")
    return out + chunk("END")


def main(argv):
    parser = argparse.ArgumentParser(prog="gltf-to-dko.py", description="Convert a glTF 2.0 model to a static .DKO.")
    parser.add_argument("src", help="input .glb, or .gltf with its buffers")
    parser.add_argument("dst", help="output .DKO; embedded images are written beside it")
    parser.add_argument("--scale", type=float, default=DEFAULT_SCALE, help="DKO units per metre (default 200)")
    args = parser.parse_args(argv)
    src, dst, scale = args.src, args.dst, args.scale
    try:
        doc, buffers, folder = load(src)
        out_dir = os.path.dirname(os.path.abspath(dst))
        materials, meshes, dummies = convert(doc, buffers, folder, out_dir, scale)
    except (GltfError, KeyError, IndexError, ValueError, OSError, struct.error) as e:
        print(f"gltf-to-dko: {src}: {e}")
        return 1
    with open(dst, "wb") as f:
        f.write(write_dko(materials, meshes, dummies, os.path.splitext(os.path.basename(dst))[0]))
    n = sum(len(g["vertices"]) for m in meshes for g in m["groups"])
    print(f"{dst}: {len(meshes)} meshes, {len(materials)} materials, {n // 3} triangles, {len(dummies)} dummies")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
