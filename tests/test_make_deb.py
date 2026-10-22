"""packaging/make_deb.py builds a .deb that apt can install: FHS layout, modes, version mapping, Depends (ADR 0031).

Builds the package from this build's `cmake --install` in a scratch folder and reads it back with dpkg-deb; its
share/bv2/main must hold exactly what runtime/main holds, so the install rules and the packages can't drift apart.
Linux client builds with dpkg-dev only. Usage: test_make_deb.py <build dir> <scratch dir>
"""
import importlib.util
import os
import pathlib
import shutil
import subprocess
import sys

root = pathlib.Path(__file__).resolve().parent.parent
spec = importlib.util.spec_from_file_location("make_deb", root / "packaging" / "make_deb.py")
make_deb = importlib.util.module_from_spec(spec)
spec.loader.exec_module(make_deb)
failures = []


def check(ok, message):
    if not ok:
        failures.append(message)


def dpkg_deb(*args):
    return subprocess.run(["dpkg-deb", *map(str, args)], capture_output=True, text=True, check=True).stdout


# Versions: tags and the dev build map to versions dpkg orders as releases are ordered
for tag, expected in (("v0.0.2", "0.0.2"), ("v0.1.0-rc1", "0.1.0~rc1"), ("0.0.0-dev", "0.0.0~dev")):
    check(make_deb.deb_version(tag) == expected, f"deb_version({tag!r}) is {make_deb.deb_version(tag)!r}, not {expected!r}")
try:
    make_deb.deb_version("vX")
    failures.append("deb_version('vX') accepted a version dpkg refuses")
except SystemExit:
    pass
older = subprocess.run(["dpkg", "--compare-versions", make_deb.deb_version("v0.1.0-rc1"), "lt", make_deb.deb_version("v0.1.0")])
check(older.returncode == 0, "0.1.0~rc1 does not sort before 0.1.0")

build, scratch = pathlib.Path(sys.argv[1]), pathlib.Path(sys.argv[2]) / "make_deb"
shutil.rmtree(scratch, ignore_errors=True)
(scratch / "licenses").mkdir(parents=True)
(scratch / "licenses" / "glad.txt").write_text("stand-in\n")
deb = make_deb.build_deb(build, scratch / "licenses", scratch / "test.deb", "v0.1.0-rc1")

entries = {}
for line in dpkg_deb("-c", deb).splitlines():  # "-rwxr-xr-x root/root 1234 2026-10-22 12:00 ./usr/games/bv2"
    fields = line.split(None, 5)
    entries[fields[5].removeprefix(".")] = (fields[0], fields[1])
for path, mode in (("/usr/games/bv2", "-rwxr-xr-x"), ("/usr/games/bv2dedicated", "-rwxr-xr-x"),
                   ("/usr/share/bv2/main/LaunchScript/CTF.cfg", "-rw-r--r--"),
                   ("/usr/share/applications/roboviolence2.desktop", "-rw-r--r--"),
                   ("/usr/share/icons/hicolor/128x128/apps/roboviolence2.png", "-rw-r--r--"),
                   ("/usr/share/doc/roboviolence2/copyright", "-rw-r--r--"),
                   ("/usr/share/doc/roboviolence2/changelog.gz", "-rw-r--r--"),
                   ("/usr/share/doc/roboviolence2/licenses/glad.txt", "-rw-r--r--")):
    check(entries.get(path, ("missing",))[0] == mode, f"{path}: {entries.get(path, ('missing',))[0]}, expected {mode}")
check(all(owner == "root/root" for _, owner in entries.values()), "files not owned by root/root")
check(not any("bv2master" in p for p in entries), "bv2master is in the .deb (it belongs to the .tar.gz)")
runtime_main = build / "runtime" / "main"
expected = {"/usr/share/bv2/main/" + (pathlib.Path(d) / f).relative_to(runtime_main).as_posix()
            for d, _, files in os.walk(runtime_main, followlinks=True) for f in files}  # through the links to content/
installed = {p for p, (mode, _) in entries.items() if p.startswith("/usr/share/bv2/main/") and not mode.startswith("d")}
check(installed == expected, f"share/bv2/main differs from runtime/main: missing {sorted(expected - installed)}, "
                             f"extra {sorted(installed - expected)}")

check(dpkg_deb("-f", deb, "Package").strip() == "roboviolence2", "Package is not roboviolence2")
check(dpkg_deb("-f", deb, "Version").strip() == "0.1.0~rc1", "Version is not 0.1.0~rc1")
depends = dpkg_deb("-f", deb, "Depends")
check("libc6" in depends and "libgl1" in depends, f"Depends lacks libc6 or libgl1: {depends}")

unpacked = scratch / "unpacked"
dpkg_deb("-x", deb, unpacked)
desktop = (unpacked / "usr/share/applications/roboviolence2.desktop").read_text()
check("\nExec=/usr/games/bv2\n" in desktop, ".desktop does not start /usr/games/bv2")

if failures:
    sys.exit("FAIL " + "\nFAIL ".join(failures))
print("ok: .deb layout, modes, version, Depends and launcher")
