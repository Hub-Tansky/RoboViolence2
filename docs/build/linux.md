# Linux (x64; reference: Ubuntu 24.04 and Linux Mint 22.3)

Common steps (game data, ports, playing a local game): [README.md](README.md).

# HOW TO INSTALL

## Point the game at your original assets data for BaboViolent 2 (servers, or to skip the dialog)

The client asks for the folder on first start and remembers it ([README](README.md#game-data-required)). Set `BV2_DATA_DIR` for the game server, or to use another folder:

Add the folder that contains `main/` to `~/.profile` (replace the path):

```bash
echo 'export BV2_DATA_DIR="$HOME/Games/bv2-data"' >> ~/.profile
```

Then log out from desktop and log in again. 

Check: 

```bash
ls "$BV2_DATA_DIR/main/maps"
```

this should show a list of `.bvm` map files (if you pointed it at `main/` itself, drop `/main`).

## Use a downloaded package of Robo Violence 2

Releases ([GitHub](https://github.com/Hub-Tansky/RoboViolence2/releases)) carry `roboviolence2-linux-x64.deb`, `roboviolence2-linux-x64.tar.gz` and the source tarball; Flatpak comes later (PNS-52). Both need glibc 2.39 or newer: Ubuntu 24.04, Linux Mint 22, Debian 13 or later.

### The .deb (Ubuntu, Mint, Debian)

```bash
sudo apt install ./roboviolence2-linux-x64.deb
```

apt installs the libraries it needs. Start **Robo Violence 2** from the applications menu, or run `bv2` (`bv2dedicated CTF` for a server). Settings, logs and saved maps go to `~/.local/share/BaboViolent2/bv2/`. To upgrade, run the same command with the newer file; `sudo apt remove roboviolence2` uninstalls. The `.deb` holds the client and `bv2dedicated`; for the master server use the `.tar.gz` ([ADR 0031](../decisions/0031-linux-deb-package.md)).

### The .tar.gz (any distribution)

1. Install the runtime libraries: the client and `bv2dedicated` link GL and GLU; `bv2master` needs nothing extra. On Ubuntu 24.04:

   ```bash
   sudo apt install libgl1 libglu1-mesa
   ```

   The client also needs the X11 or Wayland and audio libraries that every desktop install has. CI installs exactly these packages (from `tools/setup-dev.sh --linux-runtime-packages`) in a clean `ubuntu:24.04` and starts the server and master from the package (`package-check` job).

2. Unpack and run:

   ```bash
   tar -xzf roboviolence2-linux-x64.tar.gz
   ```

   ```bash
   cd roboviolence2-linux-x64 && ./run-server.sh
   ```

   Then `./run-client.sh connect 127.0.0.1` in another terminal. `./run-master.sh` is only needed for the server browser ([README](README.md#playing-a-local-game)).


# Advanced section 

## Build from source

Prerequisites: print the apt line with `tools/setup-dev.sh --linux-packages`, then run it. It installs the compiler, Ninja, autotools (for vcpkg ports) and the X11, Wayland, GL and audio headers SDL3 needs. Also install CMake 3.25+ and Python 3 (Ubuntu 24.04's packages are recent enough), and for commits `pipx install pre-commit` (the commit hooks run it).

vcpkg, once:

```bash
git clone https://github.com/microsoft/vcpkg "$HOME/vcpkg"
```

```bash
"$HOME/vcpkg/bootstrap-vcpkg.sh"
```

Add `export VCPKG_ROOT="$HOME/vcpkg"` to `~/.profile`.

Build, from the repository root:

```bash
tools/setup-dev.sh
```

```bash
cmake --preset linux-x64
```

```bash
cmake --build --preset linux-x64
```

Servers only: add `-DBV2_BUILD_CLIENT=OFF` to the configure command.

## Without vcpkg (cloud sessions)

When vcpkg can't download its ports (for example a proxy that blocks the gnu.org mirrors the autotools ports need), build against system packages and a hand-built SDL3. The SDL3 and miniaudio versions below are the `sdl3` and `miniaudio` ports at the `vcpkg.json` `builtin-baseline`; update them when the baseline moves. Checked on Ubuntu 24.04 at 34d73aa (2026-11-07): build plus `ctest -R screenshot_`, about 10 minutes.

1. Packages:

   ```bash
   sudo apt-get install build-essential cmake ninja-build python3 git libsodium-dev libsqlite3-dev libstb-dev libgl-dev libglu1-mesa-dev libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev libxss-dev libxfixes-dev libxtst-dev libgl1-mesa-dri xauth xvfb
   ```

2. SDL3 at the tag that matches the vcpkg baseline, into a prefix (here `/opt/rv2deps`):

   ```bash
   git clone --depth 1 -b release-3.4.16 https://github.com/libsdl-org/SDL.git
   cmake -S SDL -B SDL/build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/rv2deps -DSDL_TESTS=OFF -DSDL_WAYLAND=OFF -DSDL_PIPEWIRE=OFF -DSDL_PULSEAUDIO=OFF
   cmake --build SDL/build && cmake --install SDL/build
   ```

3. miniaudio 0.11.25, header and source only:

   ```bash
   git clone --depth 1 -b 0.11.25 https://github.com/mackron/miniaudio.git
   mkdir -p /opt/rv2deps/include/miniaudio && cp miniaudio/miniaudio.h miniaudio/miniaudio.c /opt/rv2deps/include/miniaudio/
   ```

4. A stand-in for vcpkg's `unofficial-sodium` package, in `/opt/rv2deps/share/unofficial-sodium/unofficial-sodiumConfig.cmake`:

   ```cmake
   find_library(SODIUM_LIB sodium REQUIRED)
   find_path(SODIUM_INC sodium.h REQUIRED)
   if(NOT TARGET unofficial-sodium::sodium)
     add_library(unofficial-sodium::sodium UNKNOWN IMPORTED)
     set_target_properties(unofficial-sodium::sodium PROPERTIES IMPORTED_LOCATION "${SODIUM_LIB}" INTERFACE_INCLUDE_DIRECTORIES "${SODIUM_INC}")
   endif()
   ```

5. Configure without a preset, build, test:

   ```bash
   cmake -S . -B build/local -G Ninja -DCMAKE_PREFIX_PATH=/opt/rv2deps
   cmake --build build/local
   ctest --test-dir build/local -R screenshot_ --output-on-failure
   ```

### Regenerate a screenshot golden

Goldens: `tests/golden/<name>.png` ([ADR 0026](../decisions/0026-screenshot-goldens.md)). With the build above:

```bash
python3 -B tests/screenshots.py "$PWD/build/local/runtime/bv2" "$PWD/build/local/shots" <name> --update
```

Then run `ctest --test-dir build/local -R screenshot_<name>` about 10 times: a golden that differs between runs fails CI at random. Placeholder data has no skins, so a frame that shows the Appearance ball needs `set cl_redDecal 0 0 0` (and green, blue) in its script, as `tests/golden/appearance.txt` does. Cvars in these scripts need the `set` prefix.

## Output and run

`build/linux-x64/runtime/` holds `bv2`, `bv2dedicated`, `bv2master` and `main/`.

```bash
cd build/linux-x64/runtime && ./bv2dedicated CTF
```

```bash
build/linux-x64/runtime/bv2
```

SDL3 picks Wayland or X11 automatically; force one with `SDL_VIDEODRIVER=x11` or `SDL_VIDEODRIVER=wayland`.

## Troubleshooting

| Symptom | Fix |
|---|---|
| `error while loading shared libraries: libGLU.so.1` | Install the runtime packages above |
| vcpkg port fails to build (autoconf, libtool) | Install the full `--linux-packages` line |
| No window under Wayland | Try `SDL_VIDEODRIVER=x11` |
| Placeholder graphics | "Use placeholders" was chosen, which is remembered: paste the BV2 data folder in Options → Game assets, press Enter and restart; a set `BV2_DATA_DIR` must contain `main/` |
