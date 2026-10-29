#!/usr/bin/env python3
"""Builds the package for one preset from build/<preset>/runtime/ (ADR 0020).

Usage: make-package.py <runtime dir> <preset> <out dir> <version>

Writes <out dir>/roboviolence2-<preset>.<zip|tar.gz> holding one folder with the client, bv2dedicated,
bv2master, main/ (links resolved), the generated databases, the run scripts from packaging/scripts/, LICENSE.txt
and licenses/<port>.txt for every vcpkg port and glad.
The macOS bundle gets a real Contents/Resources/main instead of the build's symlink and its own copy of the
licences, then an ad-hoc signature; the servers read the folder's own main/ (docs/build/macos.md).
Also writes roboviolence2-<preset>-setup.exe on Windows (Inno Setup, packaging/windows/roboviolence2.iss) and
roboviolence2-<preset>.dmg on macOS: only the app and an Applications link, laid out over
packaging/macos/dmg-background.png (dmgbuild, ADR 0030), and roboviolence2-<preset>.deb on Linux (dpkg-deb, ADR 0031).
<version> (e.g. a v1.2.0 tag or 0.0.0-dev) goes into the installer and the .deb.
Fails if the build is not Release, or if anything the package must hold is missing.
"""
import json
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Callable, NamedTuple

from make_deb import build_deb

PACKAGING = Path(__file__).resolve().parent
SCRIPTS = PACKAGING / "scripts"
GLAD = PACKAGING.parent / "engine" / "zeven" / "third_party" / "glad"
ISCC = r"C:\Program Files (x86)\Inno Setup 6\ISCC.exe"  # its default install, as on the GitHub Windows runner
APP = "Robo Violence 2.app"  # the bundle's name in the package and the .dmg: what Finder, Launchpad and the Dock show


def run(*cmd):
    subprocess.run([str(c) for c in cmd], check=True)


# Per OS: put the client into the stage (returns its path in the package), then ship the stage (returns the files).

def client_macos(runtime, stage, exe):
    shutil.copytree(runtime / "bv2.app", stage / APP)  # Resources/main symlink becomes a copy
    return APP + "/Contents/MacOS/bv2"


def client_binary(runtime, stage, exe):
    shutil.copy2(runtime / ("bv2" + exe), stage)
    return "bv2" + exe


def ship_macos(runtime, stage, out, name, version):
    from dmgbuild import build_dmg  # pip, macOS packaging only (build.yml installs it)

    archive, dmg = out / (name + ".zip"), out / (name + ".dmg")
    app = stage / APP
    # The .dmg holds only the app, so the app carries the licences; signed last, after every change to the bundle
    shutil.copy2(stage / "LICENSE.txt", app / "Contents" / "Resources")
    shutil.copytree(stage / "licenses", app / "Contents" / "Resources" / "licenses")
    run("codesign", "--force", "--sign", "-", app)
    # ditto keeps exec bits and the bundle signature, and writes no ._ files
    run("ditto", "-c", "-k", "--keepParent", "--norsrc", "--noextattr", "--noqtn", stage, archive)
    # The same layout tools/gen-app-icon.py drew the background for
    layout = json.loads((PACKAGING / "macos" / "dmg-layout.json").read_text())
    dmg.unlink(missing_ok=True)
    build_dmg(str(dmg), "Robo Violence 2", settings={
        "format": "UDZO",
        "files": [(str(app), APP)],
        "symlinks": {"Applications": "/Applications"},
        "icon": str(PACKAGING / "icons" / "roboviolence2.icns"),  # the mounted volume's icon
        "background": str(PACKAGING / "macos" / "dmg-background.png"),  # dmgbuild adds the @2x file
        "window_rect": ((200, 120), tuple(layout["window"])),
        "icon_size": 128,
        "text_size": 13,
        "icon_locations": {APP: tuple(layout["app"]), "Applications": tuple(layout["applications"])},
    })
    check_dmg(dmg)
    return [archive, dmg]


