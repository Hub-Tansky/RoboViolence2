"""Every place an OS shows the client's name says "Robo Violence 2", not the binary's name bv2.

macOS: Info.plist.in (menu bar) and the bundle name in the package and .dmg (Finder, Launchpad, Dock), which
run-client.sh must start. Windows: the exe's FileDescription/ProductName (Task Manager, jump list) and the
installer's AppName (Start menu, Apps list). Linux: the .desktop Name, matched to the window by StartupWMClass,
which must equal the SDL app id (X11 WM_CLASS, Wayland app id).
Python 3, stdlib only.
"""
import pathlib
import re
import sys

NAME = "Robo Violence 2"
root = pathlib.Path(__file__).resolve().parent.parent
packaging = root / "packaging"
failures = []


def check(ok, message):
    if not ok:
        failures.append(message)


def read(path):
    return (packaging / path).read_text(encoding="utf-8")


plist = read("macos/Info.plist.in")
for key in ("CFBundleName", "CFBundleDisplayName"):
    check(f"<key>{key}</key><string>{NAME}</string>" in plist, f"Info.plist.in: {key} is not {NAME}")
app = re.search(r'^APP = "([^"]+)"', read("make-package.py"), re.M)
check(app and app.group(1) == NAME + ".app", f"make-package.py: the bundle is not named {NAME}.app")
check(f'exec "{NAME}.app/Contents/MacOS/bv2"' in read("scripts/run-client.sh"),
      f"run-client.sh does not start {NAME}.app")

rc = read("windows/bv2.rc.in")
for key in ("FileDescription", "ProductName"):
    check(f'VALUE "{key}", "{NAME}"' in rc, f"bv2.rc.in: {key} is not {NAME}")
check(re.search(rf"^AppName={NAME}$", read("windows/roboviolence2.iss"), re.M), f"roboviolence2.iss: AppName is not {NAME}")

desktop = read("linux/roboviolence2.desktop")
check(re.search(rf"^Name={NAME}$", desktop, re.M), f"roboviolence2.desktop: Name is not {NAME}")
wm_class = re.search(r"^StartupWMClass=(.+)$", desktop, re.M)
app_id = re.search(r'SDL_SetHint\(SDL_HINT_APP_ID, "([^"]+)"\)', (root / "engine/zeven/src/dkw.cpp").read_text())
check(wm_class and app_id and wm_class.group(1) == app_id.group(1),
      "roboviolence2.desktop: StartupWMClass differs from the SDL app id in dkw.cpp")

for f in failures:
    print("FAIL", f)
if failures:
    sys.exit(1)
print("ok: the client is named", NAME, "on every OS")
