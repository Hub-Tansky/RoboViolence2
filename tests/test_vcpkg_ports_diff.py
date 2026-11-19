#!/usr/bin/env python3
"""tools/vcpkg-ports-diff.py (ROBO-251): installed ports from `<triplet>/share/<port>/vcpkg.spdx.json`, their versions at two commits of a
throwaway vcpkg-like git repo, and the Markdown table with changed ports first (ADR ROBO-251)."""
import importlib.util
import json
import os
import shutil
import stat
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
spec = importlib.util.spec_from_file_location("ports_diff", os.path.join(ROOT, "tools", "vcpkg-ports-diff.py"))
tool = importlib.util.module_from_spec(spec)
spec.loader.exec_module(tool)
failures = 0


def check(name, cond, detail=""):
    global failures
    if not cond:
        print(f"FAIL {name} {detail}")
        failures += 1


def git(repo, *args):
    return subprocess.run(["git", "-C", repo, *args], check=True, capture_output=True, text=True).stdout.strip()


def write_ports(repo, ports):
    shutil.rmtree(os.path.join(repo, "ports"), ignore_errors=True)
    for name, manifest in ports.items():
        os.makedirs(os.path.join(repo, "ports", name))
        with open(os.path.join(repo, "ports", name, "vcpkg.json"), "w") as f:
            json.dump({"name": name, **manifest}, f)
    git(repo, "add", "-A")
    git(repo, "commit", "-qm", "ports")
    return git(repo, "rev-parse", "HEAD")


def rmtree(path):
    def writable(func, p, _):
        os.chmod(p, stat.S_IWRITE)
        func(p)
    shutil.rmtree(path, **({"onexc": writable} if sys.version_info >= (3, 12) else {"onerror": writable}))


tmp = tempfile.mkdtemp(prefix="vcpkg-")
try:
    repo = os.path.join(tmp, "vcpkg")
    os.makedirs(repo)
    git(repo, "init", "-q")
    git(repo, "config", "user.name", "t")
    git(repo, "config", "user.email", "t@example.invalid")
    old = write_ports(repo, {
        "sdl3": {"version": "3.2.10", "port-version": 1},
        "sqlite3": {"version": "3.49.1"},
        "stb": {"version-date": "2024-07-29"},
        "vcpkg-cmake": {"version-date": "2024-04-23"},
    })
    new = write_ports(repo, {
        "sdl3": {"version": "3.2.20"},
        "sqlite3": {"version": "3.49.1", "port-version": 2},
        "stb": {"version-date": "2024-07-29"},
        "vcpkg-cmake": {"version-date": "2024-04-23"},
        "libsodium": {"version-semver": "1.0.20"},
    })
    installed = os.path.join(tmp, "installed")
    for triplet, names in {"x64-linux": ["sdl3", "sqlite3", "stb", "libsodium"], "x64-linux-host": ["vcpkg-cmake", "sqlite3"]}.items():
        for name in names:
            os.makedirs(os.path.join(installed, triplet, "share", name))
            open(os.path.join(installed, triplet, "share", name, "vcpkg.spdx.json"), "w").write("{}")
    os.makedirs(os.path.join(installed, "x64-linux", "share", "curl"))  # no spdx: not an installed port
    os.makedirs(os.path.join(installed, "vcpkg", "info"))  # vcpkg's own database dir

    ports = tool.installed_ports(installed)
    check("ports: every triplet once, only dirs with an spdx file", ports == ["libsodium", "sdl3", "sqlite3", "stb", "vcpkg-cmake"], ports)
    check("version with port-version", tool.version(repo, old, "sdl3") == "3.2.10#1", tool.version(repo, old, "sdl3"))
    check("version-date", tool.version(repo, old, "stb") == "2024-07-29")
    check("version-semver", tool.version(repo, new, "libsodium") == "1.0.20")
    check("missing port", tool.version(repo, old, "libsodium") == "-")

    out = tool.table(repo, old, new, ports)
    lines = out.splitlines()
    check("summary", lines[0] == "3 of 5 installed ports change.", lines[0])
    check("changed first, by name", [l.split(" | ")[0] for l in lines[4:9]] == ["| libsodium", "| sdl3", "| sqlite3", "| stb", "| vcpkg-cmake"], lines[4:9])
    check("changed row bold", "| sdl3 | 3.2.10#1 | **3.2.20** |" in lines, lines)
    check("port-version only", "| sqlite3 | 3.49.1 | **3.49.1#2** |" in lines, lines)
    check("unchanged row plain", "| stb | 2024-07-29 | 2024-07-29 |" in lines, lines)

    r = subprocess.run([sys.executable, "-B", os.path.join(ROOT, "tools", "vcpkg-ports-diff.py"), repo, old, new, installed],
                       capture_output=True, text=True)
    check("cli", r.returncode == 0 and r.stdout == out, r.stderr)
finally:
    rmtree(tmp)

print("FAILED" if failures else "vcpkg ports diff: ok")
sys.exit(1 if failures else 0)