def check_dmg(dmg):
    """Mount the image; fail unless it shows exactly the app and the Applications link, has the hidden files of
    the window settings, background and volume icon, and the app's signature verifies"""
    with tempfile.TemporaryDirectory() as tmp:
        mount = Path(tmp)
        run("hdiutil", "attach", "-quiet", "-nobrowse", "-readonly", "-mountpoint", mount, dmg)
        try:
            visible = sorted(p.name for p in mount.iterdir() if not p.name.startswith("."))
            if visible != sorted([APP, "Applications"]):
                sys.exit(f"{dmg.name} shows {visible}")
            if (mount / "Applications").readlink() != Path("/Applications"):
                sys.exit(f"{dmg.name}: Applications links to {(mount / 'Applications').readlink()}")
            missing = [f for f in (".DS_Store", ".VolumeIcon.icns", ".background.tiff") if not (mount / f).is_file()]
            if missing:
                sys.exit(f"{dmg.name} lacks {', '.join(missing)}")
            if not (mount / APP / "Contents" / "Resources" / "roboviolence2.icns").is_file():
                sys.exit(f"{dmg.name}: the app has no icon")
            run("codesign", "--verify", "--strict", mount / APP)
        finally:
            run("hdiutil", "detach", "-quiet", mount)


def reflow_licence(text):
    """Joins each paragraph's hard-wrapped lines into one, so the installer's text box wraps them to its own width.
    Centred headings (indented 10+ spaces) keep their own lines."""
    paragraphs = []
    for paragraph in text.split("\n\n"):
        lines = []
        for line in paragraph.splitlines():
            heading = line.startswith(" " * 10)
            if lines and not heading and not lines[-1].startswith(" " * 10):
                lines[-1] += " " + line.strip()
            else:
                lines.append(line.rstrip())
        paragraphs.append("\n".join(lines))
    return "\n\n".join(paragraphs)


def ship_windows(runtime, stage, out, name, version):
    archive = shutil.make_archive(str(out / name), "zip", out, name)
    with tempfile.TemporaryDirectory() as tmp:
        # The installer shows a reflowed copy; the package keeps LICENSE.txt verbatim
        info = Path(tmp) / "LICENSE.txt"
        info.write_text(reflow_licence((stage / "LICENSE.txt").read_text(encoding="utf-8")), encoding="utf-8")
        run(ISCC, "/Q", f"/DSourceDir={stage.resolve()}", f"/DInfoFile={info}", f"/DOutputDir={out.resolve()}",
            f"/DOutputName={name}-setup", f"/DAppVersion={version.removeprefix('v')}",
            PACKAGING / "windows" / "roboviolence2.iss")
    return [archive, out / (name + "-setup.exe")]


def ship_linux(runtime, stage, out, name, version):
    archive = shutil.make_archive(str(out / name), "gztar", out, name)  # keeps file modes
    return [archive, build_deb(runtime.parent, stage / "licenses", out / (name + ".deb"), version)]


class Platform(NamedTuple):
    exe: str
    script: str
    stage_client: Callable
    ship: Callable


PLATFORMS = {
    "darwin": Platform("", ".sh", client_macos, ship_macos),
    "win32": Platform(".exe", ".cmd", client_binary, ship_windows),
    "linux": Platform("", ".sh", client_binary, ship_linux),
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
    platform = PLATFORMS[sys.platform]
    exe = platform.exe
    name = f"roboviolence2-{preset}"
    stage = out / name
    shutil.rmtree(stage, ignore_errors=True)
    stage.mkdir(parents=True)

    # copytree follows symlinks and Windows junctions by default, so main/ arrives as real files.
    shutil.copytree(runtime / "main", stage / "main")
    client = platform.stage_client(runtime, stage, exe)
    servers_and_dbs = ["bv2dedicated" + exe, "bv2master" + exe, "master.db", "web.db"]
    for f in servers_and_dbs:
        shutil.copy2(runtime / f, stage)
    scripts = [f"run-{s}{platform.script}" for s in ("client", "server", "master")]
    for s in scripts:
        shutil.copy2(SCRIPTS / s, stage)  # raises if a script was renamed or removed
    stage_licences(runtime, stage)

    # sqlite3 is a dependency on every OS: its licence proves the vcpkg glob found the ports
    required = [client, *servers_and_dbs, *scripts, "main/LaunchScript/CTF.cfg", "main/languages/en.lang",
                "LICENSE.txt", "licenses/sqlite3.txt", "licenses/glad.txt"]
    missing = [f for f in required if not (stage / f).is_file()]
    if missing:
        sys.exit(f"package {name} is missing: {', '.join(missing)}")

    written = platform.ship(runtime, stage, out, name, version)
    shutil.rmtree(stage)
    for f in written:
        print(f)


if __name__ == "__main__":
    main()
