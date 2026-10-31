"""Shared by run_autotest.py, screenshots.py and e2e/e2e.py: an Xvfb display and the `bv2 --autotest` command line (ADR 0025)."""
import os
import shutil
import subprocess
import sys


def start_xvfb():
    """Starts our own Xvfb; returns (process, DISPLAY). Not xvfb-run: it replaces the client's exit code with 5 when its cleanup fails"""
    read, write = os.pipe()  # Xvfb writes its display number here
    proc = subprocess.Popen(["Xvfb", "-displayfd", str(write), "-screen", "0", "1280x720x24", "-nolisten", "tcp"],
                            pass_fds=[write], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    os.close(write)
    with os.fdopen(read) as f:
        return proc, ":" + f.readline().strip()


def client_command(exe, pref, script, config_lines=""):
    """Command line and env for `exe --autotest script` with pref dir `pref`, windowed (1024 x 768), on placeholders, with no
    master (BV2_MASTER_SERVERS=none, ADR 0028; the caller may replace it in the env)"""
    config = os.path.join(pref, "autotest.cfg")
    with open(config, "w") as f:
        f.write("r_fullScreen false\nr_resolution 1024 768\n" + config_lines)  # the goldens are 1024 x 768 frames
    cmd = [exe, "--placeholders", "--config", config, "--autotest", script]
    return cmd, dict(os.environ, BV2_PREF_DIR=pref, SDL_AUDIODRIVER="dummy", BV2_MASTER_SERVERS="none")


def run_client(exe, pref, script, config_lines="", extra_args=(), own_xvfb=False):
    """Runs `exe --autotest script` with a fresh pref dir; returns (exit code, output). On Linux it gets its own Xvfb when
    there is no display, or always with own_xvfb (and SDL on X11), so the frame doesn't depend on the desktop"""
    shutil.rmtree(pref, ignore_errors=True)
    os.makedirs(pref)
    cmd, env = client_command(exe, pref, script, config_lines)
    cmd += list(extra_args)
    xvfb = None
    if sys.platform.startswith("linux") and (own_xvfb or not os.environ.get("DISPLAY")):
        xvfb, env["DISPLAY"] = start_xvfb()
        env["SDL_VIDEO_DRIVER"] = "x11"
    try:
        r = subprocess.run(cmd, env=env, cwd=os.path.dirname(os.path.abspath(exe)), stdout=subprocess.PIPE,
                           stderr=subprocess.STDOUT, timeout=180)
    finally:
        if xvfb:
            xvfb.terminate()
            xvfb.wait()
    return r.returncode, r.stdout.decode(errors="replace")
