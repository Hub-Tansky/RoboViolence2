"""The application icon files are complete and every package format points at them (ADR 0030).

Checks the committed art from tools/gen-app-icon.py (the .ico and .icns hold every size each OS asks for, the Linux
hicolor tree holds the sizes its folders name) and the wiring: the Windows resource and installer, the macOS
Info.plist, the Linux .desktop file, and the .dmg backgrounds drawn for dmg-layout.json's window size.
Python 3, stdlib only.
"""
import json
import pathlib
import re
import struct
import sys

root = pathlib.Path(__file__).resolve().parent.parent
icons = root / "packaging" / "icons"
failures = []


def check(ok, message):
    if not ok:
        failures.append(message)


def png_size(data):
    check(data[:8] == b"\x89PNG\r\n\x1a\n", "not a PNG")
    width, height, depth, colour = struct.unpack(">IIBB", data[16:26])
    return width, height, depth, colour


# Linux hicolor tree (installed as is) and the client's built-in window icon: 8-bit RGBA, as large as the folder says
hicolor = {}
for f in sorted((icons / "hicolor").glob("*/apps/roboviolence2.png")):
    size = int(f.parent.parent.name.split("x")[0])
    hicolor[size] = f
    check(f.parent.parent.name == f"{size}x{size}" and png_size(f.read_bytes()) == (size, size, 8, 6),
          f"{f.relative_to(root)}: not a {size} px RGBA PNG")
check(sorted(hicolor) == [16, 24, 32, 48, 64, 128, 256, 512], f"hicolor sizes: {sorted(hicolor)}")
check(not [p for p in (icons / "hicolor").rglob("*") if p.is_file() and p.name != "roboviolence2.png"],
      "hicolor holds files other than roboviolence2.png")

# Windows: every size Explorer, the taskbar and the Start menu ask for, at 100-200 % scaling
ico = (icons / "roboviolence2.ico").read_bytes()
reserved, kind, count = struct.unpack("<HHH", ico[:6])
check((reserved, kind) == (0, 1), "roboviolence2.ico: not an icon file")
ico_sizes = set()
for i in range(count):
    w, h, _, _, _, _, length, offset = struct.unpack("<BBBBHHII", ico[6 + 16 * i:22 + 16 * i])
    w, h = w or 256, h or 256  # 0 means 256
    check(png_size(ico[offset:offset + length])[:2] == (w, h), f"roboviolence2.ico: the {w} px image is not {w} px")
    ico_sizes.add(w)
check(ico_sizes == {16, 24, 32, 48, 64, 128, 256}, f"roboviolence2.ico holds {sorted(ico_sizes)}")

# macOS: the full set iconutil writes, 16 pt to 512 pt at 1x and 2x
icns = (icons / "roboviolence2.icns").read_bytes()
check(icns[:4] == b"icns" and struct.unpack(">I", icns[4:8])[0] == len(icns), "roboviolence2.icns: bad header")
want = {"icp4": 16, "icp5": 32, "ic11": 32, "ic12": 64, "ic07": 128, "ic13": 256, "ic08": 256, "ic14": 512,
        "ic09": 512, "ic10": 1024}
pos, found = 8, {}
while pos < len(icns):
    kind, length = icns[pos:pos + 4].decode("latin-1"), struct.unpack(">I", icns[pos + 4:pos + 8])[0]
    found[kind] = png_size(icns[pos + 8:pos + length])[0]
    pos += length
check(found == want, f"roboviolence2.icns holds {found}")

# Wiring: each package format names a file that exists
plist = (root / "packaging/macos/Info.plist.in").read_text()
icon_file = re.search(r"<key>CFBundleIconFile</key><string>([^<]+)</string>", plist)
check(icon_file and (icons / (icon_file.group(1) + ".icns")).is_file(), "Info.plist.in: no CFBundleIconFile icns")
rc = (root / "packaging/windows/bv2.rc.in").read_text()
check('ICON "@BV2_ICONS@/roboviolence2.ico"' in rc, "bv2.rc.in: the exe does not use roboviolence2.ico")
iss = (root / "packaging/windows/roboviolence2.iss").read_text()
setup_icon = re.search(r"^SetupIconFile=(.+)$", iss, re.M)
check(setup_icon and (root / "packaging/windows" / setup_icon.group(1).replace("\\", "/")).resolve().is_file(),
      "roboviolence2.iss: SetupIconFile missing or names no file")
desktop = (root / "packaging/linux/roboviolence2.desktop").read_text()
check(re.search(r"^Icon=roboviolence2$", desktop, re.M), "roboviolence2.desktop: Icon is not roboviolence2")


# The .dmg backgrounds are drawn for the window size make-package.py opens (both read dmg-layout.json)
layout = json.loads((root / "packaging/macos/dmg-layout.json").read_text())
for name, scale in (("dmg-background.png", 1), ("dmg-background@2x.png", 2)):
    size = png_size((root / "packaging/macos" / name).read_bytes())[:2]
    check(list(size) == [n * scale for n in layout["window"]], f"{name} is {size}, the window {layout['window']}")

for f in failures:
    print("FAIL", f)
if failures:
    sys.exit(1)
print("ok: app icon files and their wiring")
