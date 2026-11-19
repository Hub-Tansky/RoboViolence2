#!/usr/bin/env python3
""".claude/hooks/guard.py (ROBO-256): each denied command with its rule named, commands that stay allowed, the guard-file
ask on Edit and Write, the hook's JSON output, and .githooks/commit-msg (ADR ROBO-256)."""
import importlib.util
import json
import os
import subprocess
import sys
import tempfile

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
    ("git commit --no-verify -m x", "--no-verify"),  # review: allow test of the rule that refuses it
    ("git push --no-verify origin claude/x", "--no-verify"),  # review: allow test of the rule that refuses it
    ("git commit -n -m x", "commit -n"),
    ("git commit -an -m x", "commit -n"),
    ("git push --force origin main", "force push"),
    ("git push -f origin HEAD:refs/heads/main", "force push"),
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
    "git tag --list v1*",
    "git tag --contains v0.1.0",
    "git push origin v2-branch",
    "git push -n origin claude/x",
    "git commit -m 'fix: n'",
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
                  (".claude/hooks/guard.py", True), (".claude/settings.json", True), (".claude/skills/rv2-code-review/SKILL.md", False), ("game/src/Game.cpp", False), ("tools/merge-risk.py", False), ("docs/build/README.md", False), ("../rv2-docs/REVIEW.md", False)]:
    for tool in ("Edit", "Write"):
        got = guard.decide({"tool_name": tool, "tool_input": {"file_path": os.path.join(ROOT, path)}})
        check(f"{tool} {path}", (got is not None and got[0] == "ask" and path in got[1]) if ask else got is None, got)
check("relative path", guard.decide({"tool_name": "Edit", "tool_input": {"file_path": "/nowhere/REVIEW.md"}}) is None)
check("other tool", guard.decide({"tool_name": "Read", "tool_input": {"file_path": os.path.join(ROOT, "REVIEW.md")}}) is None)

for event, want in [({"tool_name": "Bash", "tool_input": {"command": "gh secret list"}}, "deny"), ({"tool_name": "Bash", "tool_input": {"command": "ls"}}, None)]:
    r = subprocess.run([sys.executable, "-B", GUARD], input=json.dumps(event), capture_output=True, text=True)
    out = json.loads(r.stdout)["hookSpecificOutput"] if r.stdout.strip() else None
    check(f"cli {want}", r.returncode == 0 and (out["permissionDecision"] == want and out["hookEventName"] == "PreToolUse" if want else out is None), (r.stdout, r.stderr))

# .githooks/commit-msg: Claude trailers refused, other trailers kept
hook = os.path.join(ROOT, ".githooks", "commit-msg")
with tempfile.TemporaryDirectory() as tmp:
    msg = os.path.join(tmp, "msg")
    for text, ok in [("fix: x\n\nCo-Authored-By: Claude Opus <noreply@anthropic.com>\n", False), ("fix: x\n\nclaude-session: https://x\n", False),
                     ("fix: x\n\nCo-Authored-By: Alice <a@b>\n", True), ("fix: talk about Claude-Session: lines\n", True)]:
        open(msg, "w").write(text)
        r = subprocess.run(["sh", hook, msg], capture_output=True, text=True)
        check(f"commit-msg {text!r}", (r.returncode == 0) == ok, (r.returncode, r.stderr))

print("ok" if failures == 0 else f"{failures} failed")
sys.exit(1 if failures else 0)
