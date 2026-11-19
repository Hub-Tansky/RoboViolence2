#!/usr/bin/env python3
""".claude/hooks/guard.py (ROBO-256): each denied command with its rule named, commands that stay allowed, the guard-file
ask on Edit and Write, and the hook's JSON output (ADR ROBO-256)."""
import importlib.util
import json
import os
import subprocess
import sys

sys.dont_write_bytecode = True
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GUARD = os.path.join(ROOT, ".claude", "hooks", "guard.py")
spec = importlib.util.spec_from_file_location("guard", GUARD)
guard = importlib.util.module_from_spec(spec)
spec.loader.exec_module(guard)
failures = 0


def check(name, cond, detail=""):
    global failures
    if not cond:
        print(f"FAIL {name} {detail}")
        failures += 1


DENY = [
    ("git commit --no-verify -m x", "--no-verify"),
    ("git push --no-verify origin claude/x", "--no-verify"),
    ("git push --force origin main", "force push"),
    ("git push -f origin HEAD:main", "force push"),
    ("git push --force-with-lease origin HEAD:main", "force push"),
    ("git push origin +HEAD:main", "force push"),
    ("cd repo && GH_TOKEN=$(gh auth token) git -c x=y push -f origin main", "force push"),
    ("git tag v0.1.0", "release tags"),
    ("git tag -a v0.1.0 -m 'r'", "release tags"),
    ("git push origin v0.1.0", "push of release tags"),
    ("git push origin refs/tags/v0.1.0", "push of release tags"),
    ("git push --tags", "push of release tags"),
    ("gh auth switch --user patpi", "gh auth switch"),
    ("gh secret set NTFY_TOPIC", "gh secret"),
    ("echo x | gh secret list", "gh secret"),
    ("gh api -X PUT repos/Hub-Tansky/RoboViolence2/rulesets/24412532 --input r.json", "rulesets"),
    ("gh api repos/Hub-Tansky/RoboViolence2/rulesets -f name=x", "rulesets"),
    ("gh api --method DELETE repos/Hub-Tansky/RoboViolence2/rulesets/1", "rulesets"),
]
ALLOW = [
    "git push -u origin claude/wp-16-hooks",
    "git push -f origin claude/wp-16-hooks",
    "git push origin main",
    "git commit -m 'fix: x'",
    "git tag -l",
    "git log --tags --oneline",
    "gh api repos/Hub-Tansky/RoboViolence2/rulesets/24412532 --jq '.rules'",
    "gh auth token --user Hub-Tansky",
    "gh pr view 5",
    "ls main",
]
for command, rule in DENY:
    got = guard.decide({"tool_name": "Bash", "tool_input": {"command": command}})
    check(f"deny {command}", got and got[0] == "deny" and rule in got[1] and got[1].startswith("ROBO-256 guard: "), got)
for command in ALLOW:
    got = guard.decide({"tool_name": "Bash", "tool_input": {"command": command}})
    check(f"allow {command}", got is None, got)

for path, ask in [(".github/workflows/build.yml", True), (".githooks/commit-msg", True), ("REVIEW.md", True), ("tools/review.sh", True),
                  ("game/src/Game.cpp", False), ("tools/merge-risk.py", False), ("docs/build/README.md", False), ("../rv2-docs/REVIEW.md", False)]:
    for tool in ("Edit", "Write"):
        got = guard.decide({"tool_name": tool, "tool_input": {"file_path": os.path.join(ROOT, path)}})
        check(f"{tool} {path}", (got is not None and got[0] == "ask" and path in got[1]) if ask else got is None, got)
check("relative path", guard.decide({"tool_name": "Edit", "tool_input": {"file_path": "/nowhere/REVIEW.md"}}) is None)
check("other tool", guard.decide({"tool_name": "Read", "tool_input": {"file_path": os.path.join(ROOT, "REVIEW.md")}}) is None)

for event, want in [({"tool_name": "Bash", "tool_input": {"command": "gh secret list"}}, "deny"), ({"tool_name": "Bash", "tool_input": {"command": "ls"}}, None)]:
    r = subprocess.run([sys.executable, "-B", GUARD], input=json.dumps(event), capture_output=True, text=True)
    out = json.loads(r.stdout)["hookSpecificOutput"] if r.stdout.strip() else None
    check(f"cli {want}", r.returncode == 0 and (out["permissionDecision"] == want and out["hookEventName"] == "PreToolUse" if want else out is None), (r.stdout, r.stderr))

print("ok" if failures == 0 else f"{failures} failed")
sys.exit(1 if failures else 0)
