#!/usr/bin/env python3
"""Convert a glTF 2.0 model (.glb, or .gltf with its .bin) to a .DKO the game draws (ROBO-239, ADR 0070; animation:
ROBO-247, ADR ROBO-247).

The art format is glTF (owner decision D2); the game keeps loading .DKO through engine/dko. This converter writes:
  - one DKO material per glTF material: name, diffuse colour (baseColorFactor), diffuse texture (the image file name
    of baseColorTexture, beside the .DKO; embedded images are written and external ones copied there),
  - one DKO mesh per glTF mesh node, one material group per triangle primitive, de-indexed (DKO stores triangle
    lists), with the node's world transform applied,
  - one DKO dummy per glTF node without a mesh (weapons look up "flash1", "flash2", ... and "eject1", ...),
  - glTF's Y-up axes as the game's Z-up: (x, y, z) -> (x, -z, y), which are Blender's axes again (Blender exports
    (x, y, z) as glTF (x, z, -y)): author in Blender as the BV2 models were built, Z up, a weapon's muzzle along +Y,
  - metres times --scale (default 200): the game draws models at 0.005, so 1 m in Blender is 1 game unit (a map tile).
Animation (ROBO-247): a model with one animation (or the one --animation names) becomes a vertex-animated DKO, as the
BV2 flags, flag pole, knives and shields are. The animation is sampled at --fps (default 10, the rate the game plays
the flags at) or into exactly --frames frames; node transforms, morph target weights and skins are all baked into the
vertices, normals and dummies of every frame. TIME_INFO holds the first frame, the last frame and the frame count.
The engine plays frame n-1 back into frame 0, so by default the last sample is one step before the end (a loop);
--no-loop adds the end pose as the last frame. Without an animation the DKO has one frame.
Sparse accessors, non-triangle primitives and animated node matrices fail (exit 1).

Usage: gltf-to-dko.py <in.glb|in.gltf> <out.DKO> [--scale S] [--fps F | --frames N] [--animation A] [--no-loop]
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
DEFAULT_FPS = 10.0  # game/src/Map.cpp plays the flags at 10 frames a second
MAX_FRAMES = 32767  # TIME_INFO holds shorts
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
        return column_major(node["matrix"])
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


def walk(doc, pose=None):
    """(node index, world matrix in game axes) for every node of the default scene, depth first, at a pose
    ({node: {path: value}}, see posed)."""
    scenes = doc.get("scenes", [])
    roots = scenes[doc.get("scene", 0)]["nodes"] if scenes else range(len(doc.get("nodes", [])))
    stack = [(i, AXES) for i in reversed(list(roots))]
    while stack:
        i, parent = stack.pop()
        node = doc["nodes"][i]
        world = mat_mul(parent, node_matrix(dict(node, **(pose or {}).get(i, {}))))
        yield i, world
        stack.extend((c, world) for c in reversed(node.get("children", [])))


def interpolate(doc, buffers, sampler, t, path):
    """A sampler's value at time t: a tuple (one per weight for "weights"), clamped outside its keys."""
    times = accessor(doc, buffers, sampler["input"])
    values = accessor(doc, buffers, sampler["output"])
    mode = sampler.get("interpolation", "LINEAR")
    cubic = mode == "CUBICSPLINE"
    per_key = len(values) // len(times) // (3 if cubic else 1)  # 1, or the morph target count for weights

    def key(k, part=1):  # part 0 in-tangent, 1 value, 2 out-tangent (cubic spline only)
        base = (k * 3 + part) * per_key if cubic else k * per_key
        vals = values[base:base + per_key]
        return tuple(x for v in vals for x in (v if isinstance(v, tuple) else (v,)))

    if t <= times[0]:
        return key(0)
    if t >= times[-1]:
        return key(len(times) - 1)
    k = max(i for i in range(len(times) - 1) if times[i] <= t)
    t0, t1 = times[k], times[k + 1]
    dt = t1 - t0
    u = (t - t0) / dt if dt > 0 else 0.0
    if mode == "STEP":
        return key(k)
    if cubic:
        p0, m0, p1, m1 = key(k), key(k, 2), key(k + 1), key(k + 1, 0)
        u2, u3 = u * u, u * u * u
        out = tuple((2 * u3 - 3 * u2 + 1) * a + (u3 - 2 * u2 + u) * dt * b + (-2 * u3 + 3 * u2) * c + (u3 - u2) * dt * d
                    for a, b, c, d in zip(p0, m0, p1, m1))
        return normalise(out) if path == "rotation" else out
    a, b = key(k), key(k + 1)
    if path == "rotation":
        return slerp(a, b, u)
    return tuple(x + (y - x) * u for x, y in zip(a, b))


