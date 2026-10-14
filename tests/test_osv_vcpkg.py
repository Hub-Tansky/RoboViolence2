#!/usr/bin/env python3
"""Phase C step 4: tools/osv-vcpkg.py finds each port's upstream repository and tag (offline; no git, no OSV).

Usage: test_osv_vcpkg.py <scratch dir>
"""
import importlib.util
import json
import os
import sys
import urllib.error

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
spec = importlib.util.spec_from_file_location("osv_vcpkg", os.path.join(ROOT, "tools", "osv-vcpkg.py"))
osv_vcpkg = importlib.util.module_from_spec(spec)
spec.loader.exec_module(osv_vcpkg)

VCPKG = {"downloadLocation": "git+https://github.com/Microsoft/vcpkg@6bdd4c7007d6ef38cfd0de4a6a4c21556cbb3ac5"}
SHA = "f75e8d1cad7d90d72ef7a4661f1b994ef78b4e31"
# (case, SPDX packages, expected upstream)
CASES = [
    ("GitHub tag", [VCPKG, {"downloadLocation": "git+https://github.com/libsdl-org/SDL@release-3.4.16"}],
     ("https://github.com/libsdl-org/SDL", "release-3.4.16")),
    ("GitHub commit", [VCPKG, {"downloadLocation": "git+https://github.com/nothings/stb@" + SHA}],
     ("https://github.com/nothings/stb", SHA)),
    ("sqlite tarball", [VCPKG, {"downloadLocation": "https://sqlite.org/2026/sqlite-autoconf-3530400.tar.gz"}],
     ("https://github.com/sqlite/sqlite", "version-3.53.4")),
    ("vcpkg only", [VCPKG], None),
]

failures = 0
for name, packages, want in CASES:
    got = osv_vcpkg.upstream({"packages": packages})
    if got != want:
        print("FAIL %s: %r, want %r" % (name, got, want))
        failures += 1
if osv_vcpkg.commit("https://example.invalid/x", SHA) != SHA:
    print("FAIL a commit ref is used as is")
    failures += 1
# main(): helper ports skipped; an OSV failure means "not checked" (exit 2), not "vulnerable" (exit 1)
tree = os.path.join(sys.argv[1], "osv-vcpkg") if len(sys.argv) > 1 else None
if tree:
    for port, loc in [("stb", "git+https://github.com/nothings/stb@" + SHA), ("vcpkg-cmake", "")]:
        os.makedirs(os.path.join(tree, "x64-linux", "share", port), exist_ok=True)
        with open(os.path.join(tree, "x64-linux", "share", port, "vcpkg.spdx.json"), "w") as fh:
            json.dump({"packages": [{"downloadLocation": loc}]}, fh)
    def unreachable(sha):
        raise urllib.error.URLError("offline")
    osv_vcpkg.osv = unreachable
    sys.argv = ["osv-vcpkg.py", tree]
    try:
        osv_vcpkg.main()
        code = 0
    except SystemExit as e:
        code = e.code
    if code != 2:
        print("FAIL OSV unreachable: exit %r, want 2" % code)
        failures += 1

if not failures:
    print("ok: osv-vcpkg")
sys.exit(1 if failures else 0)
