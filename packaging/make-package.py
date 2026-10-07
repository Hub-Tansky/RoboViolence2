#!/usr/bin/env python3
"""Builds the play-test package for one preset from build/<preset>/runtime/.

Usage: make-package.py <runtime dir> <preset> <out dir>

Writes <out dir>/roboviolence2-<preset>.<tar.gz|zip> holding one folder with the client, bv2dedicated,
bv2master, main/ (links resolved), the generated databases and the run scripts from tools/.
The macOS bundle gets a real Contents/Resources/main instead of the build's symlink, then an ad-hoc signature.
"""
import os
import shutil
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent


def main():
    runtime, preset, out = Path(sys.argv[1]), sys.argv[2], Path(sys.argv[3])
    windows, macos = sys.platform == "win32", sys.platform == "darwin"
    exe = ".exe" if windows else ""
    name = f"roboviolence2-{preset}"
    stage = out / name
    shutil.rmtree(stage, ignore_errors=True)
    stage.mkdir(parents=True)

    # copytree follows symlinks and Windows junctions by default, so main/ arrives as real files.
    shutil.copytree(runtime / "main", stage / "main")
    for server in ("bv2dedicated", "bv2master"):
        shutil.copy2(runtime / (server + exe), stage)
    if macos:
        app = stage / "bv2.app"
        shutil.copytree(runtime / "bv2.app", app)  # Resources/main symlink becomes a copy
        subprocess.run(["codesign", "--force", "--sign", "-", str(app)], check=True)
    else:
        shutil.copy2(runtime / ("bv2" + exe), stage)
    for db in ("master.db", "web.db"):
        shutil.copy2(runtime / db, stage)
    for script in (REPO / "tools").glob("run-*" + (".cmd" if windows else ".sh")):
        shutil.copy2(script, stage)

    if windows:
        archive = shutil.make_archive(str(out / name), "zip", out, name)
    elif macos:
        archive = str(out / (name + ".zip"))
        subprocess.run(["ditto", "-c", "-k", "--keepParent", "--norsrc", "--noextattr", "--noqtn", str(stage), archive], check=True)  # keeps exec bits, no ._ files
    else:
        archive = str(out / (name + ".tar.gz"))
        subprocess.run(["tar", "-C", str(out), "-czf", archive, name], check=True)
    shutil.rmtree(stage)
    print(archive)


if __name__ == "__main__":
    main()
