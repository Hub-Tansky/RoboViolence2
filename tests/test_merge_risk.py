#!/usr/bin/env python3
"""tools/merge-risk.py (ROBO-228): one fixture per tier rule, the GUARDS copy, and HIGH on errors."""
import importlib.util
import os
import re
import shutil
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
spec = importlib.util.spec_from_file_location("merge_risk", os.path.join(ROOT, "tools", "merge-risk.py"))
risk = importlib.util.module_from_spec(spec)
spec.loader.exec_module(risk)

GUI_BODY = "| Item | ROBO-100 [GUI] |\n| Allowed paths | `a` |\n"
# (case, changed, added lines, diff text, body, expected tier)
CASES = [
    ("netPacket.h", ["game/src/netPacket.h"], 3, "", "", "HIGH"),
    ("babonet", ["engine/babonet/src/a.cpp"], 3, "", "", "HIGH"),
    ("credentials", ["engine/credentials/a.cpp"], 3, "", "", "HIGH"),
    ("packaging", ["packaging/linux/a.sh"], 3, "", "", "HIGH"),
    ("guard file", ["tools/review.sh"], 3, "", "", "HIGH"),
    ("guard workflow", [".github/workflows/build.yml"], 3, "", "", "HIGH"),
    ("REVIEW.md", ["REVIEW.md"], 3, "", "", "HIGH"),
    ("GAME_VERSION_SV", ["game/src/Server.h"], 1, "+#define GAME_VERSION_SV 20\n", "", "HIGH"),
    ("GAME_VERSION_CL removed line", ["game/src/Client.h"], 1, "-#define GAME_VERSION_CL 19\n", "", "HIGH"),
    ("GAME_VERSION in a +++ header only", ["game/src/a.cpp"], 1, "+++ b/GAME_VERSION_SV\n", "", "MEDIUM"),
    ("[GUI] item", ["game/src/Menu.cpp"], 3, "", GUI_BODY, "HIGH"),
    ("[GUI] item, CRLF body", ["game/src/Menu.cpp"], 3, "", GUI_BODY.replace("\n", "\r\n"), "HIGH"),
    ("added line starting with -", ["game/src/Server.h"], 1, "+--GAME_VERSION_SV;\n", "", "HIGH"),
    ("601 added lines", ["game/src/a.cpp"], 601, "", "", "HIGH"),
    ("600 added lines", ["game/src/a.cpp"], 600, "", "", "MEDIUM"),
    ("26 files", [f"tests/t{i}.py" for i in range(26)], 26, "", "", "HIGH"),
    ("25 files", [f"tests/t{i}.py" for i in range(25)], 25, "", "", "LOW"),
    ("game code", ["game/src/Game.cpp"], 3, "", "", "MEDIUM"),
    ("engine code", ["engine/dko/src/a.cpp"], 3, "", "", "MEDIUM"),
    ("master server", ["masterserver/src/a.cpp"], 3, "", "", "MEDIUM"),
    ("launch script", ["content/LaunchScript/ctf.cfg"], 3, "", "", "MEDIUM"),
    ("docs tier", ["docs/build/README.md"], 3, "", "", "LOW"),
    ("tests only", ["tests/test_a.py"], 3, "", "", "LOW"),
    ("tooling, not guard", ["tools/ci-test-summary.py", "ARCHITECTURE.md"], 3, "", "", "LOW"),
    ("root build file", ["CMakeLists.txt"], 3, "", "", "MEDIUM"),
]

failures = 0
for name, changed, added, diff, body, expected in CASES:
    tier, reason = risk.classify(changed, added, diff, body)
    if tier != expected:
        print(f"FAIL {name}: expected {expected}, got {tier} ({reason})")
        failures += 1

review_sh = open(os.path.join(ROOT, "tools", "review.sh"), encoding="utf-8").read()
copy = re.search(r'^GUARDS = re\.compile\(r"(.*)"\)$', review_sh, re.M)
if not copy or copy.group(1) != risk.GUARDS.pattern:
    print("FAIL GUARDS in tools/merge-risk.py differs from tools/review.sh")
    failures += 1

# tier_line on a real repository, and HIGH when git fails.
repo = tempfile.mkdtemp()
try:
    git = lambda *a: subprocess.run(["git", "-c", "user.email=t@t", "-c", "user.name=t", *a], cwd=repo, check=True, capture_output=True)
    git("init", "-q", "-b", "main")
    os.makedirs(os.path.join(repo, "game", "src"))
    open(os.path.join(repo, "README.md"), "w").write("a\n")
    git("add", "-A"); git("commit", "-q", "-m", "base")
    git("checkout", "-q", "-b", "pr")
    open(os.path.join(repo, "game", "src", "Game.cpp"), "w").write("int a;\n")
    with open(os.path.join(repo, "game", "src", "Old.cpp"), "wb") as f:
        f.write(b"// caf\xe9 (Latin-1, not UTF-8)\n")
    git("add", "-A"); git("commit", "-q", "-m", "change")
    for name, got, expected in [
        ("tier_line", risk.tier_line("main", "pr", "", cwd=repo), "Tier: MEDIUM (game/src/Game.cpp and 1 more game files)"),
        ("bad base", risk.tier_line("no-such-ref", "pr", "", cwd=repo), "Tier: HIGH (merge-risk error: CalledProcessError)"),
    ]:
        if got != expected:
            print(f"FAIL {name}: expected '{expected}', got '{got}'")
            failures += 1
finally:
    shutil.rmtree(repo, ignore_errors=True)

print("merge risk: ok" if failures == 0 else f"merge risk: {failures} failed")
sys.exit(1 if failures else 0)
