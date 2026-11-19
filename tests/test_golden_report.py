#!/usr/bin/env python3
"""tools/golden-report.py (ROBO-254): an identical golden, one within and one over the test's tolerance, a size change and a
new golden, with the preview files and image links each one gets (ADR ROBO-254)."""
import importlib.util
import os
import sys
import tempfile

sys.dont_write_bytecode = True
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
spec = importlib.util.spec_from_file_location("golden_report", os.path.join(ROOT, "tools", "golden-report.py"))
tool = importlib.util.module_from_spec(spec)
spec.loader.exec_module(tool)
failures = 0


def check(name, cond, detail=""):
    global failures
    if not cond:
        print(f"FAIL {name} {detail}")
        failures += 1


def image(width, height, changed=0, value=200):
    """grey frame with the first `changed` pixels set to `value`"""
    rows = []
    for y in range(height):
        row = bytearray([50] * width * 3)
        for x in range(width):
            if y * width + x < changed:
                row[x * 3:x * 3 + 3] = bytes([value] * 3)
        rows.append(bytes(row))
    return width, height, rows


with tempfile.TemporaryDirectory() as tmp:
    before, after, preview = (os.path.join(tmp, d) for d in ("before", "after", "preview"))
    os.makedirs(before)
    os.makedirs(after)
    w = tool.write_png
    w(os.path.join(before, "same.png"), *image(20, 10))
    w(os.path.join(after, "same.png"), *image(20, 10))
    w(os.path.join(before, "tiny.png"), *image(20, 10))
    w(os.path.join(after, "tiny.png"), *image(20, 10, changed=1, value=60))  # within the channel tolerance: 0 beyond it
    w(os.path.join(before, "menu.png"), *image(20, 10))
    w(os.path.join(after, "menu.png"), *image(20, 10, changed=50))  # 25 % of the pixels
    w(os.path.join(before, "resized.png"), *image(20, 10))
    w(os.path.join(after, "resized.png"), *image(10, 10))
    w(os.path.join(after, "brand-new.png"), *image(20, 10))
    w(os.path.join(before, "removed.png"), *image(20, 10))  # only new goldens are reported

    lines, changed = tool.report(before, after, preview, "https://b", "https://p")
    text = "\n".join(lines)
    check("changed list", changed == ["brand-new", "menu", "resized", "tiny"], changed)
    check("identical row", "| `same` | 0 (identical file) | unchanged |" in text, text)
    check("within row", "| `tiny` | 0.0000% | within the test's limit |" in text, text)
    check("over row", "| `menu` | 25.0000% | over the test's limit |" in text, text)
    check("size row", "| `resized` | 100.0000% | size 20x10 -> 10x10 |" in text, text)
    check("new row", "| `brand-new` | new golden | |" in text, text)
    check("removed not listed", "removed" not in text, text)
    check("menu images", "| ![before](https://b/menu.png) | ![after](https://p/menu-after.png) | ![diff](https://p/menu-diff.png) |" in text, text)
    check("new golden images", "| none | ![after](https://p/brand-new-after.png) | none |" in text, text)
    check("resized has no diff", "| ![before](https://b/resized.png) | ![after](https://p/resized-after.png) | none |" in text, text)
    check("no section for identical", "### same" not in text, text)
    files = sorted(os.listdir(preview))
    check("preview files", files == ["brand-new-after.png", "menu-after.png", "menu-diff.png", "resized-after.png", "tiny-after.png", "tiny-diff.png"], files)
    dw, dh, drows = tool.read_png(os.path.join(preview, "menu-diff.png"))
    check("diff marks changed pixels red", (dw, dh) == (20, 10) and drows[0][0:3] == bytes([255, 0, 0]) and drows[9][0:3] == bytes([16, 16, 16]), drows[0][0:6])

print("ok" if failures == 0 else f"{failures} failed")
sys.exit(1 if failures else 0)
