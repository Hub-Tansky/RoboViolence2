#!/usr/bin/env python3
"""ROBO-237: tools/flake-issues.py opens one issue per failed test and comments on an already open one.

Builds a fake Testing/Temporary in ctest's format and replaces `gh api` with a recorder.
"""
import importlib.util
import os
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
spec = importlib.util.spec_from_file_location("flake_issues", os.path.join(ROOT, "tools", "flake-issues.py"))
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)

RULE = "-" * 58
failures = 0


def check(cond, what):
    global failures
    print(("ok   " if cond else "FAIL ") + what)
    failures += 0 if cond else 1


def section(name, output, passed):
    return [f"1/2 Testing: {name}", f"1/2 Test: {name}", f'Command: "/bin/{name}"', "Directory: /b", "Output:", RULE,
            *output, "<end of output>", "Test time =   0.01 sec", RULE, "Test Passed." if passed else "Test Failed.", RULE, ""]


class Api:
    def __init__(self, open_issues, label_exists):
        self.calls, self.open_issues, self.label_exists = [], open_issues, label_exists

    def __call__(self, method, path, payload=None):
        self.calls.append((method, path, payload))
        if path.endswith("/labels") and self.label_exists:
            raise subprocess.CalledProcessError(1, "gh", "422 already_exists")
        if method == "GET":
            return self.open_issues
        return {}


env = {"GITHUB_REPOSITORY": "o/r", "GITHUB_RUN_ID": "42", "GITHUB_SHA": "abcdef0123", "GITHUB_SERVER_URL": "https://github.com"}

with tempfile.TemporaryDirectory() as d:
    tmp = os.path.join(d, "Testing", "Temporary")
    os.makedirs(tmp)
    log = (section("e2e_chat", ["fine"], True) + section("bots", ["first run ok"], True)
           + section("bots", ["bot 3 stuck", "assert failed"], False) + section("e2e_weapon_choice", ["timeout"], False))
    with open(os.path.join(tmp, "LastTest.log"), "w", encoding="utf-8") as f:
        f.write("\n".join(log))
    with open(os.path.join(tmp, "LastTestsFailed.log"), "w", encoding="utf-8") as f:
        f.write("2:bots\n13:e2e_weapon_choice\n")

    api = Api([{"title": "Flaky test: e2e_weapon_choice", "number": 7}, {"title": "Flaky test: bots", "number": 9, "pull_request": {}}], True)
    done = mod.report(d, env, api)
    check(done == [("opened", "bots"), ("commented", "e2e_weapon_choice")], f"opens a new issue, comments on the open one: {done}")
    posts = [c for c in api.calls if c[0] == "POST" and not c[1].endswith("/labels")]
    check(posts[0][1] == "repos/o/r/issues" and posts[0][2]["title"] == "Flaky test: bots" and posts[0][2]["labels"] == ["flaky-test"],
          "new issue is titled with the test name and labelled")
    check("bot 3 stuck\nassert failed" in posts[0][2]["body"] and "first run ok" not in posts[0][2]["body"],
          "issue body holds the failing run's output, not an earlier passing run")
    check("abcdef0" in posts[0][2]["body"] and "https://github.com/o/r/actions/runs/42" in posts[0][2]["body"], "issue body names the commit and the run")
    check(posts[1][1] == "repos/o/r/issues/7/comments", "an open issue for the test gets a comment")
    check(any(c[1] == "repos/o/r/issues?state=open&labels=flaky-test&per_page=100" for c in api.calls), "looks up open flaky-test issues")

    os.remove(os.path.join(tmp, "LastTestsFailed.log"))
    api = Api([], False)
    check(mod.report(d, env, api) == [] and api.calls == [], "no failed tests: no API calls")

print(f"{failures} failures")
sys.exit(1 if failures else 0)
