#!/usr/bin/env python3
"""One GitHub issue per flaky test from the nightly flake watch (ROBO-237, ADR 0064).

Usage: flake-issues.py <ctest build dir>
Env: GITHUB_REPOSITORY (owner/name), GH_TOKEN for `gh api`; GITHUB_SERVER_URL, GITHUB_RUN_ID and GITHUB_SHA name the run.

Reads the failed tests and their output with tools/ci-test-summary.py. For each failed test it comments on the open
issue titled "Flaky test: <name>" (label flaky-test), or opens one. Exits 1 when a `gh api` call fails.
"""
import importlib.util
import json
import os
import subprocess
import sys

LABEL = "flaky-test"
TAIL = 60
FENCE = "````"

_spec = importlib.util.spec_from_file_location(
    "ci_test_summary", os.path.join(os.path.dirname(os.path.abspath(__file__)), "ci-test-summary.py"))
summary = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(summary)


def gh(method, path, payload=None):
    """`gh api` call; the parsed JSON reply."""
    cmd = ["gh", "api", "-X", method, path] + (["--input", "-"] if payload is not None else [])
    out = subprocess.run(cmd, input=json.dumps(payload) if payload is not None else None,
                         capture_output=True, text=True, check=True).stdout
    return json.loads(out) if out.strip() else None


def title(name):
    return f"Flaky test: {name}"


def body(name, lines, env):
    run = f"{env.get('GITHUB_SERVER_URL', 'https://github.com')}/{env.get('GITHUB_REPOSITORY', '')}/actions/runs/{env.get('GITHUB_RUN_ID', '?')}"
    tail = lines[-TAIL:] if lines else ["(no output recorded in LastTest.log)"]
    return (f"`{name}` failed in the nightly flake watch (`ctest --repeat until-fail:10`) at {env.get('GITHUB_SHA', '?')[:7]}: {run}\n\n"
            f"Last {TAIL} lines of the failing run:\n\n{FENCE}\n" + "\n".join(tail) + f"\n{FENCE}\n")


def report(build_dir, env, api=gh):
    """File each failed test of build_dir; returns the list of (action, test name)."""
    tmp = os.path.join(build_dir, "Testing", "Temporary")
    names = summary.failed_names(tmp)
    if not names:
        return []
    out = summary.outputs(tmp)
    repo = env["GITHUB_REPOSITORY"]
    try:
        api("POST", f"repos/{repo}/labels", {"name": LABEL, "color": "d93f0b", "description": "Failed in the nightly flake watch"})
    except subprocess.CalledProcessError:
        pass  # 422: the label exists
    open_issues = {i["title"]: i["number"] for i in api("GET", f"repos/{repo}/issues?state=open&labels={LABEL}&per_page=100")
                   if "pull_request" not in i}
    done = []
    for name in names:
        text = body(name, out.get(name), env)
        number = open_issues.get(title(name))
        if number:
            api("POST", f"repos/{repo}/issues/{number}/comments", {"body": text})
            done.append(("commented", name))
        else:
            api("POST", f"repos/{repo}/issues", {"title": title(name), "body": text, "labels": [LABEL]})
            done.append(("opened", name))
    return done


def main(argv):
    if len(argv) != 2:
        print(__doc__.strip(), file=sys.stderr)
        return 2
    try:
        for action, name in report(argv[1], os.environ):
            print(f"{action}: {title(name)}")
    except (subprocess.CalledProcessError, KeyError) as e:
        print(f"flake-issues: {e}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.dont_write_bytecode = True
    sys.exit(main(sys.argv))
