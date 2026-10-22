#!/usr/bin/env python3
"""Render the application icons and the macOS disk image background into packaging/ (ADR 0030).

The icon is the website favicon (website/img/favicon.svg) grown up: a dark rounded tile, a blue ball bottom-left
and a red ball top-right. Large sizes add sphere shading and a light edge that keeps the tile visible on dark docks
and taskbars; 16, 24 and 32 px keep the flat favicon shapes so they stay sharp.

Writes, relative to the repository root:
  packaging/icons/roboviolence2.icns          macOS: Apple's grid (824 px squircle in 1024) with its drop shadow
  packaging/icons/roboviolence2.ico           Windows: 16-256 px, near full bleed, no baked shadow
  packaging/icons/hicolor/<n>x<n>/apps/roboviolence2.png
                                              Linux hicolor tree (installed as is); 128 px is the client's
                                              window icon (same art as the .ico)
  packaging/macos/dmg-background(@2x).png     the .dmg window: arrow from the app to Applications, laid out by
                                              packaging/macos/dmg-layout.json (make-package.py reads it too)

The outputs are committed: run this only to change the art. Needs numpy and Pillow (not a build dependency).
Usage: tools/gen-app-icon.py [--font <ttf/otf for the .dmg caption, default Inter Regular as on Debian/Ubuntu>]
"""
import argparse
import io
import json
import os
import struct

import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ICONS = os.path.join(ROOT, "packaging", "icons")
MACOS = os.path.join(ROOT, "packaging", "macos")
SS = 4  # supersampling per output pixel

MAC_SIZES = (1024, 512, 256, 128, 64, 32, 16)
# The full set iconutil writes: each type holds a PNG of this many pixels (the 2x types reuse the same images)
ICNS_TYPES = (("icp4", 16), ("icp5", 32), ("ic11", 32), ("ic12", 64), ("ic07", 128), ("ic13", 256),
              ("ic08", 256), ("ic14", 512), ("ic09", 512), ("ic10", 1024))
PNG_SIZES = (16, 24, 32, 48, 64, 128, 256, 512)
ICO_SIZES = (256, 128, 64, 48, 32, 24, 16)
FLAT_MAX = 32  # sizes up to this use the flat favicon shapes

# The .dmg window, in points. The image is as tall as the whole window, so whether Finder counts the title bar in
# the window bounds or not, no edge of it shows.
with open(os.path.join(MACOS, "dmg-layout.json")) as f:
    DMG = json.load(f)


def hexrgb(h):
    h = h.lstrip("#")
    return np.array([int(h[i:i + 2], 16) for i in (0, 2, 4)], float) / 255


BG_TOP, BG_BOT, BG_FLAT = hexrgb("#2a2f40"), hexrgb("#12141b"), hexrgb("#1b1e28")
BLUE, RED, EDGE = hexrgb("#2f7df6"), hexrgb("#e5383b"), hexrgb("#4a5170")


def superellipse_mask(X, Y, cx, cy, half, n=5.0):
    # Apple-like squircle |x|^n + |y|^n <= 1, antialiased by the supersampling
    return ((np.abs((X - cx) / half) ** n + np.abs((Y - cy) / half) ** n) <= 1.0).astype(float)


def rrect_mask(X, Y, x0, y0, size, r):
    dx = np.maximum(np.maximum(x0 + r - X, X - (x0 + size - r)), 0)
    dy = np.maximum(np.maximum(y0 + r - Y, Y - (y0 + size - r)), 0)
    inside = (X >= x0) & (X <= x0 + size) & (Y >= y0) & (Y <= y0 + size)
    return (inside & (dx * dx + dy * dy <= r * r)).astype(float)


def over(dst, rgb, a):
    a = a[..., None]
    dst[..., :3] = rgb * a + dst[..., :3] * (1 - a)
    dst[..., 3:] = a + dst[..., 3:] * (1 - a)