def slerp(a, b, u):
    dot = sum(x * y for x, y in zip(a, b))
    if dot < 0:
        b, dot = tuple(-x for x in b), -dot
    if dot > 0.9995:
        return normalise(tuple(x + (y - x) * u for x, y in zip(a, b)))
    theta = math.acos(dot)
    sa, sb = math.sin((1 - u) * theta) / math.sin(theta), math.sin(u * theta) / math.sin(theta)
    return tuple(sa * x + sb * y for x, y in zip(a, b))


def pick_animation(doc, choice):
    """The animation to sample (None for a static model): the only one, or the one --animation names."""
    animations = doc.get("animations", [])
    if choice is not None:
        for i, a in enumerate(animations):
            if choice == a.get("name") or choice == str(i):
                return a
        raise GltfError(f"no animation {choice!r} (have {[a.get('name', str(i)) for i, a in enumerate(animations)]})")
    if len(animations) > 1:
        raise GltfError(f"{len(animations)} animations: choose one with --animation (name or index)")
    return animations[0] if animations else None


def frame_times(doc, buffers, animation, fps, frames, loop):
    """([sample time per frame], first frame number) of the animation; one frame at time None without one."""
    if animation is None:
        return [None], 0
    keys = [t for s in animation["samplers"] for t in accessor(doc, buffers, s["input"])]
    if not keys:
        raise GltfError("the animation has no keys")
    start, end = min(keys), max(keys)
    length = end - start
    if frames is None:
        steps = max(1, round(length * fps))
        frames = steps if loop else steps + 1
    if length <= 0 or frames == 1:
        return [start], round(start * fps)
    step = length / (frames if loop else frames - 1)
    return [start + i * step for i in range(frames)], round(start / step)


def posed(doc, buffers, animation, t):
    """{node index: {"translation"|"rotation"|"scale"|"weights": value}} at time t."""
    pose = {}
    if animation is None:
        return pose
    for ch in animation["channels"]:
        target = ch["target"]
        path = target.get("path")
        if path not in ("translation", "rotation", "scale", "weights"):
            raise GltfError(f"animation channel path {path!r} is not supported")
        if "node" not in target:
            continue
        if "matrix" in doc["nodes"][target["node"]] and path != "weights":
            raise GltfError(f"node {target['node']} has a matrix: animated nodes need translation, rotation, scale")
        pose.setdefault(target["node"], {})[path] = interpolate(doc, buffers, animation["samplers"][ch["sampler"]],
                                                                t, path)
    return pose


def world_matrices(doc, pose, scale):
    """{node index: world matrix in game axes and DKO units} for the default scene at a pose."""
    out = {}
    for i, world in walk(doc, pose):
        out[i] = [[x * scale for x in row[:3]] + [row[3] * scale] for row in world[:3]] + [world[3]]
    return out


