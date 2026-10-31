# macOS (12+, Apple silicon)

Common steps (game data, ports, playing a local game): [README.md](README.md).

## Point the game at your original data (servers, or to skip the dialog)

The client asks for the folder on first start and remembers it ([README](README.md#game-data-required)). Set `BV2_DATA_DIR` for the game server, or to use another folder:

Add the folder that contains `main/` (with `maps/`, `textures/` and `sounds/`) to `~/.zshrc` (replace the path with yours), then open a new Terminal tab:

```bash
echo 'export BV2_DATA_DIR="$HOME/Games/bv2-data"' >> ~/.zshrc
```

Check it in the new tab; this must list `.bvm` map files (if you pointed it at `main/` itself, drop `/main`):

```bash
ls "$BV2_DATA_DIR/main/maps"
```

Terminal tabs and the run scripts now use your data. Starting the app by double-click or `open "Robo Violence 2.app"` (`open bv2.app` in a build) doesn't read `~/.zshrc`: run `launchctl setenv BV2_DATA_DIR "$BV2_DATA_DIR"` once per login for that.

## Install from a release

1. Download `roboviolence2-macos-arm64.dmg` from the [releases](https://github.com/Hub-Tansky/RoboViolence2/releases), open it and drag **Robo Violence 2** to Applications (the image holds only the app; [ADR 0030](../decisions/0030-app-icon-and-dmg-layout.md)).
2. It is unsigned, so the first start is blocked: open System Settings → Privacy & Security and click **Open Anyway** (or run `xattr -dr com.apple.quarantine "/Applications/Robo Violence 2.app"`).
3. The game asks for your BaboViolent 2 folder on first start. The servers are not in the disk image: download `roboviolence2-macos-arm64.zip` from the same release and follow the next section.

## Use a downloaded package

1. Unzip `roboviolence2-macos-arm64.zip` (from a release or a CI run).
2. The build is unsigned, so Gatekeeper blocks it. Clear the quarantine flag once:

   ```bash
   xattr -dr com.apple.quarantine roboviolence2-macos-arm64
   ```

3. In that folder, run each in its own Terminal tab: `./run-server.sh`, then `./run-client.sh connect 127.0.0.1`. Add `./run-master.sh` only for the server browser ([README](README.md#playing-a-local-game)).

The package's own `main/` copies (one in `Robo Violence 2.app/Contents/Resources/` for the client, one in the folder for the servers) hold placeholders, launch scripts and languages; your data replaces the placeholders.

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
| Placeholder graphics | "Use placeholders" was chosen, which is remembered: paste the BV2 data folder in Options → Game assets, press Enter and restart; a set `BV2_DATA_DIR` must contain `main/` |
| Start takes a minute with a black window | The data folder is in iCloud-synced `Documents` or `Desktop`, so every file is fetched on first read: move it elsewhere (e.g. `~/Games/bv2-data`), paste that folder in Options → Game assets, press Enter and restart |
| "app is damaged" on a downloaded zip | Run the `xattr` command above |
