#!/usr/bin/env python3
"""Builds the play-test package for one preset from build/<preset>/runtime/.

Usage: make-package.py <runtime dir> <preset> <out dir>

Writes <out dir>/roboviolence2-<preset>.<zip|tar.gz> holding one folder with the client, bv2dedicated,
bv2master, main/ (links resolved), the generated databases and the run scripts from packaging/scripts/.
The macOS bundle gets a real Contents/Resources/main instead of the build's symlink, then an ad-hoc signature;
the servers read the folder's own main/ (docs/build/macos.md).
Fails if the build is not Release, or if anything the package must hold is missing.
"""
import shutil
import subprocess
import sys
from pathlib import Path

SCRIPTS = Path(__file__).resolve().parent / "scripts"


def main():
    runtime, preset, out = Path(sys.argv[1]), sys.argv[2], Path(sys.argv[3])
    # Players get Release only: Debug starts windowed and, on Windows, carries the debug C++ runtime.
    if "CMAKE_BUILD_TYPE:STRING=Release" not in (runtime.parent / "CMakeCache.txt").read_text().splitlines():
        sys.exit(f"{runtime.parent} is not a Release build")
    windows, macos = sys.platform == "win32", sys.platform == "darwin"
    exe, script_ext = (".exe", ".cmd") if windows else ("", ".sh")
    name = f"roboviolence2-{preset}"
    stage = out / name
    shutil.rmtree(stage, ignore_errors=True)
    stage.mkdir(parents=True)

    # copytree follows symlinks and Windows junctions by default, so main/ arrives as real files.
    shutil.copytree(runtime / "main", stage / "main")
    if macos:
        shutil.copytree(runtime / "bv2.app", stage / "bv2.app")  # Resources/main symlink becomes a copy
        subprocess.run(["codesign", "--force", "--sign", "-", str(stage / "bv2.app")], check=True)
        client = "bv2.app/Contents/MacOS/bv2"
    else:
        client = "bv2" + exe
        shutil.copy2(runtime / client, stage)
    servers_and_dbs = ["bv2dedicated" + exe, "bv2master" + exe, "master.db", "web.db"]
    for f in servers_and_dbs:
        shutil.copy2(runtime / f, stage)
    scripts = [f"run-{s}{script_ext}" for s in ("client", "server", "master")]
    for s in scripts:
        shutil.copy2(SCRIPTS / s, stage)  # raises if a script was renamed or removed

    required = [client, *servers_and_dbs, *scripts, "main/LaunchScript/CTF.cfg", "main/languages/en.lang"]
    missing = [f for f in required if not (stage / f).is_file()]
    if missing:
        sys.exit(f"package {name} is missing: {', '.join(missing)}")

    if macos:  # ditto keeps exec bits and the bundle signature, and writes no ._ files
        archive = str(out / (name + ".zip"))
        subprocess.run(["ditto", "-c", "-k", "--keepParent", "--norsrc", "--noextattr", "--noqtn", str(stage), archive], check=True)
    else:  # gztar keeps file modes for Linux; zip is what Windows users expect
        archive = shutil.make_archive(str(out / name), "zip" if windows else "gztar", out, name)
    shutil.rmtree(stage)
    print(archive)


if __name__ == "__main__":
    main()