def convert(doc, buffers, folder, scale=DEFAULT_SCALE, fps=DEFAULT_FPS, frames=None, animation=None, loop=True):
    """The DKO's parts: (materials, meshes, dummies, images, (first frame, frame count)), images as {file name beside
    the .DKO: bytes}. Each group's vertices and normals and each dummy's position and matrix are lists, one per frame."""
    unsupported(doc)
    materials, images = [], {}
    for i, m in enumerate(doc.get("materials", [])):
        pbr = m.get("pbrMetallicRoughness", {})
        colour = list(pbr.get("baseColorFactor", (1, 1, 1, 1)))
        texture = None
        if "baseColorTexture" in pbr:
            image_index = doc["textures"][pbr["baseColorTexture"]["index"]].get("source")
            if image_index is not None:
                texture, images[texture] = image_file(doc, buffers, folder, image_index)
        materials.append({"name": m.get("name") or f"material{i}", "diffuse": colour, "texture": texture,
                          "two_sided": bool(m.get("doubleSided"))})
    anim = pick_animation(doc, animation)
    times, first = frame_times(doc, buffers, anim, fps, frames, loop)
    poses = [posed(doc, buffers, anim, t) for t in times]
    worlds = [world_matrices(doc, p, scale) for p in poses]
    default_material = None
    meshes, dummies = [], []
    for i, _ in walk(doc, {}):
        node = doc["nodes"][i]
        name = node.get("name") or f"node{i}"
        if "mesh" not in node:
            pos, mats = [], []
            for w in worlds:
                pos.append(apply(w[i], (0, 0, 0), 1))
                axes = [normalise(apply(w[i], a, 0)) for a in ((1, 0, 0), (0, 1, 0), (0, 0, 1))]
                mats.append([x for a in axes for x in a])
            dummies.append({"name": name, "position": pos, "matrix": mats})
            continue
        mesh = doc["meshes"][node["mesh"]]
        skin = doc["skins"][node["skin"]] if "skin" in node else None
        groups = []
        for p, prim in enumerate(mesh["primitives"]):
            if prim.get("mode", TRIANGLES) != TRIANGLES:
                raise GltfError(f"node {name} primitive {p}: only triangle lists are supported")
            attrs = prim["attributes"]
            positions = accessor(doc, buffers, attrs["POSITION"])
            normals = accessor(doc, buffers, attrs["NORMAL"]) if "NORMAL" in attrs else None
            uvs = accessor(doc, buffers, attrs["TEXCOORD_0"]) if "TEXCOORD_0" in attrs else None
            order = accessor(doc, buffers, prim["indices"]) if "indices" in prim else range(len(positions))
            if len(order) % 3:
                raise GltfError(f"node {name} primitive {p}: index count is not a multiple of 3")
            targets = [(accessor(doc, buffers, tg["POSITION"]) if "POSITION" in tg else None,
                        accessor(doc, buffers, tg["NORMAL"]) if "NORMAL" in tg else None)
                       for tg in prim.get("targets", [])]
            if skin is not None:
                if "JOINTS_0" not in attrs or "WEIGHTS_0" not in attrs:
                    raise GltfError(f"node {name} primitive {p}: a skinned mesh needs JOINTS_0 and WEIGHTS_0")
                if "JOINTS_1" in attrs:
                    raise GltfError(f"node {name} primitive {p}: more than 4 joints per vertex are not supported")
                joints = accessor(doc, buffers, attrs["JOINTS_0"])
                weights = accessor(doc, buffers, attrs["WEIGHTS_0"])
            mat = prim.get("material")
            if mat is None:
                if default_material is None:
                    default_material = len(materials)
                    materials.append({"name": "default", "diffuse": [1, 1, 1, 1], "texture": None,
                                      "two_sided": False})
                mat = default_material
            coords = []
            for k in order:
                u, v = uvs[k] if uvs else (0.0, 0.0)
                coords.append((u, 1.0 - v))  # glTF v runs down the image, the game's TGA rows run up
            frame_verts, frame_norms = [], []
            for pose, w in zip(poses, worlds):
                morph = pose.get(i, {}).get("weights") or tuple(node.get("weights") or mesh.get("weights")
                                                               or [0.0] * len(targets))
                if skin is not None:
                    inverse = (accessor(doc, buffers, skin["inverseBindMatrices"])
                               if "inverseBindMatrices" in skin else None)
                    joint_mats = [mat_mul(w[j], column_major(inverse[n]) if inverse else IDENTITY)
                                  for n, j in enumerate(skin["joints"])]
                verts, norms = [], []
                for k in order:
                    pos, nrm = positions[k], normals[k] if normals else None
                    if targets and any(morph):
                        pos = tuple(pos[c] + sum(wt * tp[k][c] for wt, (tp, _) in zip(morph, targets) if tp)
                                    for c in range(3))
                        if nrm:
                            nrm = tuple(nrm[c] + sum(wt * tn[k][c] for wt, (_, tn) in zip(morph, targets) if tn)
                                        for c in range(3))
                    if skin is not None:
                        m = blend([joint_mats[j] for j in joints[k]], weights[k])
                    else:
                        m = w[i]
                    verts.append(apply(m, pos, 1))
                    # the matrix, not its inverse transpose: normals tilt under non-uniform node scale
                    norms.append(normalise(apply(m, nrm, 0)) if nrm else (0.0, 0.0, 1.0))
                frame_verts.append(verts)
                frame_norms.append(norms)
            groups.append({"material": mat, "vertices": frame_verts, "normals": frame_norms, "uvs": coords})
        meshes.append({"name": name, "groups": groups})
    if not meshes:
        raise GltfError("the scene has no mesh")
    if len(dummies) > MAX_DUMMIES:
        raise GltfError(f"{len(dummies)} nodes without a mesh, engine/dko keeps at most {MAX_DUMMIES} dummies")
    if len(times) > MAX_FRAMES:
        raise GltfError(f"{len(times)} frames, a DKO holds at most {MAX_FRAMES}")
    return materials, meshes, dummies, images, (first, len(times))


IDENTITY = [[1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1]]


def column_major(m):
    return [[m[c * 4 + r] for c in range(4)] for r in range(4)]


def blend(mats, weights):
    """The weighted sum of joint matrices (linear blend skinning)."""
    total = sum(weights) or 1.0
    return [[sum(wt * m[r][c] for m, wt in zip(mats, weights)) / total for c in range(4)] for r in range(4)]


