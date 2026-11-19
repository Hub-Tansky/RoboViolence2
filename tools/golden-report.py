#!/usr/bin/env python3
"""Before and after of regenerated screenshot goldens, as Markdown for a job summary (ROBO-254, ADR ROBO-254).

Usage: tools/golden-report.py <before dir> <after dir> <preview dir> <before URL> <preview URL>
<before dir> holds the branch's goldens (tests/golden/*.png before regenerating), <after dir> the new ones. For each
new golden that differs from the old one at all, writes <preview dir>/<name>-after.png and, when the sizes match,
<name>-diff.png (changed pixels red, the rest the old golden dimmed, as tests/screenshots.py draws it). Prints Markdown:
one row per golden with the share of pixels beyond the test's tolerance, then the three images of each changed one.
Image links are <before URL>/<name>.png and <preview URL>/<name>-after.png / -diff.png. Python 3, stdlib only.
"""
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "tests"))
from screenshots import MAX_CHANGED, compare, read_png, write_png  # noqa: E402  the test's own comparison


def report(before_dir, after_dir, preview_dir, before_url, preview_url):
    """(Markdown lines, names that changed)"""
    os.makedirs(preview_dir, exist_ok=True)
    rows, shown = [], []
    for png in sorted(f for f in os.listdir(after_dir) if f.endswith(".png")):
        name = png[:-4]
        after_path, before_path = os.path.join(after_dir, png), os.path.join(before_dir, png)
        after = read_png(after_path)
        if not os.path.exists(before_path):
            rows.append(f"| `{name}` | new golden | |")
            write_png(os.path.join(preview_dir, f"{name}-after.png"), *after)
            shown.append((name, False, False))
            continue
        with open(after_path, "rb") as a, open(before_path, "rb") as b:
            identical = a.read() == b.read()
        if identical:
            rows.append(f"| `{name}` | 0 (identical file) | unchanged |")
            continue
        before = read_png(before_path)
        fraction, diff = compare(after, before)
        if diff is None:
            verdict = f"size {before[0]}x{before[1]} -> {after[0]}x{after[1]}"
        else:
            verdict = "over the test's limit" if fraction > MAX_CHANGED else "within the test's limit"
        rows.append(f"| `{name}` | {fraction:.4%} | {verdict} |")
        write_png(os.path.join(preview_dir, f"{name}-after.png"), *after)
        if diff is not None:
            write_png(os.path.join(preview_dir, f"{name}-diff.png"), after[0], after[1], diff)
        shown.append((name, True, diff is not None))
    lines = ["| Golden | Pixels changed beyond tolerance | |", "|---|---|---|"] + rows
    for name, has_before, has_diff in shown:
        lines += ["", f"### {name}", "", "| Before | After | Diff |", "|---|---|---|"]
        before_img = f"![before]({before_url}/{name}.png)" if has_before else "none"
        diff_img = f"![diff]({preview_url}/{name}-diff.png)" if has_diff else "none"
        lines.append(f"| {before_img} | ![after]({preview_url}/{name}-after.png) | {diff_img} |")
    return lines, [name for name, _, _ in shown]


def main(argv):
    if len(argv) != 6:
        sys.exit(__doc__)
    lines, changed = report(*argv[1:6])
    print("\n".join(lines))
    if os.environ.get("GITHUB_OUTPUT"):
        with open(os.environ["GITHUB_OUTPUT"], "a") as f:
            f.write(f"changed={' '.join(changed)}\n")


if __name__ == "__main__":
    main(sys.argv)
