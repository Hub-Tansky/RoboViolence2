#!/usr/bin/env python3
"""Checks the installed vcpkg ports against OSV (PNS-22; CI `osv-vcpkg` workflow, weekly).

Dependabot and OSV can't match `pkg:vcpkg` purls, but each port's vcpkg.spdx.json names its upstream repository and
tag. The tag is resolved to a commit with `git ls-remote` and OSV is queried by commit (its GIT ranges).
Usage: tools/osv-vcpkg.py <vcpkg_installed dir>   (e.g. build/linux-x64/vcpkg_installed)
Exit 0: no known vulnerability; 1: vulnerabilities found; 2: a port could not be checked.
Python 3, stdlib only; needs git and network access.
"""
import glob
import json
import os
import re
import subprocess
import sys
import urllib.request

def upstream(spdx):
    """(repo URL, ref) of the port's source, or None"""
    for pkg in spdx["packages"]:
        loc = pkg.get("downloadLocation", "")
        m = re.match(r"git\+(https://github\.com/[^@]+)@(.+)$", loc)
        if m and "Microsoft/vcpkg" not in m.group(1):
            return m.group(1), m.group(2)
        m = re.search(r"sqlite\.org/\d+/sqlite-autoconf-(\d)(\d\d)(\d\d)\d\d\.tar", loc)
        if m:  # sqlite.org tarball: the GitHub mirror tags each release
            return "https://github.com/sqlite/sqlite", "version-%d.%d.%d" % tuple(int(g) for g in m.groups())
    return None


def commit(repo, ref):
    if re.fullmatch(r"[0-9a-f]{40}", ref):
        return ref
    out = subprocess.run(["git", "ls-remote", repo, "refs/tags/" + ref, "refs/tags/" + ref + "^{}"],
                         capture_output=True, text=True).stdout.split()
    # An annotated tag lists the tag object, then ^{} with the commit: take the last hash
    hashes = [w for w in out if re.fullmatch(r"[0-9a-f]{40}", w)]
    return hashes[-1] if hashes else None


def osv(sha):
    req = urllib.request.Request("https://api.osv.dev/v1/query", data=json.dumps({"commit": sha}).encode(),
                                 headers={"Content-Type": "application/json"})
    with urllib.request.urlopen(req, timeout=60) as r:
        return [v["id"] for v in json.load(r).get("vulns", [])]


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    found, unchecked = False, False
    ports = {}
    for f in glob.glob(os.path.join(sys.argv[1], "*", "share", "*", "vcpkg.spdx.json")):
        ports.setdefault(os.path.basename(os.path.dirname(f)), f)  # one triplet is enough
    for port in sorted(ports):
        if port.startswith("vcpkg-"):  # vcpkg's own helper ports carry no upstream code
            continue
        with open(ports[port], encoding="utf-8") as fh:
            src = upstream(json.load(fh))
        sha = commit(*src) if src else None
        try:
            vulns = osv(sha) if sha else None
        except OSError as e:  # URLError and HTTPError are OSErrors
            print("%-12s NOT CHECKED (OSV: %s)" % (port, e))
            unchecked = True
            continue
        if vulns is None:
            print("%-12s NOT CHECKED (no upstream commit: %s)" % (port, src))
            unchecked = True
            continue
        print("%-12s %s@%s %s %s" % (port, src[0], src[1], sha[:12], ", ".join(vulns) if vulns else "ok"))
        found = found or bool(vulns)
    sys.exit(1 if found else 2 if unchecked else 0)


if __name__ == "__main__":
    main()
