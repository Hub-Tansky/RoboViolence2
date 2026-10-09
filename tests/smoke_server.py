#!/usr/bin/env python3
"""Start bv2dedicated headless on the placeholder content, run a launch script, quit.

Usage: smoke_server.py <bv2dedicated> <scratch pref dir> [--install-under <dir>]
Fails if the server crashes, hangs, exits non-zero, doesn't create its server (the "Server Created on port" line
comes after main/, the launch script and the map loaded) or doesn't save bv2.cfg in the pref dir.

--install-under <dir> (Windows, ADR 0012): copy the executable and main/ into <dir>/Łódź/ and start it from <dir>,
so only the executable's own directory, a narrow UTF-8 path in game/src/Paths.cpp, can lead to main/.
"""
import os
import shutil
import subprocess
import sys

exe, pref = sys.argv[1], sys.argv[2]
cwd = os.path.dirname(os.path.abspath(exe))
env = dict(os.environ, BV2_PREF_DIR=pref)
if len(sys.argv) == 5 and sys.argv[3] == "--install-under":
    base = sys.argv[4]
    shutil.rmtree(base, ignore_errors=True)
    install = os.path.join(base, "Łódź")  # Łódź
    os.makedirs(install)
    shutil.copy2(exe, install)
    shutil.copytree(os.path.join(cwd, "main"), os.path.join(install, "main"))  # follows junctions
    exe, cwd = os.path.join(install, os.path.basename(exe)), base
    env.pop("BV2_DATA_DIR", None)
    # Paths.cpp also tries the working directory and <exe dir>/../share/bv2: neither may hold main/
    assert not os.path.exists(os.path.join(base, "main")) and not os.path.exists(os.path.join(base, "share"))
os.makedirs(pref, exist_ok=True)
try:
    r = subprocess.run([exe], input=b"execute CTF\nquit\n", env=env, cwd=cwd,
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)
except subprocess.TimeoutExpired:
    print("FAIL: server did not quit within 30 s")
    sys.exit(1)
out = r.stdout.decode(errors="replace")
print(out[-2000:])
if r.returncode != 0:
    print("FAIL: exit code", r.returncode)
    sys.exit(1)
if "Server Created on port" not in out:
    print("FAIL: no 'Server Created on port' (main/ not found, or the launch script or map failed) for", ascii(exe))
    sys.exit(1)
if not os.path.isfile(os.path.join(pref, "bv2.cfg")):
    print("FAIL: bv2.cfg was not created in the pref dir")
    sys.exit(1)
print("ok: dedicated server smoke")
