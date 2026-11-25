#!/usr/bin/env python3
"""Unpack an archive that isn't hash-pinned (RV2_ASSETS url and original, ADR ROBO-261) without letting it write outside
the destination.

Usage: safe-extract.py <archive> <dest dir> [<member prefix>]
Refuses the whole archive, writing nothing, when any member is absolute, has a `..` segment, a backslash or a drive
letter or a `;`, or is a link, device or FIFO. With a prefix, only members under it are unpacked. Reads .zip, .tar, .tar.gz.
Exit 0 on success, 1 with the reason on stderr. Python 3, stdlib only.
"""
import os
import shutil
import sys
import tarfile
import zipfile


def bad_name(name):
    """Why a member name could leave the destination, or None"""
    if not name or name.startswith(("/", "\\")) or "\\" in name or (len(name) > 1 and name[1] == ":"):
        return "absolute or Windows path"
    if ".." in name.split("/"):
        return "'..' segment"
    if ";" in name:
        return "';' (CMake would split the name)"
    return None


def members(archive):
    """[(name, kind)] with kind 'file', 'dir' or 'other'"""
    if zipfile.is_zipfile(archive):
        with zipfile.ZipFile(archive) as z:
            out = []
            for info in z.infolist():
                mode = info.external_attr >> 16
                link = (mode & 0o170000) == 0o120000
                out.append((info.filename, "other" if link else ("dir" if info.is_dir() else "file")))
            return out
    with tarfile.open(archive) as t:
        return [(m.name, "file" if m.isfile() else ("dir" if m.isdir() else "other")) for m in t.getmembers()]


def check(archive, prefix):
    for name, kind in members(archive):
        if prefix and not (name + "/").startswith(prefix.rstrip("/") + "/"):
            continue
        reason = bad_name(name) or ("link or special file" if kind == "other" else None)
        if reason:
            return f"{name}: {reason}"
    return None


def extract(archive, dest, prefix):
    os.makedirs(dest, exist_ok=True)
    wanted = [name for name, kind in members(archive)
              if kind in ("file", "dir") and (not prefix or (name + "/").startswith(prefix.rstrip("/") + "/"))]
    if zipfile.is_zipfile(archive):
        with zipfile.ZipFile(archive) as z:
            for name in wanted:
                z.extract(name, dest)
    else:
        with tarfile.open(archive) as t:
            for name in wanted:
                member = t.getmember(name)
                if hasattr(tarfile, "data_filter"):  # the same on every Python, and a second net under check()
                    t.extract(member, dest, set_attrs=False, filter="data")
                else:
                    t.extract(member, dest, set_attrs=False)


def main():
    if len(sys.argv) not in (3, 4):
        sys.exit(__doc__)
    archive, dest = sys.argv[1], sys.argv[2]
    prefix = sys.argv[3] if len(sys.argv) == 4 else ""
    try:
        problem = check(archive, prefix)
    except (OSError, tarfile.TarError, zipfile.BadZipFile, EOFError) as e:
        problem = f"unreadable archive: {e}"
    if problem:
        print(f"refused: {problem}", file=sys.stderr)
        return 1
    try:
        extract(archive, dest, prefix)
    except (OSError, tarfile.TarError, zipfile.BadZipFile, EOFError) as e:
        shutil.rmtree(dest, ignore_errors=True)
        print(f"unpack failed: {e}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