def unsupported(doc):
    """Raises GltfError for what a .DKO can't hold, instead of writing a file that looks valid."""
    for i, s in enumerate(doc.get("skins", [])):
        if not s.get("joints"):
            raise GltfError(f"skin {i} has no joints")


def image_file(doc, buffers, folder, index):
    """(file name beside the .DKO, bytes) of image index: engine/dko opens textures next to the model."""
    image = doc["images"][index]
    uri = image.get("uri")
    if uri and not uri.startswith("data:"):
        with open(os.path.join(folder, uri), "rb") as f:
            return os.path.basename(uri), f.read()
    if uri:
        head, payload = uri.split(",", 1)
        data, mime = base64.b64decode(payload), head[5:].split(";")[0]
    else:
        view = doc["bufferViews"][image["bufferView"]]
        start = view.get("byteOffset", 0)
        data, mime = buffers[view["buffer"]][start:start + view["byteLength"]], image.get("mimeType", "")
    ext = {"image/png": ".png", "image/jpeg": ".jpg"}.get(mime, ".png")
    name = (image.get("name") or f"image{index}") + ext
    return name, data


def chunk(name):
    return struct.pack("<h", CHUNK[name])


def string(s):
    return s.encode("latin-1", "replace") + b"\0"


def floats(values):
    return struct.pack(f"<{len(values)}f", *values)


def write_dko(materials, meshes, dummies, name="model", timing=(0, 1)):
    """Bytes of a DKO v2 in engine/dko's chunk layout; timing is (first frame, frame count)."""
    first, count = timing
    out = chunk("VERSION") + struct.pack("<h", DKO_VERSION)
    out += chunk("TIME_INFO") + struct.pack("<hhh", first, first + count - 1, count)
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
            out += chunk("NB_VERTEX") + struct.pack("<i", len(g["uvs"]))
            # engine/dko reads TIME_INFO's count of arrays back to back (CdkoMesh::loadMatGroup)
            out += chunk("VERTEX_ARRAY") + floats([x for f in g["vertices"] for v in f for x in v])
            out += chunk("NORMAL_ARRAY") + floats([x for f in g["normals"] for v in f for x in v])
            out += chunk("TEXCOORD_ARRAY") + floats([x for v in g["uvs"] for x in v])
            out += chunk("END")
        out += chunk("END")
    for d in dummies:
        out += chunk("DUMMY") + chunk("NAME") + string(d["name"])
        out += chunk("POSITION") + floats([x for p in d["position"] for x in p])
        out += chunk("MATRIX") + floats([x for m in d["matrix"] for x in m]) + chunk("END")
    return out + chunk("END")


def main(argv):
    parser = argparse.ArgumentParser(prog="gltf-to-dko.py", description="Convert a glTF 2.0 model to a .DKO.")
    parser.add_argument("src", help="input .glb, or .gltf with its buffers")
    parser.add_argument("dst", help="output .DKO; embedded images are written and external ones copied beside it")
    parser.add_argument("--scale", type=float, default=DEFAULT_SCALE, help="DKO units per metre (default 200)")
    rate = parser.add_mutually_exclusive_group()
    rate.add_argument("--fps", type=float, default=DEFAULT_FPS, help="frames per second of animation (default 10)")
    rate.add_argument("--frames", type=int, help="sample the whole animation into exactly this many frames")
    parser.add_argument("--animation", help="name or index of the animation, when the model has more than one")
    parser.add_argument("--no-loop", action="store_true", help="keep the end pose as the last frame (no wrap)")
    args = parser.parse_args(argv)
    src, dst = args.src, args.dst
    if args.fps <= 0 or (args.frames is not None and not 1 <= args.frames <= MAX_FRAMES):
        print(f"gltf-to-dko: --fps must be above 0 and --frames 1 to {MAX_FRAMES}")
        return 1
    try:
        doc, buffers, folder = load(src)
        materials, meshes, dummies, images, timing = convert(doc, buffers, folder, args.scale, args.fps,
                                                             args.frames, args.animation, not args.no_loop)
        out_dir = os.path.dirname(os.path.abspath(dst))
        for name, data in images.items():
            with open(os.path.join(out_dir, name), "wb") as f:
                f.write(data)
        with open(dst, "wb") as f:
            f.write(write_dko(materials, meshes, dummies, os.path.splitext(os.path.basename(dst))[0], timing))
    except (GltfError, KeyError, IndexError, ValueError, OSError, struct.error) as e:
        print(f"gltf-to-dko: {src}: {e}")
        return 1
    n = sum(len(g["uvs"]) for m in meshes for g in m["groups"])
    print(f"{dst}: {len(meshes)} meshes, {len(materials)} materials, {n // 3} triangles, {len(dummies)} dummies, "
          f"{timing[1]} frames")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
