#!/usr/bin/env python3
"""tools/check-docs-only.py (ROBO-246): which runs are docs-only, the changed paths of a PR merge commit, and that every
step after the docs-only check in build.yml and containers.yml is skipped on a docs-only PR (ADR 0068)."""
import importlib.util
import os
import re
import shutil
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
spec = importlib.util.spec_from_file_location("docs_only", os.path.join(ROOT, "tools", "check-docs-only.py"))
tool = importlib.util.module_from_spec(spec)
spec.loader.exec_module(tool)

# (case, event, changed paths, docs only)
CASES = [
    ("README", "pull_request", ["README.md"], True),
    ("docs dir", "pull_request", ["docs/build/windows.md", "docs/decisions/0001-x.md"], True),
    ("one .cpp", "pull_request", ["README.md", "game/src/Game.cpp"], False),
    ("AGENTS.md steers agents", "pull_request", ["AGENTS.md"], False),
    ("REVIEW.md steers the gate", "pull_request", ["REVIEW.md"], False),
    ("workflow", "pull_request", [".github/workflows/README.md"], False),
    ("skills", "pull_request", [".claude/skills/x/SKILL.md"], False),
    ("no paths", "pull_request", [], False),
    ("linux.md: package-check reads it", "pull_request", ["docs/build/linux.md"], False),
    ("docs/agents steers agents", "pull_request", ["docs/agents/x.md"], False),
    ("push to main", "push", ["README.md"], False),
    ("schedule", "schedule", ["README.md"], False),
    ("by hand", "workflow_dispatch", ["README.md"], False),
]

failures = 0
for name, event, changed, expected in CASES:
    got = tool.docs_only(event, changed)
    if got != expected:
        print(f"FAIL {name}: expected {expected}, got {got}")
        failures += 1

# changed_paths on a merge commit like refs/pull/N/merge: the PR's paths, not the base branch's newer ones.
repo = tempfile.mkdtemp()
try:
    git = lambda *a: subprocess.run(["git", "-c", "user.email=t@t", "-c", "user.name=t", *a], cwd=repo, check=True, capture_output=True)
    git("init", "-q", "-b", "main")
    open(os.path.join(repo, "README.md"), "w").write("a\n")
    git("add", "-A"); git("commit", "-q", "-m", "base")
    git("checkout", "-q", "-b", "pr")
    open(os.path.join(repo, "README.md"), "w").write("b\n")
    git("commit", "-qam", "pr")
    git("checkout", "-q", "main")
    open(os.path.join(repo, "main.cpp"), "w").write("int a;\n")
    git("add", "-A"); git("commit", "-q", "-m", "main moved on")
    git("merge", "-q", "--no-ff", "-m", "merge", "pr")
    got = tool.changed_paths(repo)
    if got != ["README.md"]:
        print(f"FAIL changed_paths: expected ['README.md'], got {got}")
        failures += 1
finally:
    shutil.rmtree(repo, ignore_errors=True)

# Every step after the check carries the DOCS_ONLY condition, or runs only on failure or a tag (never on a docs-only PR).
GATED = {".github/workflows/build.yml": ["build", "smoke", "tidy"], ".github/workflows/containers.yml": ["images"]}
# An if: that is never true on a docs-only PR: the DOCS_ONLY test, failure() or a tag test, then only && terms. A ||
# outside parentheses would bind looser than && and could make it true.
FIRST = re.compile(r"^(env\.DOCS_ONLY != 'true'|failure\(\)|github\.ref_type == 'tag')( && |$)")


def safe(cond):
    top = cond
    while re.search(r"\([^()]*\)", top):
        top = re.sub(r"\([^()]*\)", "", top)  # drop parenthesised groups, innermost first
    return bool(FIRST.match(cond)) and "||" not in top


for path, jobs in GATED.items():
    text = open(os.path.join(ROOT, path), encoding="utf-8").read()
    for job in jobs:
        body = re.search(rf"^  {job}:\n(.*?)(?=^  [a-z-]+:\n|\Z)", text, re.M | re.S)
        steps = re.split(r"^      - ", body.group(1).split("    steps:\n", 1)[1], flags=re.M)[1:] if body else []
        gate = next((i for i, s in enumerate(steps) if "uses: ./.github/actions/docs-only" in s), None)
        if gate is None or "fetch-depth: 2" not in steps[0]:
            print(f"FAIL {path} {job}: needs checkout with fetch-depth 2, then ./.github/actions/docs-only")
            failures += 1
            continue
        for s in steps[gate + 1:]:
            cond = re.search(r"^(?:        )?if: (.*)$", s, re.M)
            c = cond.group(1).strip() if cond else ""
            if not safe(c):
                print(f"FAIL {path} {job}: step runs on a docs-only PR: {s.splitlines()[0]}")
                failures += 1
    for job, block in re.findall(r"^  ([a-z-]+):\n(.*?)(?=^  [a-z-]+:\n|\Z)", text, re.M | re.S):
        if re.search(r"^    needs: build$", block, re.M) and not re.search(r"^    if: needs\.build\.outputs\.docs != 'true'", block, re.M):
            print(f"FAIL {path} {job}: needs build but runs on a docs-only PR")
            failures += 1

print("docs only: ok" if failures == 0 else f"docs only: {failures} failed")
sys.exit(1 if failures else 0)
