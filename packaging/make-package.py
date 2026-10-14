#!/usr/bin/env python3
"""Builds the package for one preset from build/<preset>/runtime/ (ADR 0018).

Usage: make-package.py <runtime dir> <preset> <out dir> [<version>]

Writes <out dir>/roboviolence2-<preset>.<zip|tar.gz> holding one folder with the client, bv2dedicated,
bv2master, main/ (links resolved), the generated databases, the run scripts from packaging/scripts/, LICENSE.txt
and licenses/<port>.txt for every vcpkg port.
The macOS bundle gets a real Contents/Resources/main instead of the build's symlink, then an ad-hoc signature;
the servers read the folder's own main/ (docs/build/macos.md).
Also writes roboviolence2-<preset>-setup.exe on Windows (Inno Setup, packaging/windows/roboviolence2.iss) and
roboviolence2-<preset>.dmg on macOS. <version> (default 0.0.0-dev) goes into the installer.
Fails if the build is not Release, or if anything the package must hold is missing.
"""
import os
import shutil
import subprocess
import sys
from pathlib import Path

PACKAGING = Path(__file__).resolve().parent
SCRIPTS = PACKAGING / "scripts"
ISCC = r"C:\Program Files (x86)\Inno Setup 6\ISCC.exe"  # its default install, as on the GitHub Windows runner


def main():
    runtime, preset, out = Path(sys.argv[1]), sys.argv[2], Path(sys.argv[3])
    version = sys.argv[4] if len(sys.argv) > 4 else "0.0.0-dev"
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

    # Licences: ours, then each vcpkg port's (the helper ports vcpkg-* ship no code)
    shutil.copy2(PACKAGING.parent / "LICENSE.txt", stage)
    (stage / "licenses").mkdir()
    for f in sorted((runtime.parent / "vcpkg_installed").glob("*/share/*/copyright")):
        if not f.parent.name.startswith("vcpkg-"):
            shutil.copy2(f, stage / "licenses" / (f.parent.name + ".txt"))

    required = [client, *servers_and_dbs, *scripts, "main/LaunchScript/CTF.cfg", "main/languages/en.lang",
                "LICENSE.txt", "licenses/sqlite3.txt"]
    missing = [f for f in required if not (stage / f).is_file()]
    if missing:
        sys.exit(f"package {name} is missing: {', '.join(missing)}")

    if macos:  # ditto keeps exec bits and the bundle signature, and writes no ._ files
        archive = str(out / (name + ".zip"))
        subprocess.run(["ditto", "-c", "-k", "--keepParent", "--norsrc", "--noextattr", "--noqtn", str(stage), archive], check=True)
    else:  # gztar keeps file modes for Linux; zip is what Windows users expect
        archive = shutil.make_archive(str(out / name), "zip" if windows else "gztar", out, name)
    print(archive)
    if windows:
        subprocess.run([ISCC, "/Q", f"/DSourceDir={stage.resolve()}", f"/DOutputDir={out.resolve()}",
                        f"/DOutputName={name}-setup", f"/DAppVersion={version}",
                        str(PACKAGING / "windows" / "roboviolence2.iss")], check=True)
        print(out / (name + "-setup.exe"))
    if macos:  # the folder plus an Applications link, so bv2.app can be dragged across
        os.symlink("/Applications", stage / "Applications")
        dmg = out / (name + ".dmg")
        subprocess.run(["hdiutil", "create", "-quiet", "-volname", "Robo Violence 2", "-srcfolder", str(stage),
                        "-ov", "-format", "UDZO", str(dmg)], check=True)
        print(dmg)
    shutil.rmtree(stage)


if __name__ == "__main__":
    main()
