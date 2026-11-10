#!/usr/bin/env python3
"""Moves the debug symbols out of a Release runtime before packaging (ROBO-188, ADR 0050).

Usage: split-symbols.py <runtime dir> <preset> <out dir> <version>

Writes <out dir>/roboviolence2-<preset>-<version>-symbols.zip and leaves the binaries in <runtime dir> stripped, so
make-package.py packs them without symbols:
- Linux: <name>.debug per binary (objcopy --only-keep-debug), under .build-id/<xx>/<rest>.debug as gdb and
  debuginfod look them up; the binary keeps a .gnu_debuglink.
- macOS: <name>.dSYM per binary (dsymutil), then strip -S. make-package.py signs the bundle after this.
- Windows: a copy of the linker's <name>.pdb (the .exe carries none; make-package.py packs only named files).
Fails if a binary or its symbols are missing.
"""
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

BINARIES = ["bv2", "bv2dedicated", "bv2master"]


def run(*cmd, **kw):
    return subprocess.run([str(c) for c in cmd], check=True, **kw)


def binary(runtime, name):
    if sys.platform == "darwin" and name == "bv2":
        return runtime / "bv2.app" / "Contents" / "MacOS" / "bv2"
    return runtime / (name + (".exe" if sys.platform == "win32" else ""))


def build_id(path):
    # readelf exits 1 on a warning about another note (e.g. a newer binutils note type), after printing the build ID
    notes = subprocess.run(["readelf", "-n", str(path)], capture_output=True, text=True)
    m = re.search(r"Build ID: ([0-9a-f]+)", notes.stdout)
    if not m:
        sys.exit(f"{path} has no GNU build ID: {notes.stderr.strip()}")
    return m.group(1)


def split(path, name, symbols):
    if sys.platform == "win32":
        shutil.copy2(path.with_suffix(".pdb"), symbols / (name + ".pdb"))
    elif sys.platform == "darwin":
        run("dsymutil", path, "-o", symbols / (name + ".dSYM"))
        run("strip", "-S", path)
    else:
        bid = build_id(path)
        debug = symbols / ".build-id" / bid[:2] / (bid[2:] + ".debug")
        debug.parent.mkdir(parents=True, exist_ok=True)
        run("objcopy", "--only-keep-debug", path, debug)
        run("objcopy", "--strip-debug", "--add-gnu-debuglink=" + str(debug), path)
        print(f"{name}: build ID {bid}")


def main():
    runtime, preset, out, version = Path(sys.argv[1]), sys.argv[2], Path(sys.argv[3]), sys.argv[4]
    out.mkdir(parents=True, exist_ok=True)
    name = f"roboviolence2-{preset}-{version}-symbols"
    with tempfile.TemporaryDirectory() as tmp:
        symbols = Path(tmp) / name
        symbols.mkdir()
        for b in BINARIES:
            path = binary(runtime, b)
            if not path.is_file():
                sys.exit(f"missing {path}")
            split(path, b, symbols)
        archive = shutil.make_archive(str(out / name), "zip", tmp, name)
    print(archive)


if __name__ == "__main__":
    main()
