#!/usr/bin/env python3
"""tools/review.sh diff-rules: scope (Allowed paths globs) and anti-gaming (banned added lines).

Each case copies review.sh into a fresh git repo, commits the case's files on top of a base commit and runs
review.sh against that base with the case's PR description. The other checks fail there (their scripts are
missing), so only the diff-rules block is read.
"""
import os
import shutil
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if os.name == "nt":  # review.sh needs bash; Linux and macOS CI run this
    print("review diff rules: skipped on Windows")
    sys.exit(0)

# Built from parts so this file's own added lines don't trip the rule it tests.
LINT_OFF = "NO" + "LINT"
VERIFY_OFF = "--no-" + "verify"
ERRORS_OFF = "continue-on-" + "error: true"
ROW = "| Allowed paths | {} |\n"


def git(cwd, *args):
    return subprocess.run(["git", "-c", "user.name=t", "-c", "user.email=t@example.invalid", "-c", "commit.gpgsign=false",
                           *args], cwd=cwd, check=True, capture_output=True, text=True).stdout


def diff_rules(files, body):
    """(PASS or FAIL, the block's indented lines) of review.sh's diff-rules check on a commit adding `files`."""
    repo, scratch = tempfile.mkdtemp(), tempfile.mkdtemp()
    try:
        os.makedirs(os.path.join(repo, "tools"))
        shutil.copy(os.path.join(ROOT, "tools", "review.sh"), os.path.join(repo, "tools", "review.sh"))
        git(repo, "init", "-q")
        git(repo, "add", "-A")
        git(repo, "commit", "-q", "-m", "base")
        base = git(repo, "rev-parse", "HEAD").strip()
        for path, text in files.items():
            full = os.path.join(repo, *path.split("/"))
            os.makedirs(os.path.dirname(full), exist_ok=True)
            with open(full, "w", encoding="utf-8", newline="\n") as f:
                f.write(text)
        git(repo, "add", "-A")
        git(repo, "commit", "-q", "-m", "change")
        scope = os.path.join(scratch, "body.md")
        with open(scope, "w", encoding="utf-8", newline="\n") as f:
            f.write(body)
        out = subprocess.run(["bash", "tools/review.sh", "--base", base, "--scope", scope],
                             cwd=repo, capture_output=True, text=True).stdout.splitlines()
    finally:
        shutil.rmtree(repo, ignore_errors=True)
        shutil.rmtree(scratch, ignore_errors=True)
    start = next((i for i, l in enumerate(out) if l in ("PASS diff-rules", "FAIL diff-rules")), None)
    if start is None:
        return None, out
    block = []
    for line in out[start + 1:]:
        if not line.startswith("     "):
            break
        block.append(line.strip())
    return out[start].split()[0], block


SERVER, TESTS, DOCS = "`game/src/Server*.cpp`", "`tests/**`", "`docs/{a,b}.md`"
# (case, files the change adds, PR description, expected status, text each block line must hold in order)
CASES = [
    ("path inside Allowed paths", {"game/src/Server.cpp": "x\n"}, ROW.format(SERVER), "PASS", []),
    ("path outside Allowed paths", {"game/src/Server.cpp": "x\n", "game/src/Client.cpp": "x\n"}, ROW.format(SERVER), "FAIL",
     ["scope: PR Allowed paths; 2 changed files", "scope: game/src/Client.cpp is outside"]),
    ("* stays inside one folder", {"game/src/net/Server.cpp": "x\n"}, ROW.format(SERVER), "FAIL",
     ["scope: PR Allowed paths", "scope: game/src/net/Server.cpp is outside"]),
    ("** spans folders", {"tests/a.py": "x\n", "tests/e2e/deep/b.py": "x\n"}, ROW.format(TESTS), "PASS", []),
    ("**/ matches no folder too", {"docs/x.md": "x\n", "docs/y/z.md": "x\n"}, ROW.format("`docs/**/*.md`"), "PASS", []),
    ("brace alternatives inside", {"docs/a.md": "x\n", "docs/b.md": "x\n"}, ROW.format(DOCS), "PASS", []),
    ("brace alternatives outside", {"docs/a.md": "x\n", "docs/c.md": "x\n"}, ROW.format(DOCS), "FAIL",
     ["scope: PR Allowed paths", "scope: docs/c.md is outside"]),
    ("several globs in one row", {"game/src/Server.cpp": "x\n", "tests/a.py": "x\n", "docs/b.md": "x\n"},
     ROW.format(f"{SERVER} {TESTS} {DOCS}"), "PASS", []),
    ("guard file without Allowed paths", {".github/workflows/x.yml": "x\n"}, "", "PASS",
     ["NOTE .github/workflows/x.yml: guard file changed"]),
    ("banned added line", {"game/src/a.cpp": f"int x; // {LINT_OFF}\n"}, "", "FAIL",
     ["scope: none", f"anti-gaming: game/src/a.cpp: int x; // {LINT_OFF}"]),
    ("banned line with review: allow", {"game/src/a.cpp": f"int x; // {LINT_OFF} review: allow vendored macro\n"}, "", "PASS", []),
    ("banned words in Markdown", {"docs/a.md": f"Never use {VERIFY_OFF} or {LINT_OFF}.\n"}, "", "PASS", []),
    ("each banned line is reported", {"build.sh": f"git commit {VERIFY_OFF}\n", "ci.yml": f"{ERRORS_OFF}\n"}, "", "FAIL",
     ["scope: none", f"anti-gaming: build.sh: git commit {VERIFY_OFF}", f"anti-gaming: ci.yml: {ERRORS_OFF}"]),
]

failed = 0
for name, files, body, status, expected in CASES:
    got, block = diff_rules(files, body)
    good = got == status and len(block) == len(expected) and all(e in b for e, b in zip(expected, block))
    if not good:
        failed += 1
        print(f"FAIL {name}: {got} {block}")

print("review diff rules: FAILED" if failed else "review diff rules: ok")
sys.exit(1 if failed else 0)
