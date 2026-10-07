# macOS (12+, Apple silicon)

Common steps (game data, ports, playing a local game): [README.md](README.md).

## Use a downloaded package

1. Unzip `roboviolence2-macos-arm64.zip`.
2. The build is unsigned, so Gatekeeper blocks it. Clear the quarantine flag once:

   ```bash
   xattr -dr com.apple.quarantine roboviolence2-macos-arm64
   ```

3. In that folder: `./run-master.sh`, `./run-server.sh`, `./run-client.sh connect 127.0.0.1`.

## Build from source

Prerequisites, with [Homebrew](https://brew.sh):

```bash
brew install cmake ninja autoconf autoconf-archive automake libtool pkg-config
```

vcpkg, once (any folder works; `VCPKG_ROOT` must point to it):

```bash
git clone https://github.com/microsoft/vcpkg "$HOME/vcpkg"
```

```bash
"$HOME/vcpkg/bootstrap-vcpkg.sh"
```

Add `export VCPKG_ROOT="$HOME/vcpkg"` to `~/.zshrc`. The preset fails without it.

Build, from the repository root:

```bash
tools/setup-dev.sh
```

```bash
cmake --preset macos-arm64
```

```bash
cmake --build --preset macos-arm64
```

`tools/setup-dev.sh` runs once per clone and activates the commit hooks.

## Output

`build/macos-arm64/runtime/`:

- `bv2.app`: the client. The build links `Contents/Resources/main` to `runtime/main` and signs the bundle ad hoc.
- `bv2dedicated`, `bv2master`: the servers. Run them from `runtime/`.

## Run

```bash
build/macos-arm64/runtime/bv2.app/Contents/MacOS/bv2
```

Starting the binary directly shows its log in the terminal. `open bv2.app` hides the log and drops environment variables, so use it only when nothing goes wrong.

```bash
cd build/macos-arm64/runtime && ./bv2dedicated CTF
```

## Point the game at your original data

The game looks for `main/` in `BV2_DATA_DIR` first. Example with the data in `~/Games/bv2-data/main/`:

| How you start the client | How to set it |
|---|---|
| From Terminal | `BV2_DATA_DIR="$HOME/Games/bv2-data" build/macos-arm64/runtime/bv2.app/Contents/MacOS/bv2`, or add `export BV2_DATA_DIR="$HOME/Games/bv2-data"` to `~/.zshrc` |
| Double-click or `open bv2.app` | GUI apps don't read `~/.zshrc`. `launchctl setenv BV2_DATA_DIR "$HOME/Games/bv2-data"` sets it until the next reboot |
| No variable | Replace `bv2.app/Contents/Resources/main` with a link to your `main/`. A rebuild or a new package undoes it |

## Troubleshooting

| Symptom | Fix |
|---|---|
| `Could not find toolchain file` / vcpkg errors at configure | `VCPKG_ROOT` is unset or wrong in this shell |
| Black window or immediate exit | Start the binary from Terminal and read the log |
| "Can not load language file" | No `main/` found: check `BV2_DATA_DIR`, or start from `runtime/` |
| "app is damaged" on a downloaded zip | Run the `xattr` command above |
