#!/usr/bin/env python3
"""Port versions at two vcpkg baselines, as a Markdown table for the monthly baseline PR (ROBO-251, ADR ROBO-251).

Usage: tools/vcpkg-ports-diff.py <vcpkg root> <old baseline> <new baseline> <vcpkg_installed dir>
The ports are those installed (the `Package:` paragraphs without `Feature:` in <vcpkg_installed>/vcpkg/status),
host tools included. Each version is read from `ports/<port>/vcpkg.json` at each commit of the vcpkg checkout, which
must hold both commits. Changed rows come first. Python 3, stdlib only.
"""
import json
import os
import subprocess
import sys

VERSION_KEYS = ("version", "version-semver", "version-date", "version-string")


def installed_ports(installed):
    with open(os.path.join(installed, "vcpkg", "status"), encoding="utf-8") as f:
        paragraphs = f.read().split("\n\n")
    ports = set()
    for para in paragraphs:
        fields = dict(line.split(": ", 1) for line in para.splitlines() if ": " in line)
        if "Package" in fields and "Feature" not in fields and "not-installed" not in fields.get("Status", ""):
            ports.add(fields["Package"])
    return sorted(ports)


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
