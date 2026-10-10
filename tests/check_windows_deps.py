#!/usr/bin/env python3
"""Windows: the executables must not need a Visual C++ runtime DLL (players don't have the debug ones).

Usage: check_windows_deps.py <exe>...  (dumpbin comes from the MSVC environment)
"""
import re
import subprocess
import sys

bad = []
for exe in sys.argv[1:]:
    out = subprocess.run(["dumpbin", "/dependents", exe], capture_output=True, text=True, check=True).stdout
    bad += [f"{exe}: {dll}" for dll in re.findall(r"(?im)^\s+((?:msvcp|vcruntime|ucrtbase|concrt)\S*\.dll)\s*$", out)]
print("\n".join(bad) or "ok: no Visual C++ runtime DLLs")
sys.exit(1 if bad else 0)
