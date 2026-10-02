#!/usr/bin/env python3
"""Inventory the original BaboViolent 2 assets that Step 0.4 removes.

Writes docs/assets/ASSET-INVENTORY.md and docs/assets/original-assets.sha256.
Run it BEFORE the removal commit: it reads the files from the working tree.
Binaries untracked in step0.1 (DLLs) are read from git history (DLL_REV).
Python 3, stdlib only.
"""
import hashlib
import os
import re
import struct
import subprocess
import sys
from collections import defaultdict

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT_MD = "docs/assets/ASSET-INVENTORY.md"
OUT_SHA = "docs/assets/original-assets.sha256"
DLL_REV = "ca42425^"
DLLS = ["fmod.dll", "fmod64.dll", "libcurl.dll", "sqlite3.dll"]
C = "BaboViolent2/Content/"
M = C + "main/"

# (folder, matcher) in report order
FOLDERS = [
    (M + "maps", "bvm"),
    (M + "models", None),
    (M + "skins", None),
    (M + "textures", None),
    (M + "fonts", None),
    (M + "sounds", None),
    (M + "languages", None),
    ("BaboViolent2/Assets", None),
    ("BaboViolent2/Design", None),
    (C + "[dll]", None),
    (M + "[legal]", None),
]
LEGAL = {M + "License.txt", M + "BV2-EULALegalForm.doc"}
CODE_EXT = (".cpp", ".h", ".c", ".hpp", ".inl", ".cc")
SRC_DIRS = ["BaboViolent2/Code", "Engine"]
TEXT_DIRS = [M + "LaunchScript", M + "languages"]


def git(*args):
    return subprocess.run(["git", *args], cwd=ROOT, check=True,
                          capture_output=True).stdout


def removed_paths():
    out = []
    for p in git("ls-files").decode().splitlines():
        low = p.lower()
        if p in LEGAL:
            out.append(p)
        elif p.startswith(M) and p[len(M):].split("/")[0] in (
                "maps", "models", "skins", "textures", "fonts", "sounds"):
            out.append(p)
        elif p.startswith(M + "languages/") and not p.endswith("/en.lang"):
            out.append(p)
        elif p.startswith(("BaboViolent2/Assets/", "BaboViolent2/Design/")):
            out.append(p)
    for d in DLLS:
        out.append(C + d)
    return sorted(set(out))


def read(path):
    full = os.path.join(ROOT, path)
    if os.path.exists(full):
        with open(full, "rb") as f:
            return f.read()
    return git("show", f"{DLL_REV}:{path}")


# ---------------------------------------------------------------- formats
def tga_info(b):
    if len(b) < 18:
        return None
    w, h, bpp = struct.unpack_from("<HHB", b, 12)
    return w, h, bpp


def wav_info(b):
    if b[:4] != b"RIFF" or b[8:12] != b"WAVE":
        return None
    pos, fmt, data = 12, None, 0
    while pos + 8 <= len(b):
        cid, size = b[pos:pos + 4], struct.unpack_from("<I", b, pos + 4)[0]
        if cid == b"fmt ":
            fmt = struct.unpack_from("<HHIIHH", b, pos + 8)
        elif cid == b"data":
            data = size
        pos += 8 + size + (size & 1)
    if not fmt:
        return None
    _, ch, rate, byterate, _, bits = fmt
    return ch, rate, bits, (data / byterate if byterate else 0)


def ogg_info(b):
    i = b.find(b"\x01vorbis")
    if i < 0:
        return None
    ch, rate = struct.unpack_from("<BI", b, i + 11)
    j = b.rfind(b"OggS")
    gran = struct.unpack_from("<q", b, j + 6)[0] if j >= 0 else 0
    return ch, rate, gran / rate if rate else 0


def bvm_info(b):
    ver = struct.unpack_from("<I", b, 0)[0]
    pos = 4
    author = ""
    if ver == 20202:
        author = b[pos:pos + 25].split(b"\0")[0].decode("latin-1")
        pos += 25
    if ver in (20201, 20202):
        pos += 4  # theme, weather: FileIO::getInt reads int16
    w, h = struct.unpack_from("<hh", b, pos)
    return ver, w, h, author


def dko_info(b):
    ver = struct.unpack_from("<H", b, 2)[0] if len(b) > 4 else 0
    nmat = struct.unpack_from("<H", b, 14)[0] if len(b) > 16 else 0
    meshes = len(re.findall(rb"\x00\x03\x10\x01", b))
    texs = sorted({m.decode("latin-1") for m in re.findall(
        rb"[\w ./\\-]+\.(?:tga|TGA|bmp|jpg|png)", b)})
    return ver, nmat, meshes, texs


