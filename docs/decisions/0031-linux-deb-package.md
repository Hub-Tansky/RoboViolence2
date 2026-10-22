# 0031: A .deb with the client and bv2dedicated beside the Linux .tar.gz

- Status: Accepted
- Date: 2026-10-22
- Affects: `packaging/make_deb.py`, `packaging/make-package.py`, `.github/workflows/build.yml`, `website/index.html`, `docs/build/linux.md`; amends ADR 0020

## Context
- ADR 0020 rejected a `.deb` and left Linux installs to the `.tar.gz` and a later Flatpak (PNS-52).
- The owner reversed that for the first public release (2026-10-20, map "First public release delivery"): a `.deb` beside the `.tar.gz`, linked from the website under "Other platforms".
- The game already finds its content in `<exe>/../share/bv2` (`game/src/Paths.cpp`) and keeps all user data in the per-user pref directory, so it runs from a read-only system install. `bv2master` doesn't: it opens `master.db` and `web.db` in its working directory.

## Decision
- `make-package.py` builds `roboviolence2-linux-x64.deb` next to the `.tar.gz` from the same stage, through `packaging/make_deb.py` and `dpkg-deb` (no CPack, no nfpm: dpkg-dev is on every Ubuntu runner and build machine).
- Contents: `bv2` and `bv2dedicated` (stripped) in `/usr/games` (Debian policy for games), the package's `main/` in `/usr/share/bv2`, the `.desktop` file with `Exec=/usr/games/bv2` (a launcher's PATH may lack `/usr/games`), the hicolor icons (ADR 0030), and the licences in `/usr/share/doc/roboviolence2` (`copyright` names GPL-3+ and points at `licenses/`). dpkg's triggers refresh the icon and menu caches; no maintainer scripts.
- `bv2master` and its databases stay in the `.tar.gz` and the server image: a master is an operator's job, not a player's.
- Depends comes from `dpkg-shlibdeps` on the two binaries, plus `libgl1`, which SDL loads at run time. Version: the tag without `v`, `-` turned into `~`, so a `-rc` sorts before its release.
- Compatibility follows the build host (`ubuntu-24.04`, glibc 2.39), as for the `.tar.gz`: Ubuntu 24.04+, Linux Mint 22, Debian 13. CI's `deb-check` installs the `.deb` with apt in clean `ubuntu:24.04` and `debian:13`, starts the server as a non-root user and removes the package.

## Consequences
- The `.deb` isn't signed and there is no apt repository: players download it and run `sudo apt install ./roboviolence2-linux-x64.deb`; upgrades are the same command with the newer file.
- Building on Debian 12 or Ubuntu 22.04 would widen both Linux packages (older glibc); that changes the build host for every Linux job and is a separate item.
- lintian still reports the vendored tinyxml and missing man pages; neither stops an install.
- Rejected: `/usr/bin` (Debian puts games in `/usr/games`; the game finds `share/bv2` from either); shipping `bv2master` with a writable `/var/lib` folder (needs a system user and a service; the server image covers operators); CPack's DEB generator (it would package `cmake --install`, which lacks the generated placeholder content and the licences).
