#!/usr/bin/env python3
"""ROBO-188 (ADR 0051): split-symbols.py moves a Release build's symbols into the zip and leaves stripped binaries.

Usage: test_split_symbols.py <runtime dir> <scratch dir>. Works on a copy of the three binaries, never on the runtime.
"""
import importlib.util
import shutil
import subprocess
import sys
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
spec = importlib.util.spec_from_file_location("split_symbols", ROOT / "packaging" / "split-symbols.py")
split_symbols = importlib.util.module_from_spec(spec)
spec.loader.exec_module(split_symbols)

runtime, scratch = Path(sys.argv[1]), Path(sys.argv[2]) / "split-symbols"
shutil.rmtree(scratch, ignore_errors=True)
copy = scratch / "runtime"
copy.mkdir(parents=True)
for name in split_symbols.BINARIES:
    if sys.platform == "darwin" and name == "bv2":
        shutil.copytree(runtime / "bv2.app" / "Contents" / "MacOS", copy / "bv2.app" / "Contents" / "MacOS")
        continue
    src = split_symbols.binary(runtime, name)
    shutil.copy2(src, copy)
    if sys.platform == "win32":
        shutil.copy2(src.with_suffix(".pdb"), copy)

subprocess.run([sys.executable, ROOT / "packaging" / "split-symbols.py", copy, "test", scratch / "out", "9.9.9"], check=True)
names = zipfile.ZipFile(scratch / "out" / "roboviolence2-test-9.9.9-symbols.zip").namelist()
failures = []
for name in split_symbols.BINARIES:
    path = split_symbols.binary(copy, name)
    if sys.platform == "win32":
        ok = f"roboviolence2-test-9.9.9-symbols/{name}.pdb" in names
    elif sys.platform == "darwin":
        ok = f"roboviolence2-test-9.9.9-symbols/{name}.dSYM/Contents/Resources/DWARF/{name}" in names
        stabs = subprocess.run(["nm", "-ap", path], capture_output=True, text=True).stdout
        ok = ok and " OSO " not in stabs  # strip -S removed the debug map
        if name != "bv2":  # the servers keep a valid signature, so they still start on arm64; make-package.py re-signs the app
            ok = ok and subprocess.run(["codesign", "--verify", path]).returncode == 0
    else:
        bid = split_symbols.build_id(path)
        ok = f"roboviolence2-test-9.9.9-symbols/.build-id/{bid[:2]}/{bid[2:]}.debug" in names
        sections = subprocess.run(["readelf", "-S", path], capture_output=True, text=True).stdout
        ok = ok and ".debug_info" not in sections and ".gnu_debuglink" in sections
    if not ok:
        failures.append(name)
if failures:
    print("FAIL: symbols not split for", ", ".join(failures), "; zip holds", names[:20])
    sys.exit(1)
print("ok: symbols split for", ", ".join(split_symbols.BINARIES))
