# macOS (12+, Apple silicon)

Common steps (game data, ports, playing a local game): [README.md](README.md).

## Point the game at your original data (required, once)

Find your BaboViolent 2 data folder: the one that contains `main/` with `maps/`, `textures/` and `sounds/` inside. Add it to `~/.zshrc` (replace the path with yours), then open a new Terminal tab:

```bash
echo 'export BV2_DATA_DIR="$HOME/Games/bv2-data"' >> ~/.zshrc
```

Check it in the new tab; this must list `.bvm` map files (if you pointed it at `main/` itself, drop `/main`):

```bash
ls "$BV2_DATA_DIR/main/maps"
```

Terminal tabs and the run scripts now use your data. Starting the app by double-click or `open bv2.app` doesn't read `~/.zshrc`: run `launchctl setenv BV2_DATA_DIR "$BV2_DATA_DIR"` once per login for that.

## Use a downloaded package

1. Unzip `roboviolence2-macos-arm64.zip`.
2. The build is unsigned, so Gatekeeper blocks it. Clear the quarantine flag once:

   ```bash
   xattr -dr com.apple.quarantine roboviolence2-macos-arm64
   ```

3. In that folder, run each in its own Terminal tab: `./run-server.sh`, then `./run-client.sh connect 127.0.0.1`. Add `./run-master.sh` only for the server browser ([README](README.md#playing-a-local-game)).

The package's own `main/` copies (one in `bv2.app/Contents/Resources/` for the client, one in the folder for the servers) hold placeholders; `BV2_DATA_DIR` takes priority over both.

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

## Other ways to set the data folder

| Case | How |
|---|---|
| One command only | Prefix it: `BV2_DATA_DIR="$HOME/Games/bv2-data" ./run-client.sh` |
| No variable at all | Replace `bv2.app/Contents/Resources/main` with a link to your `main/`, then re-sign with `codesign --force --sign - bv2.app` (the change breaks the bundle signature). A rebuild or a new package undoes it |

## Troubleshooting

| Symptom | Fix |
|---|---|
| `Could not find toolchain file` / vcpkg errors at configure | `VCPKG_ROOT` is unset or wrong in this shell |
| Black window or immediate exit | Start the binary from Terminal and read the log |
| "Can not load language file", or placeholder graphics | `BV2_DATA_DIR` is unset in this tab or doesn't contain `main/`: open a new tab, check with `ls "$BV2_DATA_DIR/main/maps"` |
| "app is damaged" on a downloaded zip | Run the `xattr` command above |
