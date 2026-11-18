#!/usr/bin/env python3
"""dependabot-automerge.yml (ROBO-250): only Dependabot's own GitHub Actions minor and patch PRs get auto-merge; a
major, a Docker update or a PR from anyone else does not (ADR ROBO-250). Evaluates the workflow's two `if:`
expressions for each case, so a widened condition fails here."""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
text = open(os.path.join(ROOT, ".github", "workflows", "dependabot-automerge.yml"), encoding="utf-8").read()
conditions = re.findall(r"^\s+if: (.+)$", text, re.M)
assert len(conditions) == 2, conditions


def evaluate(expr, ctx):
    """The subset of the Actions expression syntax the workflow uses: ==, &&, ||, parentheses, '...' literals."""
    py = re.sub(r"'([^']*)'", lambda m: repr(m.group(1)), expr)
    py = py.replace("&&", " and ").replace("||", " or ")
    py = re.sub(r"\b(github|steps)(\.[\w.-]+)", lambda m: repr(ctx.get(m.group(0), "")), py)
    return eval(py, {"__builtins__": {}})


BOT = "dependabot[bot]"
base = {
    "github.event.pull_request.user.login": BOT,
    "github.actor": BOT,
    "github.repository": "Hub-Tansky/RoboViolence2",
    "steps.meta.outputs.package-ecosystem": "github_actions",
    "steps.meta.outputs.update-type": "version-update:semver-minor",
}
CASES = [
    ("actions minor", {}, True),
    ("actions patch", {"steps.meta.outputs.update-type": "version-update:semver-patch"}, True),
    ("actions major", {"steps.meta.outputs.update-type": "version-update:semver-major"}, False),
    ("docker minor", {"steps.meta.outputs.package-ecosystem": "docker"}, False),
    ("person's PR", {"github.event.pull_request.user.login": "Hub-Tansky", "github.actor": "Hub-Tansky"}, False),
    ("person pushes to Dependabot's branch", {"github.actor": "Hub-Tansky"}, False),
    ("fork of the repo", {"github.repository": "someone/RoboViolence2"}, False),
]
failures = 0
for name, change, expected in CASES:
    ctx = {**base, **change}
    got = all(evaluate(c, ctx) for c in conditions)
    if got != expected:
        print(f"FAIL {name}: expected {expected}, got {got}")
        failures += 1
print("FAILED" if failures else "dependabot automerge: ok")
sys.exit(1 if failures else 0)
