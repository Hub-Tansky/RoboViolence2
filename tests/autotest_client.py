"""Shared by run_autotest.py and e2e/e2e.py: an Xvfb display and the `bv2 --autotest` command line (ADR 0025)."""
import os
import subprocess


def start_xvfb():
    """Starts our own Xvfb; returns (process, DISPLAY). Not xvfb-run: it replaces the client's exit code with 5 when its cleanup fails"""
    read, write = os.pipe()  # Xvfb writes its display number here
    proc = subprocess.Popen(["Xvfb", "-displayfd", str(write), "-screen", "0", "1280x720x24", "-nolisten", "tcp"],
                            pass_fds=[write], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    os.close(write)
    with os.fdopen(read) as f:
        return proc, ":" + f.readline().strip()


def client(exe, pref, script, config_lines=""):
    """Command line and env for `exe --autotest script` with pref dir `pref`, windowed, on the placeholder data"""
    config = os.path.join(pref, "autotest.cfg")
    with open(config, "w") as f:
        f.write("r_fullScreen false\n" + config_lines)
    cmd = [exe, "--placeholders", "--config", config, "--autotest", script]
    return cmd, dict(os.environ, BV2_PREF_DIR=pref, SDL_AUDIODRIVER="dummy")
