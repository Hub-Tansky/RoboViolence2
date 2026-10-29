"""Builds the Linux .deb from a Release build's `cmake --install` (ADR 0031). Needs dpkg-deb and dpkg-shlibdeps (dpkg-dev).

The install rules (CMakeLists.txt) define the layout: binaries, share/bv2/main (the game looks in <exe>/../share/bv2,
game/src/Paths.cpp), the .desktop file and the hicolor icons. The .deb moves the client and bv2dedicated to /usr/games
and adds the licences, copyright and changelog in /usr/share/doc/roboviolence2. bv2master (install component master)
stays in the .tar.gz: it keeps master.db and web.db in its working directory.
"""
import email.utils
import gzip
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

PACKAGING = Path(__file__).resolve().parent
PACKAGE = "roboviolence2"
MAINTAINER = "Robo Violence 2 contributors <337104978+Hub-Tansky@users.noreply.github.com>"
REPO = "https://github.com/Hub-Tansky/RoboViolence2"
BINARIES = ("bv2", "bv2dedicated")

CONTROL = """\
Package: {package}
Version: {version}
Architecture: amd64
Maintainer: {maintainer}
Installed-Size: {size_kib}
Depends: {depends}
Section: games
Priority: optional
Homepage: https://roboviolence.com
Description: free top-down multiplayer arena shooter
 Robo Violence 2 is an unofficial revival of BaboViolent 2. This package holds
 the game client (bv2) and the dedicated server (bv2dedicated). Without the
 original BaboViolent 2 data it plays with placeholder art.
 .
 Online play sends your IP address, nickname and chat to the servers you join;
 privacy policy: https://roboviolence.com/privacy.html
"""

COPYRIGHT = f"""\
Format: https://www.debian.org/doc/packaging-manuals/copyright-format/1.0/
Upstream-Name: Robo Violence 2
Source: {REPO}

Files: *
Copyright: 2012 bitHeads inc.
           Robo Violence 2 contributors
License: GPL-3+
 On Debian systems the full text of the GNU General Public License version 3
 is in /usr/share/common-licenses/GPL-3. The libraries built into the binaries
 come with their own licences in /usr/share/doc/{PACKAGE}/licenses/.
"""

CHANGELOG = """\
{package} ({version}) unstable; urgency=medium

  * {tag}: {repo}/releases

 -- {maintainer}  {date}
"""


def deb_version(version):
    """v0.0.2 -> 0.0.2; a '-' suffix becomes '~' so v0.1.0-rc1 sorts before v0.1.0 (0.0.0-dev -> 0.0.0~dev)"""
    v = version.removeprefix("v").replace("-", "~")
    if not v[:1].isdigit():
        sys.exit(f"version {version!r} must start with a digit after the v")
    return v


def depends(binaries):
    """The packages of the shared libraries the binaries need, with minimum versions (dpkg-shlibdeps)"""
    with tempfile.TemporaryDirectory() as tmp:
        (Path(tmp) / "debian").mkdir()  # dpkg-shlibdeps insists on a source package around the binaries
        (Path(tmp) / "debian" / "control").write_text(f"Source: {PACKAGE}\n\nPackage: {PACKAGE}\nArchitecture: any\n")
        r = subprocess.run(["dpkg-shlibdeps", "-O", *(f"-e{b.resolve()}" for b in binaries)], cwd=tmp,
                           capture_output=True, text=True)
    if r.returncode != 0:
        sys.exit(f"dpkg-shlibdeps failed:\n{r.stderr}")
    # SDL loads libGL.so.1 at run time (glad, no link), so dpkg-shlibdeps can't see it; the client runs curl and unzip
    # for the first-run data download (ADR 0040)
    return r.stdout.strip().removeprefix("shlibs:Depends=") + ", libgl1, curl, unzip"


def point_launcher_at_games(desktop):
    """Exec=bv2 becomes Exec=/usr/games/bv2: a launcher can't count on /usr/games being in its PATH"""
    text = desktop.read_text()
    if "\nExec=bv2\n" not in text:
        sys.exit(f"{desktop.name}: expected the line Exec=bv2")
    desktop.write_text(text.replace("\nExec=bv2\n", "\nExec=/usr/games/bv2\n"))


def build_deb(build, licences, deb, version):
    """deb from `cmake --install <build>`, licences (a folder of <library>.txt) in the doc folder"""
    root = deb.parent / (deb.stem + "-root")
    shutil.rmtree(root, ignore_errors=True)
    for component in ("Unspecified", "icons"):  # everything but the master
        subprocess.run(["cmake", "--install", build, "--prefix", "/usr", "--component", component, "--strip"],
                       env={**os.environ, "DESTDIR": str(root)}, check=True, stdout=subprocess.DEVNULL)
    usr = root / "usr"
    (usr / "bin").rename(usr / "games")  # Debian policy: games in /usr/games; the game finds ../share/bv2 from either
    binaries = [usr / "games" / b for b in BINARIES]
    point_launcher_at_games(usr / "share" / "applications" / f"{PACKAGE}.desktop")
    doc = usr / "share" / "doc" / PACKAGE
    shutil.copytree(licences, doc / "licenses")
    (doc / "copyright").write_text(COPYRIGHT)
    v = deb_version(version)
    changelog = CHANGELOG.format(package=PACKAGE, version=v, repo=REPO, tag=version, maintainer=MAINTAINER,
                                 date=email.utils.formatdate())
    (doc / "changelog.gz").write_bytes(gzip.compress(changelog.encode(), compresslevel=9, mtime=0))
    # dpkg records modes as they are on disk: the umask of whoever built main/ must not leak into the package
    for p in root.rglob("*"):
        p.chmod(0o755 if p.is_dir() or p in binaries else 0o644)
    root.chmod(0o755)
    size_kib = sum(p.stat().st_size for p in root.rglob("*") if p.is_file()) // 1024 + 1
    (root / "DEBIAN").mkdir()
    (root / "DEBIAN" / "control").write_text(CONTROL.format(
        package=PACKAGE, version=v, maintainer=MAINTAINER, size_kib=size_kib, depends=depends(binaries)))
    subprocess.run(["dpkg-deb", "--root-owner-group", "-Zxz", "--build", root, deb], check=True)
    shutil.rmtree(root)
    return deb
