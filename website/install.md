# Install Robo Violence 2

**Alpha:** in-progress experimental version with placeholder art. For now we recommend using your own original [BaboViolent 2](https://github.com/Daivuk/BaboViolent2) data.

[Download](#downloads)

## Original game data

1. Download the original game: [BaboViolent2-master.zip](https://github.com/Daivuk/BaboViolent2/archive/refs/heads/master.zip).
2. Unzip it into your Downloads, Desktop or Documents folder. Robo Violence 2 finds it there on start.
3. Elsewhere, the game asks on first start: choose `BaboViolent2-master/BaboViolent2/Content/main/` or any folder above it.

## Requirements

- Windows 10 22H2 or later (x64) with an OpenGL 2.1 GPU driver.
- macOS 12 or later, Apple Silicon (M1 or newer).
- Linux x64 with glibc 2.39 or newer: Ubuntu 24.04, Linux Mint 22, Debian 13 or later.

## Downloads

- [All files on the GitHub releases page](https://github.com/Hub-Tansky/RoboViolence2/releases)

### Windows

Run `roboviolence2-win-x64-msvc-setup.exe`. It installs for your user only, without admin rights, and adds a Start-menu entry.

Project uses the [SignPath Foundation](https://signpath.org/) for code signing.

Privacy: the game sends nothing to other networked systems unless you ask it to (the server list, joining a server). The installer links the [privacy policy](https://roboviolence.com/privacy.md) and can turn the public server list off.

Until signed releases ship, the installer is unsigned. On "Windows protected your PC" (SmartScreen) choose **More info → Run anyway**.

### macOS

Open `roboviolence2-macos-arm64.dmg` and drag **Robo Violence 2** to Applications.

The app is unsigned, so Gatekeeper blocks the first start. Open **System Settings → Privacy & Security** and click **Open Anyway**, or run:

```
xattr -dr com.apple.quarantine "/Applications/Robo Violence 2.app"
```

To run a game server on a Mac, get `roboviolence2-macos-arm64.zip` from the same release ([Own server](https://roboviolence.com/server.md)).

### Linux

Ubuntu, Mint, Debian: install `roboviolence2-linux-x64.deb`, then start Robo Violence 2 from the applications menu.

```
sudo apt install ./roboviolence2-linux-x64.deb
```

Other distributions, or without installing: get the GL libraries (Ubuntu, Mint, Debian names):

```
sudo apt install libgl1 libglu1-mesa
```

Unpack and start:

```
tar -xzf roboviolence2-linux-x64.tar.gz
cd roboviolence2-linux-x64
./run-client.sh
```

More detail, troubleshooting and building from source: [build guides](https://github.com/Hub-Tansky/RoboViolence2/tree/main/docs/build).
