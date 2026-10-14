#!/usr/bin/env python3
"""Builds the package for one preset from build/<preset>/runtime/ (ADR 0018).

Usage: make-package.py <runtime dir> <preset> <out dir> <version>

Writes <out dir>/roboviolence2-<preset>.<zip|tar.gz> holding one folder with the client, bv2dedicated,
bv2master, main/ (links resolved), the generated databases, the run scripts from packaging/scripts/, LICENSE.txt
and licenses/<port>.txt for every vcpkg port and glad.
The macOS bundle gets a real Contents/Resources/main instead of the build's symlink, then an ad-hoc signature;
the servers read the folder's own main/ (docs/build/macos.md).
Also writes roboviolence2-<preset>-setup.exe on Windows (Inno Setup, packaging/windows/roboviolence2.iss) and
roboviolence2-<preset>.dmg on macOS. <version> (e.g. a v1.2.0 tag or 0.0.0-dev) goes into the installer.
Fails if the build is not Release, or if anything the package must hold is missing.
"""
import os
import shutil
import subprocess
import sys
from pathlib import Path

PACKAGING = Path(__file__).resolve().parent
SCRIPTS = PACKAGING / "scripts"
GLAD = PACKAGING.parent / "engine" / "zeven" / "third_party" / "glad"
ISCC = r"C:\Program Files (x86)\Inno Setup 6\ISCC.exe"  # its default install, as on the GitHub Windows runner


def run(*cmd):
    subprocess.run([str(c) for c in cmd], check=True)


# Per OS: put the client into the stage (returns its path in the package), then ship the stage.

def client_macos(runtime, stage):
    shutil.copytree(runtime / "bv2.app", stage / "bv2.app")  # Resources/main symlink becomes a copy
    run("codesign", "--force", "--sign", "-", stage / "bv2.app")
    return "bv2.app/Contents/MacOS/bv2"


def client_binary(exe):
    def stage_client(runtime, stage):
        shutil.copy2(runtime / ("bv2" + exe), stage)
        return "bv2" + exe
    return stage_client


def ship_macos(stage, out, name, version):
    # ditto keeps exec bits and the bundle signature, and writes no ._ files
    run("ditto", "-c", "-k", "--keepParent", "--norsrc", "--noextattr", "--noqtn", stage, out / (name + ".zip"))
    os.symlink("/Applications", stage / "Applications")  # in the .dmg only, so bv2.app can be dragged across
    run("hdiutil", "create", "-quiet", "-volname", "Robo Violence 2", "-srcfolder", stage, "-ov", "-format", "UDZO",
        out / (name + ".dmg"))


def ship_windows(stage, out, name, version):
    shutil.make_archive(str(out / name), "zip", out, name)
    run(ISCC, "/Q", f"/DSourceDir={stage.resolve()}", f"/DOutputDir={out.resolve()}", f"/DOutputName={name}-setup",
        f"/DAppVersion={version.removeprefix('v')}", PACKAGING / "windows" / "roboviolence2.iss")


def ship_linux(stage, out, name, version):
    shutil.make_archive(str(out / name), "gztar", out, name)  # keeps file modes


PLATFORMS = {  # sys.platform: (executable suffix, script suffix, client, ship)
    "darwin": ("", ".sh", client_macos, ship_macos),
    "win32": (".exe", ".cmd", client_binary(".exe"), ship_windows),
    "linux": ("", ".sh", client_binary(""), ship_linux),
}


def stage_licences(runtime, stage):
    """LICENSE.txt, each vcpkg port's copyright (helper ports vcpkg-* ship no code) and glad's notice"""
    shutil.copy2(PACKAGING.parent / "LICENSE.txt", stage)
    licences = stage / "licenses"
    licences.mkdir()
    for f in sorted((runtime.parent / "vcpkg_installed").glob("*/share/*/copyright")):
        if not f.parent.name.startswith("vcpkg-"):
            shutil.copy2(f, licences / (f.parent.name + ".txt"))
    khr = (GLAD / "include" / "KHR" / "khrplatform.h").read_text(encoding="utf-8")
    (licences / "glad.txt").write_text("glad 2.0.8 generated loader: WTFPL OR CC0-1.0.\n\nKHR/khrplatform.h:\n"
                                       + khr[khr.index("/*"):khr.index("*/") + 2] + "\n", encoding="utf-8")


def main():
    runtime, preset, out, version = Path(sys.argv[1]), sys.argv[2], Path(sys.argv[3]), sys.argv[4]
    # Players get Release only: Debug starts windowed and, on Windows, carries the debug C++ runtime.
    if "CMAKE_BUILD_TYPE:STRING=Release" not in (runtime.parent / "CMakeCache.txt").read_text().splitlines():
        sys.exit(f"{runtime.parent} is not a Release build")
    exe, script_ext, stage_client, ship = PLATFORMS[sys.platform]
    name = f"roboviolence2-{preset}"
    stage = out / name
    shutil.rmtree(stage, ignore_errors=True)
    stage.mkdir(parents=True)

    # copytree follows symlinks and Windows junctions by default, so main/ arrives as real files.
    shutil.copytree(runtime / "main", stage / "main")
    client = stage_client(runtime, stage)
    servers_and_dbs = ["bv2dedicated" + exe, "bv2master" + exe, "master.db", "web.db"]
    for f in servers_and_dbs:
        shutil.copy2(runtime / f, stage)
    scripts = [f"run-{s}{script_ext}" for s in ("client", "server", "master")]
    for s in scripts:
        shutil.copy2(SCRIPTS / s, stage)  # raises if a script was renamed or removed
    stage_licences(runtime, stage)

    # sqlite3 is a dependency on every OS: its licence proves the vcpkg glob found the ports
    required = [client, *servers_and_dbs, *scripts, "main/LaunchScript/CTF.cfg", "main/languages/en.lang",
                "LICENSE.txt", "licenses/sqlite3.txt", "licenses/glad.txt"]
    missing = [f for f in required if not (stage / f).is_file()]
    if missing:
        sys.exit(f"package {name} is missing: {', '.join(missing)}")

    ship(stage, out, name, version)
    shutil.rmtree(stage)
    for f in sorted(out.glob(name + "*")):
        print(f)


if __name__ == "__main__":
    main()
