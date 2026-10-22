# 0030: One committed application icon for every OS; the .dmg shows only the app and Applications

- Status: Accepted
- Date: 2026-10-22
- Affects: `packaging/icons/`, `packaging/macos/`, `packaging/windows/`, `packaging/make-package.py`, `game/CMakeLists.txt`, `engine/zeven` (`dkwSetWindowIcon`), `tools/gen-app-icon.py`, `.github/workflows/build.yml`

## Context
- The client had only a generated placeholder `.ico` on Windows; the macOS bundle and the Linux window had no icon.
- The owner approved an icon on 2026-10-22: the website favicon (`website/img/favicon.svg`: dark tile, blue ball, red ball) with shading at large sizes.
- The `.dmg` used to hold the whole package folder (servers, scripts, `main/`, databases, licences), so its window showed a dozen icons. The owner asked for the standard macOS layout: the app, an arrow, the Applications folder.
- Each OS expects its own shape: macOS draws no mask, so the icon carries Apple's grid (824 px squircle in a 1024 px canvas) and drop shadow; Windows and Linux add their own effects, so their icon fills the canvas.

## Decision
- `tools/gen-app-icon.py` (numpy and Pillow, run by hand) renders the art; its outputs are committed in `packaging/icons/`: `roboviolence2.icns` (the ten types `iconutil` writes), `roboviolence2.ico` (16-256 px) and `hicolor/<n>x<n>/apps/roboviolence2.png` (the Linux hicolor tree, installed as is). 16-32 px keep the flat favicon shapes. The icon is new work, not an original BV2 asset.
- Windows: `bv2.rc.in` embeds the `.ico` in `bv2.exe`; the installer uses it too (`SetupIconFile`). Taskbar and Start menu take it from the exe.
- macOS: the `.icns` goes into `bv2.app/Contents/Resources`, named by `CFBundleIconFile`.
- Linux: no file format carries an icon, so the client compiles in the 128 px PNG (CMake writes `AppIcon.h`, target `bv2_app_icon`) and sets it with `SDL_SetWindowIcon` (`dkwSetWindowIcon`). Only a PNG that doesn't decode is an error; a compositor without window icons (GNOME on Wayland) takes the `.desktop` file's icon instead. `cmake --install` puts the hicolor tree in `share/icons` for the `.desktop` file.
- The `.dmg` (dmgbuild, pinned with hashes in `packaging/macos/requirements.txt`) holds `Robo Violence 2.app` and an `Applications` link over `packaging/macos/dmg-background(@2x).png`, with the icon as the volume icon. `packaging/macos/dmg-layout.json` holds the window size and icon positions for both the generator and `make-package.py`. The app carries `LICENSE.txt` and `licenses/`, since nothing else in the image does. `make-package.py` mounts the result and fails unless exactly those two items show and the signature verifies. The servers stay in the macOS `.zip` package.
- In the image the bundle is named `Robo Violence 2.app` (what Finder, Launchpad and the Dock show); the package folder keeps `bv2.app` for the run scripts.

## Consequences
- Changing the art means running `tools/gen-app-icon.py` and committing its outputs; `tests/test_app_icon.py` checks the sizes, the wiring and the background size; `tests/test_window_icon.cpp` decodes the compiled-in icon; `install_hicolor_icons` installs the tree.
- The generator needs the Inter font (Debian/Ubuntu path by default, `--font` elsewhere) so a re-run reproduces the caption.
- macOS packaging needs dmgbuild: CI installs it into a venv; a local `make-package.py` run on a Mac needs `pip install --require-hashes -r packaging/macos/requirements.txt` first.
- Finder in dark mode draws icon labels in white, which is faint on the light background; that is how most `.dmg` windows look, and the arrow and caption carry the instruction.
- Rejected: generating the icons at build time (the build would need numpy and Pillow on every OS); Finder AppleScript or `create-dmg` for the layout (drives a GUI Finder, which times out on CI runners); one icon shape for all OSes (wrong on macOS or too small elsewhere).
