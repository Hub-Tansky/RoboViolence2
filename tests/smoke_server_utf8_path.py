#!/usr/bin/env python3
"""Windows: bv2dedicated finds main/ when installed under a non-ASCII path (UTF-8 code page manifest, ADR 0012).

Usage: smoke_server_utf8_path.py <bv2dedicated.exe> <scratch dir>
Copies the executable and main/ into <scratch>/Łódź/ and starts it from <scratch>, so only the executable's own
directory (converted to a narrow UTF-8 path, game/src/Paths.cpp) can lead to main/.
"""
import os
import shutil
import subprocess
import sys

exe, scratch = sys.argv[1], sys.argv[2]
dest = os.path.join(scratch, "Łódź")  # Łódź
shutil.rmtree(dest, ignore_errors=True)
os.makedirs(dest)
shutil.copy2(exe, dest)
shutil.copytree(os.path.join(os.path.dirname(os.path.abspath(exe)), "main"), os.path.join(dest, "main"))  # follows junctions

pref = os.path.join(scratch, "utf8-path-pref")
os.makedirs(pref, exist_ok=True)
env = dict(os.environ, BV2_PREF_DIR=pref)
env.pop("BV2_DATA_DIR", None)
try:
    r = subprocess.run([os.path.join(dest, os.path.basename(exe))], input=b"execute CTF\nquit\n", env=env, cwd=scratch,
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)
except subprocess.TimeoutExpired:
    print("FAIL: server did not quit within 30 s")
    sys.exit(1)
out = r.stdout.decode(errors="replace")
print(out[-2000:])
if "Server Created on port" not in out:  # printed only after main/, the launch script and the map loaded
    print("FAIL: the server did not find main/ under", dest)
    sys.exit(1)