def describe(path, b):
    low = path.lower()
    try:
        if low.endswith(".tga"):
            w, h, bpp = tga_info(b)
            return f"TGA {w}×{h}×{bpp}", {"tga": (w, h, bpp)}
        if low.endswith(".wav"):
            ch, rate, bits, dur = wav_info(b)
            return f"WAV {ch}ch {rate} Hz {bits}-bit {dur:.2f} s", {"wav": (ch, rate, bits)}
        if low.endswith(".ogg"):
            ch, rate, dur = ogg_info(b)
            return f"OGG {ch}ch {rate} Hz {dur:.1f} s", {"ogg": (ch, rate)}
        if low.endswith(".bvm"):
            ver, w, h, _ = bvm_info(b)
            return f"BVM v{ver} {w}×{h}", {"bvm": (ver, w, h)}
        if low.endswith(".dko"):
            ver, nmat, meshes, texs = dko_info(b)
            t = f", textures: {', '.join(texs)}" if texs else ""
            return f"DKO v{ver}, {meshes} mesh, {nmat} material{t}", {"dko": ver}
    except Exception as e:  # malformed header: report, don't abort
        return f"{low.rsplit('.', 1)[-1].upper()} (unparsed: {e})", {}
    ext = low.rsplit(".", 1)[-1].upper()
    return f"{ext}, {len(b)} bytes", {}


def label(folder):
    return {C + "[dll]": "BaboViolent2/Content/*.dll", M + "[legal]": "main/License.txt, main/BV2-EULALegalForm.doc"}.get(folder, folder.replace(C, ""))


# ---------------------------------------------------------------- references
def load_sources():
    lines = []  # (relpath, lineno, text)
    for d in SRC_DIRS:
        for dp, _, fns in os.walk(os.path.join(ROOT, d)):
            for fn in fns:
                if fn.lower().endswith(CODE_EXT):
                    rp = os.path.relpath(os.path.join(dp, fn), ROOT)
                    with open(os.path.join(ROOT, rp), "rb") as f:
                        for n, t in enumerate(f.read().decode("latin-1").splitlines(), 1):
                            lines.append((rp, n, t))
    for d in TEXT_DIRS:
        for fn in sorted(os.listdir(os.path.join(ROOT, d))):
            if fn.endswith((".cfg", ".lang")):
                rp = f"{d}/{fn}"
                with open(os.path.join(ROOT, rp), "rb") as f:
                    for n, t in enumerate(f.read().decode("latin-1").splitlines(), 1):
                        lines.append((rp, n, t))
    return lines


ASSIGN = re.compile(r"([A-Za-z_][\w\.\->\[\]]*)\s*=\s*(?:\(\w+\))?\s*(?:dk\w+|\w+)\s*\(")


def find_refs(path, lines):
    folder = path.split("/main/", 1)[-1] if "/main/" in path else ""
    sub = folder.split("/", 1)[-1] if "/" in folder else os.path.basename(path)
    base = os.path.basename(path)
    stem, ext = os.path.splitext(base)
    needles = {sub.lower(), base.lower()}
    if sub != base:
        needles.add(sub.lower())
    hits = []
    for rp, n, t in lines:
        tl = t.lower()
        if not any(nd in tl for nd in needles):
            continue
        # a bare basename must appear as a whole token (avoid "gib.tga" in "xgib.tga")
        if not any(re.search(r"(?<![\w])" + re.escape(nd), tl) for nd in needles):
            continue
        hits.append((rp, n, t))
    return hits


WEAPON = re.compile(r"weapons\[(\w+)\]\s*=\s*new Weapon\(\s*\"([^\"]*)\"\s*,\s*\"([^\"]*)\"")


def purpose_from(path, hits, embedded):
    sub = path.split("/main/", 1)[-1]
    if sub.startswith("textures/themes/"):
        theme, name = sub.split("/")[2:4]
        return f"`{os.path.splitext(name)[0]}` of map theme `{theme}` (`Map.cpp:1006-1026`)"
    for rp, n, t in hits:
        m = WEAPON.search(t)
        if m:
            kind = "world/view model" if m.group(2).lower().endswith("dko") and path.lower().endswith("dko") else "fire sound"
            return f"{kind} of `{m.group(1)}` (`{os.path.basename(rp)}:{n}`)"
    for rp, n, t in hits:
        if rp.startswith(M):
            continue
        m = ASSIGN.search(t)
        if m and not t.lstrip().startswith("//"):
            return f"`{m.group(1)}` in `{os.path.basename(rp)}`"
    for rp, n, t in hits:
        if not rp.startswith(M) and not t.lstrip().startswith("//"):
            return f"loaded in `{os.path.basename(rp)}:{n}`"
    if embedded:
        return "texture of model " + ", ".join(sorted(embedded))
    return "TODO"


