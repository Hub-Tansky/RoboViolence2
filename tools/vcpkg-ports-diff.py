#!/usr/bin/env python3
"""Port versions at two vcpkg baselines, as a Markdown table for the monthly baseline PR (ROBO-251, ADR ROBO-251).

Usage: tools/vcpkg-ports-diff.py <vcpkg root> <old baseline> <new baseline> <vcpkg_installed dir>
The ports are those installed in any triplet of <vcpkg_installed> (their `share/<port>/vcpkg.spdx.json`, as
tools/osv-vcpkg.py reads them), host tools included. Each version is read from `ports/<port>/vcpkg.json` at each commit of the vcpkg checkout, which
must hold both commits. Changed rows come first. Python 3, stdlib only.
"""
import glob
import json
import os
import subprocess
import sys

VERSION_KEYS = ("version", "version-semver", "version-date", "version-string")


def installed_ports(installed):
    """Port names with a `<triplet>/share/<port>/vcpkg.spdx.json`, as tools/osv-vcpkg.py finds them."""
    found = glob.glob(os.path.join(installed, "*", "share", "*", "vcpkg.spdx.json"))
    return sorted({os.path.basename(os.path.dirname(f)) for f in found})


def version(root, commit, port):
    """`1.2.3#4`, or `-` when the port does not exist at that commit."""
    shown = subprocess.run(["git", "-C", root, "show", f"{commit}:ports/{port}/vcpkg.json"], capture_output=True, text=True)
    if shown.returncode != 0:
        return "-"
    manifest = json.loads(shown.stdout)
    text = next((str(manifest[k]) for k in VERSION_KEYS if k in manifest), "?")
    return f"{text}#{manifest['port-version']}" if manifest.get("port-version") else text


def table(root, old, new, ports):
    rows = [(port, version(root, old, port), version(root, new, port)) for port in ports]
    rows.sort(key=lambda r: (r[1] == r[2], r[0]))
    lines = ["| Port | Old | New |", "|---|---|---|"]
    lines += [f"| {p} | {a} | {'**' + b + '**' if a != b else b} |" for p, a, b in rows]
    changed = sum(a != b for _, a, b in rows)
    return f"{changed} of {len(rows)} installed ports change.\n\n" + "\n".join(lines) + "\n"


def main():
    if len(sys.argv) != 5:
        sys.exit(__doc__)
    root, old, new, installed = sys.argv[1:]
    sys.stdout.write(table(root, old, new, installed_ports(installed)))


if __name__ == "__main__":
    main()
