#!/usr/bin/env python3
"""Run the bv2 client on one autotest script (Phase T3) and check its exit code.

Usage: run_autotest.py <bv2> <scratch dir> <script> <expected exit code> <expected result text> [--timeout <s>]
The client gets a fresh pref dir, a windowed config and the placeholder data. On Linux without a display it runs
under its own Xvfb. 0 = pass, 1 = fail, 2 = timeout (ADR 0025); the result text names the path, so a test passes only by the path it is for.
"""
import os
import sys

from autotest_client import run_client

exe, scratch, script, expected, result = sys.argv[1], sys.argv[2], os.path.abspath(sys.argv[3]), int(sys.argv[4]), sys.argv[5]
pref = os.path.join(scratch, os.path.splitext(os.path.basename(script))[0])
returncode, out = run_client(exe, pref, script, extra_args=sys.argv[6:])
print(out[-4000:])
if returncode != expected:
    print(f"FAIL: exit code {returncode}, expected {expected}")
    sys.exit(1)
if result not in out:
    print(f"FAIL: no '{result}' in the output")
    sys.exit(1)
print(f"ok: {os.path.basename(script)} exited {expected}")
