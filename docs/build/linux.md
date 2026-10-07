# Linux (x64; reference: Ubuntu 24.04)

Common steps (game data, ports, playing a local game): [README.md](README.md).

## Use a downloaded package

1. Install the runtime libraries (the client and both servers link GL and GLU). Print the exact line with `tools/setup-dev.sh --linux-runtime-packages`; on Ubuntu 24.04 it is:

   ```bash
   sudo apt install libgl1 libglu1-mesa
   ```

   The client also needs the X11 or Wayland and audio libraries that every desktop install has. CI checks the package against a clean `ubuntu:24.04` with only these two packages (`package-check` job).
2. Unpack and run:

   ```bash
   tar -xzf roboviolence2-linux-x64.tar.gz
   ```

   ```bash
   cd roboviolence2-linux-x64 && ./run-server.sh
   ```

   Also `./run-master.sh` and `./run-client.sh connect 127.0.0.1`.

## Build from source

Prerequisites: print the apt line with `tools/setup-dev.sh --linux-packages`, then run it. It installs the compiler, Ninja, autotools (for vcpkg ports) and the X11, Wayland, GL and audio headers SDL3 needs. Also install CMake 3.25+ and Python 3 (Ubuntu 24.04's packages are recent enough).

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

## Output and run

`build/linux-x64/runtime/` holds `bv2`, `bv2dedicated`, `bv2master` and `main/`.

```bash
cd build/linux-x64/runtime && ./bv2dedicated CTF
```

```bash
build/linux-x64/runtime/bv2
```

SDL3 picks Wayland or X11 automatically; force one with `SDL_VIDEODRIVER=x11` or `SDL_VIDEODRIVER=wayland`.

## Point the game at your original data

Add `export BV2_DATA_DIR="$HOME/Games/bv2-data"` (the folder containing `main/`) to `~/.profile`, or prefix one command with it.

## Troubleshooting

| Symptom | Fix |
|---|---|
| `error while loading shared libraries: libGLU.so.1` | Install the runtime packages above |
| vcpkg port fails to build (autoconf, libtool) | Install the full `--linux-packages` line |
| No window under Wayland | Try `SDL_VIDEODRIVER=x11` |
| "Can not load language file" | No `main/` found: check `BV2_DATA_DIR`, or start from the package or `runtime/` folder |