def render(px, style):
    """style: 'mac' (824/1024 squircle + shadow), 'full' (Windows/Linux, small margin), 'flat' (favicon)."""
    N = px * SS
    Y, X = np.mgrid[0:N, 0:N].astype(float) + 0.5
    X /= N
    Y /= N
    img = np.zeros((N, N, 4))
    if style == "mac":
        x0, size = 100 / 1024, 824 / 1024
        tile = superellipse_mask(X, Y, 0.5, 0.5, size / 2)
        inner = superellipse_mask(X, Y, 0.5, 0.5, size / 2 - size * 0.4 / 32)
    elif style == "full":
        x0, size = 32 / 1024, 960 / 1024
        tile = rrect_mask(X, Y, x0, x0, size, size * 0.1875)
        w = size * 0.4 / 32
        inner = rrect_mask(X, Y, x0 + w, x0 + w, size - 2 * w, size * 0.1875 - w)
    else:  # the favicon: full bleed, 6/32 corners
        x0, size = 0.0, 1.0
        tile = rrect_mask(X, Y, 0, 0, 1, 6 / 32)
        inner = rrect_mask(X, Y, 1 / 32, 1 / 32, 30 / 32, 5 / 32)

    # U, V: favicon units (0..32) across the tile
    U = (X - x0) / size * 32
    V = (Y - x0) / size * 32
    if style == "flat":
        over(img, BG_FLAT, tile)
    else:
        t = np.clip(V / 32, 0, 1)[..., None]
        img[..., :3] = BG_TOP * (1 - t) + BG_BOT * t
        img[..., 3] = tile
        img[..., :3] += (tile * np.clip(1 - V / 1.2, 0, 1) * 0.10)[..., None]  # faint top rim light

    # edge stroke so the dark tile stays visible on dark docks and taskbars
    ea = (np.clip(tile - inner, 0, 1) * (1.0 if style == "flat" else 0.6))[..., None]
    img[..., :3] = EDGE * ea + img[..., :3] * (1 - ea)

    for (cx, cy, base) in ((11, 21, BLUE), (22, 10, RED)):
        d = np.hypot(U - cx, V - cy)
        ball = (d <= 6).astype(float) * tile
        if style == "flat":
            over(img, base, ball)
            continue
        # contact shadow below and right of the ball
        sd = np.hypot(U - cx - 0.6, V - cy - 1.1)
        sh = np.clip(1 - (sd - 5.2) / 1.6, 0, 1) * 0.45 * tile * (1 - ball)
        img[..., :3] *= (1 - sh)[..., None]
        # sphere shading, light from the upper left
        ld = np.clip(np.hypot(U - (cx - 2.4), V - (cy - 2.6)) / 9.0, 0, 1)[..., None]
        col = np.minimum(base * 1.35 + 0.08, 1) * (1 - ld) + base * ld
        rimk = np.clip((d - 4.6) / 1.4, 0, 1)[..., None]
        col = col * (1 - rimk * 0.55) + base * 0.45 * rimk * 0.55
        sp = np.exp(-(((U - (cx - 2.3)) / 1.5) ** 2 + ((V - (cy - 2.5)) / 1.0) ** 2))[..., None] * 0.55
        col = col * (1 - sp) + sp
        a = ball[..., None]
        img[..., :3] = col * a + img[..., :3] * (1 - a)

    img[..., :3] = np.clip(img[..., :3], 0, 1) * img[..., 3:]  # premultiplied, for a clean downscale
    im = Image.fromarray((img * 255).round().astype(np.uint8), "RGBa").resize((px, px), Image.LANCZOS)
    im = im.convert("RGBA")

    if style == "mac":
        # Apple's icon shadow: black ~30 %, ~12/1024 down, ~28/1024 blur
        a = Image.fromarray(np.array(im)[..., 3], "L")
        canvas = Image.new("L", (px, px), 0)
        canvas.paste(a, (0, max(1, round(px * 12 / 1024))))
        canvas = canvas.filter(ImageFilter.GaussianBlur(px * 28 / 1024 / 2))
        shadow = Image.new("RGBA", (px, px), (0, 0, 0, 0))
        shadow.putalpha(canvas.point(lambda v: int(v * 0.30)))
        im = Image.alpha_composite(shadow, im)
    return im


def write_icns(path, images):
    def png(im):
        buf = io.BytesIO()
        im.save(buf, "PNG", optimize=True)
        return buf.getvalue()

    body = b"".join(t.encode() + struct.pack(">I", 8 + len(d)) + d
                    for t, d in ((t, png(images[n])) for t, n in ICNS_TYPES))
    with open(path, "wb") as f:
        f.write(b"icns" + struct.pack(">I", 8 + len(body)) + body)


def dmg_background(scale, font_path):
    """Light window (Finder's own look), a grey arrow between the two icon slots, a one-line caption."""
    w, h = DMG["window"][0] * scale, DMG["window"][1] * scale
    t = np.linspace(0, 1, h)[:, None, None]
    top, bot = np.array([250, 250, 252], float), np.array([236, 237, 241], float)
    im = Image.fromarray(np.broadcast_to(top * (1 - t) + bot * t, (h, w, 3)).round().astype(np.uint8), "RGB")
    s = SS * scale
    big = im.resize((w * SS, h * SS), Image.NEAREST)
    draw = ImageDraw.Draw(big)
    grey = (142, 146, 156)
    y = DMG["app"][1] * s
    x0, x1 = (DMG["app"][0] + 96) * s, (DMG["applications"][0] - 96) * s
    width, head = 6 * s, 22 * s
    draw.line([(x0, y), (x1 - width // 2, y)], fill=grey, width=width)
    draw.line([(x1 - head, y - head), (x1, y), (x1 - head, y + head)], fill=grey, width=width, joint="curve")
    for (cx, cy) in ((x0, y), (x1 - head, y - head), (x1 - head, y + head), (x1, y)):
        r = width // 2
        draw.ellipse([cx - r, cy - r, cx + r, cy + r], fill=grey)
    im = big.resize((w, h), Image.LANCZOS)
    font = ImageFont.truetype(font_path, 14 * scale)
    ImageDraw.Draw(im).text((w // 2, DMG["caption_y"] * scale), "Drag Robo Violence 2 to Applications", font=font,
                            fill=(110, 115, 128), anchor="mm")
    return im


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--font", default="/usr/share/fonts/opentype/inter/Inter-Regular.otf")
    args = parser.parse_args()
    if not os.path.isfile(args.font):
        parser.error(f"no font at {args.font}: pass --font (the committed caption uses Inter Regular)")
    font = args.font
    os.makedirs(ICONS, exist_ok=True)

    mac = {s: render(s, "mac") for s in MAC_SIZES}
    write_icns(os.path.join(ICONS, "roboviolence2.icns"), mac)

    icons = {s: render(s, "flat" if s <= FLAT_MAX else "full") for s in PNG_SIZES}
    for s in PNG_SIZES:
        apps = os.path.join(ICONS, "hicolor", f"{s}x{s}", "apps")
        os.makedirs(apps, exist_ok=True)
        icons[s].save(os.path.join(apps, "roboviolence2.png"), optimize=True)
    icons[256].save(os.path.join(ICONS, "roboviolence2.ico"), sizes=[(s, s) for s in ICO_SIZES],
                    append_images=[icons[s] for s in ICO_SIZES[1:]])

    dmg_background(1, font).save(os.path.join(MACOS, "dmg-background.png"), optimize=True)
    dmg_background(2, font).save(os.path.join(MACOS, "dmg-background@2x.png"), optimize=True)


if __name__ == "__main__":
    main()
