#!/usr/bin/env python3
"""Run the bv2 client on one autotest script (Phase T3) and check its exit code.

Usage: run_autotest.py <bv2> <scratch dir> <script> <expected exit code> <expected result text> [--timeout <s>]
The client gets a fresh pref dir, a windowed config and the placeholder data. On Linux without a display it runs
under its own Xvfb. 0 = pass, 1 = fail, 2 = timeout (ADR 0025); the result text names the path, so a test passes only by the path it is for.
"""
import os
import shutil
import subprocess
import sys

exe, scratch, script, expected, result = sys.argv[1], sys.argv[2], os.path.abspath(sys.argv[3]), int(sys.argv[4]), sys.argv[5]
pref = os.path.join(scratch, os.path.splitext(os.path.basename(script))[0])
shutil.rmtree(pref, ignore_errors=True)
os.makedirs(pref)
config = os.path.join(pref, "autotest.cfg")
with open(config, "w") as f:
    f.write("r_fullScreen false\n")
cmd = [exe, "--placeholders", "--config", config, "--autotest", script] + sys.argv[6:]
env = dict(os.environ, BV2_PREF_DIR=pref, SDL_AUDIODRIVER="dummy")
xvfb = None
if sys.platform.startswith("linux") and not os.environ.get("DISPLAY"):
    # Our own Xvfb, not xvfb-run: xvfb-run replaces the client's exit code with 5 when its cleanup fails
    read, write = os.pipe()
    xvfb = subprocess.Popen(["Xvfb", "-displayfd", str(write), "-screen", "0", "1280x720x24", "-nolisten", "tcp"],
                            pass_fds=[write], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    os.close(write)
    with os.fdopen(read) as f:
        env["DISPLAY"] = ":" + f.readline().strip()
try:
    r = subprocess.run(cmd, env=env, cwd=os.path.dirname(os.path.abspath(exe)), stdout=subprocess.PIPE,
                       stderr=subprocess.STDOUT, timeout=180)
finally:
    if xvfb:
        xvfb.terminate()
        xvfb.wait()
out = r.stdout.decode(errors="replace")
print(out[-4000:])
if r.returncode != expected:
    print(f"FAIL: exit code {r.returncode}, expected {expected}")
    sys.exit(1)
if result not in out:
    print(f"FAIL: no '{result}' in the output")
    sys.exit(1)
print(f"ok: {os.path.basename(script)} exited {expected}")