# ---------------------------------------------------------------- main
def main():
    paths = removed_paths()
    lines = load_sources()
    rows, stats = [], defaultdict(lambda: defaultdict(set))
    shas = []
    blobs = {}
    for p in paths:
        b = read(p)
        blobs[p] = b
        shas.append(f"{hashlib.sha256(b).hexdigest()}  {p}")

    dko_tex = defaultdict(set)  # texture basename(lower) -> models
    for p in paths:
        if p.lower().endswith(".dko"):
            for t in dko_info(blobs[p])[3]:
                dko_tex[os.path.basename(t.replace("\\", "/")).lower()].add(os.path.basename(p))

    for p in paths:
        b = blobs[p]
        low = p.lower()
        typ, meta = describe(p, b)
        hits = find_refs(p, lines)
        live = [h for h in hits if not h[2].lstrip().startswith("//")]
        dead = [h for h in hits if h[2].lstrip().startswith("//")]
        code_hits = [h for h in live if not h[0].startswith(M)]
        cfg_hits = [h for h in live if h[0].startswith(M)]
        emb = dko_tex.get(os.path.basename(p).lower(), set())
        used = [f"{rp}:{n}" for rp, n, _ in code_hits[:4]]
        if len(code_hits) > 4:
            used.append(f"+{len(code_hits) - 4} more")
        used += [f"{rp.replace(M, '')}:{n}" for rp, n, _ in cfg_hits[:2]]
        if dead and not used and not emb:
            used.append("commented out: " + ", ".join(f"{rp}:{n}" for rp, n, _ in dead[:2]))
        if emb:
            used.append("model " + ", ".join(sorted(emb)))
        folder = next(f for f, _ in FOLDERS if p.startswith(f.split("[")[0]) and
                      (("[" not in f) or (f.endswith("[dll]") and low.endswith(".dll"))
                       or (f.endswith("[legal]") and p in LEGAL)))
        sub = p[len(folder) + 1:] if "[" not in folder else p
        # runtime-selected content
        if folder == M + "maps":
            rc, purpose = "content", "map chosen at runtime (`Map.cpp:158`, `CHost.cpp:46`)"
        elif folder == M + "skins":
            rc, purpose = "content", "player skin chosen at runtime by name (`Player.cpp:172`, `Extended.cpp:101`)"
        elif folder == M + "languages":
            rc, purpose = "content", "translation of `en.lang` (`OptionMenu.cpp:163`)"
        elif folder == "BaboViolent2/Assets":
            rc = "source" if low.endswith((".max", ".jpg")) else "drop"
            purpose = "3ds Max source scene / reference art" if low.endswith((".max", ".jpg")) \
                else "3ds Max DKO exporter plugin (3rd-party SDK build)"
        elif folder == "BaboViolent2/Design":
            rc, purpose = "source", "design mock-up"
        elif low.endswith(".dll"):
            rc, purpose = "drop", "third-party runtime library (replaced by vcpkg in Step 3)"
        elif p in LEGAL:
            rc, purpose = "drop", "RndLabs proprietary EULA; conflicts with GPLv3"
        else:
            if code_hits or emb or cfg_hits:
                rc = "required"
            else:
                rc = "drop"
            purpose = purpose_from(p, live, emb)
            if rc == "drop":
                purpose = "unreferenced" if not dead else "only in commented-out code"
        rows.append(dict(path=p, folder=folder, type=typ, used=("runtime-selected by name" if rc == "content" and folder != M + "languages" else
                               ", ".join(f"`{u}`" if "/" in u and not u.startswith("commented") else u for u in used) or "unreferenced"),
                         purpose=purpose, rc=rc))
        for k, v in meta.items():
            stats[folder][k].add(v)

    # ------------------------------------------------ write sha list
    with open(os.path.join(ROOT, OUT_SHA), "w", newline="\n") as f:
        f.write("\n".join(sorted(shas, key=lambda s: s.split("  ", 1)[1])) + "\n")

    # ------------------------------------------------ write markdown
    classes = ["required", "content", "source", "drop"]
    out = ["# Original asset inventory", "",
           "Generated by `tools/asset-inventory.py`. One row per file removed in step 0.4. "
           "Hashes: `original-assets.sha256`. Policy: [../ASSETS-LICENSE.md](../ASSETS-LICENSE.md).", "",
           "Recreate classes: `required` referenced by code; `content` map, skin or translation chosen at runtime; "
           "`source` source art (the replacement needs a source file, not this one); `drop` unreferenced or third-party binary.", "",
           "## Summary", "", "| Folder | Files | " + " | ".join(classes) + " |",
           "|---|---|" + "---|" * len(classes)]
    tot = defaultdict(int)
    for folder, _ in FOLDERS:
        rs = [r for r in rows if r["folder"] == folder]
        if not rs:
            continue
        cnt = [sum(1 for r in rs if r["rc"] == c) for c in classes]
        for c, n in zip(classes, cnt):
            tot[c] += n
        out.append(f"| `{label(folder)}` | {len(rs)} | " + " | ".join(map(str, cnt)) + " |")
    out.append(f"| **Total** | **{len(rows)}** | " + " | ".join(f"**{tot[c]}**" for c in classes) + " |")
    out.append("")

    def pow2(n):
        return n > 0 and n & (n - 1) == 0

    for folder, _ in FOLDERS:
        rs = [r for r in rows if r["folder"] == folder]
        if not rs:
            continue
        out += [f"## `{label(folder)}`", ""]
        s = stats[folder]
        notes = []
        if "tga" in s:
            t = sorted(s["tga"])
            bad = sum(1 for r in rs if (m := re.match(r"TGA (\d+)×(\d+)×", r["type"])) and not (pow2(int(m[1])) and pow2(int(m[2]))))
            notes.append(f"TGA, bit depths {sorted({x[2] for x in t})}, width/height {min(x[0] for x in t)}–{max(x[0] for x in t)} × "
                         f"{min(x[1] for x in t)}–{max(x[1] for x in t)}; {bad} non-power-of-two. "
                         "Replacements: power-of-two sizes, 32-bit with alpha where the original has it (UI, sprites, shadows, halos).")
        if "wav" in s:
            notes.append(f"WAV PCM; channels {sorted({x[0] for x in s['wav']})}, rates {sorted({x[1] for x in s['wav']})} Hz, "
                         f"bits {sorted({x[2] for x in s['wav']})}. Replacements: same PCM WAV; 3D-positioned sounds mono.")
        if "ogg" in s:
            notes.append(f"OGG Vorbis {sorted(s['ogg'])}; also referenced: `IntroScreen.mp3` (not in the tree).")
        if "bvm" in s:
            notes.append(f"BVM versions {sorted({x[0] for x in s['bvm']})}, grids {min(x[1] for x in s['bvm'])}×{min(x[2] for x in s['bvm'])} "
                         f"to {max(x[1] for x in s['bvm'])}×{max(x[2] for x in s['bvm'])}. Format: "
                         "[../analysis/02_DATA_STRUCTURES.md](../analysis/02_DATA_STRUCTURES.md) §4.3. Map names are truncated to 15 characters.")
        if "dko" in s:
            notes.append("DKO v2 chunk format (`Engine/dko/Code/DKO Chunk Info.txt`), exported from 3ds Max with `dkoExporter.dle`. "
                         "Scale and orientation: match the originals' world units (top-down view, +Z up); check against `Engine/dko` before replacing. "
                         "Texture names are embedded in the model. Mesh counts are a chunk-signature heuristic.")
        if folder == M + "skins":
            notes.append("One TGA per skin, mapped onto the player model by `main/models/`. Skin name = file stem, saved in `cl_skin`.")
        if folder == M + "languages":
            notes.append("Replacements are written from `en.lang` (kept). `OptionMenu.cpp:163-251` lists the language choices.")
        if folder == M + "fonts":
            notes.append("Bitmap font atlas read by `dkfCreateFont`; keep the glyph grid layout of the original.")
        for n in notes:
            out.append(f"- {n}")
        if notes:
            out.append("")
        out += ["| Path | Type | Used by | Purpose | Recreate | Replaced by |", "|---|---|---|---|---|---|"]
        for r in rs:
            rel = r["path"][len(folder) + 1:] if "[" not in folder else r["path"].replace(C, "")
            out.append(f"| `{rel}` | {r['type']} | {r['used']} | {r['purpose']} | {r['rc']} | |")
        out.append("")
    with open(os.path.join(ROOT, OUT_MD), "w", newline="\n") as f:
        f.write("\n".join(out))
    print(f"{len(rows)} files; " + ", ".join(f"{c}={tot[c]}" for c in classes))
    todo = [r["path"] for r in rows if r["rc"] == "required" and r["purpose"] == "TODO"]
    if todo:
        print("required with TODO purpose:", *todo, sep="\n  ")
    return 0


if __name__ == "__main__":
    sys.exit(main())
