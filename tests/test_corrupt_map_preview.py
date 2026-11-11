#!/usr/bin/env python3
"""ROBO-195: the client reaches the menu with a map of unknown version in its maps/ (the Host tab previews every map).

Usage: test_corrupt_map_preview.py <bv2> <scratch dir>
"""
import os
import struct
import sys

from autotest_client import run_client

exe, scratch = sys.argv[1], sys.argv[2]
script = os.path.join(os.path.dirname(os.path.abspath(__file__)), "autotest", "corrupt-map.txt")
corrupt = struct.pack("<Ihh", 99999, 32, 32)  # a version no map loader knows
returncode, out = run_client(exe, os.path.join(scratch, "corrupt-map"), script, pref_files={"maps/zz-Corrupt.bvm": corrupt})  # sorts after CTF-Placeholder, whose g_size makes the loop read cells
print(out[-4000:])
if returncode != 0 or "autotest: PASS" not in out:
    print(f"FAIL: exit code {returncode}, expected 0 and autotest: PASS")
    sys.exit(1)
print("ok: menu built with a corrupt map in maps/")
