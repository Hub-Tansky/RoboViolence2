"""The client binary carries the release version the build was given (ADR 0046).

Usage: test_client_version.py <bv2 binary> <version text>
Python 3, stdlib only.
"""
import sys

binary, version = sys.argv[1], sys.argv[2]
# g_releaseVersion is a NUL-terminated array; NUL on both sides, as a suffix match hits "127.0.0.1"
if b"\0" + version.encode() + b"\0" in open(binary, "rb").read():
    print("ok: client_version")
    sys.exit(0)
print(f"FAIL: {binary} does not contain {version!r}")
sys.exit(1)
