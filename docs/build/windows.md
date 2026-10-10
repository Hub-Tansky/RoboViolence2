# Windows 10 22H2+ and Windows 11 (x64)

Common steps (game data, ports, playing a local game): [README.md](README.md).

Minimum: Windows 10 22H2 ([ADR 0012](../decisions/0012-windows-10-22h2-minimum.md)). Verified by a person on Windows 10 22H2 only (Phase B step 2); CI builds and tests on `windows-latest` (Windows Server 2025, the Windows 11 24H2 generation), so Windows 10 runtime behaviour is covered only by that person's runs. Report anything that differs.

The client needs a GPU driver with OpenGL 2.1. GPUs without a vendor Windows 10 driver (e.g. Intel G41/GMA X4500, which Windows runs on its basic WDDM 1.1 driver) fall back to OpenGL 1.1: the client starts but draws garbled text and ignores clicks. The servers don't need a GPU.

## Point the game at your original data (required, once)

In PowerShell, with the folder that contains your BaboViolent 2 `main\` (replace the path):

```powershell
setx BV2_DATA_DIR "C:\Games\bv2-data"
```

Open a new window and check; this must list `.bvm` map files:

```powershell
dir "$env:BV2_DATA_DIR\main\maps"
```

`setx` applies to new windows and to programs started from Explorer, including the `.cmd` scripts.

## Use a downloaded package

1. Extract `roboviolence2-win-x64-msvc.zip` (right-click → Extract All).
2. Double-click `run-server.cmd`, then `run-client.cmd` (`run-master.cmd` only for the server browser, see [README](README.md#playing-a-local-game)). The builds are unsigned: on the SmartScreen warning choose **More info → Run anyway**.
3. Allow the servers through Windows Defender Firewall when asked (private networks), so other machines can join.
4. To join from a terminal: `run-client.cmd connect 127.0.0.1`.

## Build from source

Prerequisites:

- Visual Studio 2022 Build Tools with **Desktop development with C++** (MSVC, Windows SDK).
- CMake 3.25+, Ninja and Python 3, for example `winget install Kitware.CMake Ninja-build.Ninja Python.Python.3.12`.
- Git.

vcpkg, once, in PowerShell:

```powershell
git clone https://github.com/microsoft/vcpkg $HOME\vcpkg
```

```powershell
& $HOME\vcpkg\bootstrap-vcpkg.bat
```

```powershell
setx VCPKG_ROOT "$HOME\vcpkg"
```

Open a new **Developer PowerShell for VS 2022** (it sets up MSVC), then from the repository root:

```powershell
powershell -ExecutionPolicy Bypass -File tools\setup-dev.ps1
```

```powershell
cmake --preset win-x64-msvc
```

```powershell
cmake --build --preset win-x64-msvc
```

## Output and run

`build\win-x64-msvc\runtime\` holds `bv2.exe`, `bv2dedicated.exe`, `bv2master.exe` and `main\`.

```powershell
cd build\win-x64-msvc\runtime; .\bv2dedicated.exe CTF
```

```powershell
build\win-x64-msvc\runtime\bv2.exe
```

## Troubleshooting

| Symptom | Fix |
|---|---|
| `cl.exe` not found, or the wrong compiler at configure | Use the Developer PowerShell for VS 2022 |
| vcpkg errors at configure | `VCPKG_ROOT` is unset in this window: open a new one after `setx` |
| "Windows protected your PC" | SmartScreen: **More info → Run anyway** |
| "running scripts is disabled on this system" | Windows blocks `.ps1` files by default: use the `-ExecutionPolicy Bypass -File` form above |
| Garbled text, flicker, window ignores clicks | No OpenGL 2.1 driver: install the GPU vendor's driver (see the requirement above) |
| "Can not load language file", or placeholder graphics | `BV2_DATA_DIR` is unset in this window or doesn't contain `main\`: open a new window, check with `dir "$env:BV2_DATA_DIR\main\maps"` |
