#!/usr/bin/env python3
"""dependabot-automerge.yml (ROBO-250): only Dependabot's own GitHub Actions minor and patch PRs get auto-merge; a
major, a Docker update or a push by anyone else turns it off (ADR ROBO-250). Evaluates the workflow's `if:`
expressions for each case and pins the trigger and the token's permissions, so a widened workflow fails here."""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
text = open(os.path.join(ROOT, ".github", "workflows", "dependabot-automerge.yml"), encoding="utf-8").read()
failures = 0


def check(name, cond, detail=""):
    global failures
    if not cond:
        print(f"FAIL {name} {detail}")
        failures += 1


# Trigger and token: plain pull_request (no pull_request_target), read-only at the top, write only in the one job
check("trigger", re.search(r"^on:\n  pull_request:\n    types: \[opened, synchronize, reopened\]\npermissions:\n  contents: read\njobs:\n", text, re.M))
check("one job", len(re.findall(r"^  \w[\w-]*:\n", text.split("\njobs:\n", 1)[1], re.M)) == 1)
check("job permissions", re.findall(r"^ {6}(\S+): (\S+)", text, re.M) == [("contents", "write"), ("pull-requests", "write")],
      re.findall(r"^ {6}(\S+): (\S+)", text, re.M))
check("pinned metadata action", re.search(r"uses: dependabot/fetch-metadata@[0-9a-f]{40}  # v", text))

job_if, enable_if, disable_if = (re.search(rf"^{indent}if: (.+)$", part, re.M).group(1) for indent, part in [
    (" {4}", text),
    (" {8}", text.split("name: Turn on auto-merge", 1)[1]),
    (" {8}", text.split("name: Turn off auto-merge", 1)[1]),
])


def evaluate(expr, ctx):
    """The subset of the Actions expression syntax the workflow uses: ==, &&, ||, !, (), always(), '...' literals."""
    py = re.sub(r"'([^']*)'", lambda m: repr(m.group(1)), expr)
    py = py.replace("&&", " and ").replace("||", " or ").replace("always()", "True").replace("!(", " not (")
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
# (case, change, job runs, auto-merge on)
CASES = [
    ("actions minor", {}, True, True),
    ("actions patch", {"steps.meta.outputs.update-type": "version-update:semver-patch"}, True, True),
    ("actions major", {"steps.meta.outputs.update-type": "version-update:semver-major"}, True, False),
    ("docker minor", {"steps.meta.outputs.package-ecosystem": "docker"}, True, False),
    ("person pushes to Dependabot's branch (no metadata)", {"github.actor": "Hub-Tansky",
     "steps.meta.outputs.package-ecosystem": "", "steps.meta.outputs.update-type": ""}, True, False),
    ("person pushes, metadata still read", {"github.actor": "Hub-Tansky"}, True, False),
    ("person's PR", {"github.event.pull_request.user.login": "Hub-Tansky", "github.actor": "Hub-Tansky"}, False, False),
    ("fork of the repo", {"github.repository": "someone/RoboViolence2"}, False, False),
]
for name, change, runs, on in CASES:
    ctx = {**base, **change}
    job = evaluate(job_if, ctx)
    enable, disable = job and evaluate(enable_if, ctx), job and evaluate(disable_if, ctx)
    check(name, job == runs and enable == on and disable == (runs and not on),
          f"job {job} enable {enable} disable {disable}")
print("FAILED" if failures else "dependabot automerge: ok")
sys.exit(1 if failures else 0)
