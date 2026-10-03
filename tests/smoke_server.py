#!/usr/bin/env python3
"""Start bv2dedicated headless on the placeholder content, run a launch script, quit.

Usage: smoke_server.py <bv2dedicated> <scratch pref dir>
Fails if the server crashes, hangs, or does not print its banner.
"""
import os
import subprocess
import sys

exe, pref = sys.argv[1], sys.argv[2]
os.makedirs(pref, exist_ok=True)
env = dict(os.environ, BV2_PREF_DIR=pref)
try:
    r = subprocess.run([exe], input=b"execute CTF\nquit\n", env=env, cwd=os.path.dirname(os.path.abspath(exe)),
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)
except subprocess.TimeoutExpired:
    print("FAIL: server did not quit within 30 s")
    sys.exit(1)
out = r.stdout.decode(errors="replace")
print(out[-2000:])
if r.returncode != 0:
    print("FAIL: exit code", r.returncode)
    sys.exit(1)
if "Dedicated Server" not in out:
    print("FAIL: no banner")
    sys.exit(1)
if not os.path.isfile(os.path.join(pref, "bv2.cfg")):
    print("FAIL: bv2.cfg was not created in the pref dir")
    sys.exit(1)
print("ok: dedicated server smoke")
