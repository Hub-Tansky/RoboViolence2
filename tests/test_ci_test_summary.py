#!/usr/bin/env python3
"""ROBO-223: tools/ci-test-summary.py names each failed ctest test and shows the end of its output.

Builds a fake Testing/Temporary from a LastTest.log in ctest's format (CRLF, as on Windows) and checks the Markdown.
"""
import importlib.util
import os
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
spec = importlib.util.spec_from_file_location("ci_test_summary", os.path.join(ROOT, "tools", "ci-test-summary.py"))
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)

RULE = "-" * 58


def section(i, name, output, passed):
    return [f"{i}/3 Testing: {name}", f"{i}/3 Test: {name}", f'Command: "/bin/{name}"', "Directory: /b", "Output:", RULE,
            *output, "<end of output>", "Test time =   0.01 sec", RULE, "Test Passed." if passed else "Test Failed.", RULE, ""]


failures = 0


def check(cond, what):
    global failures
    if not cond:
        failures += 1
        print("FAIL:", what)


with tempfile.TemporaryDirectory() as d:
    tmp = os.path.join(d, "Testing", "Temporary")
    os.makedirs(tmp)
    long_out = [f"line {i}" for i in range(1, 41)] + ["ASSERT failed: x == 2"]
    log = ["Start testing", RULE] + section(1, "good", ["fine"], True) + section(2, "net_layout", long_out, False) \
        + section(3, "smoke", ["```", "1/9 Test: nested ctest progress", "caf\u00e9 \u2713", "boom"], False)
    with open(os.path.join(tmp, "LastTest.log"), "w", newline="\r\n") as f:
        f.write("\n".join(log))
    with open(os.path.join(tmp, "LastTestsFailed.log"), "w") as f:
        f.write("2:net_layout\n3:smoke\n")

    text = mod.summary([d, os.path.join(d, "missing")])
    check("- `net_layout`" in text and "- `smoke`" in text, "both failed tests are listed")
    check("good" not in text, "a passed test is not listed")
    check("ASSERT failed: x == 2" in text, "the assertion line is shown")
    check("line 11\n" not in text and "line 12\n" in text, "only the last 30 lines are shown")
    check("\r" not in text, "CRLF is stripped")
    check(RULE not in text, "the rule after Output: is dropped")

    check("````\n```\n1/9 Test: nested ctest progress\ncaf\u00e9 \u2713\nboom\n````" in text, "a ctest-like line inside output keeps the tail")
    check(mod.summary([os.path.join(d, "missing")]).startswith("No failed ctest tests"), "no failures: one plain line")

    # main(): appends to $GITHUB_STEP_SUMMARY, writes UTF-8 to a cp1252 console (Windows runner), returns 0.
    step = os.path.join(d, "summary.md")
    with open(step, "w", encoding="utf-8") as f:
        f.write("earlier step\n")
    code = subprocess.run([sys.executable, "-B", os.path.join(ROOT, "tools", "ci-test-summary.py"), d], capture_output=True,
                          env=dict(os.environ, GITHUB_STEP_SUMMARY=step, PYTHONIOENCODING="cp1252")).returncode
    with open(step, encoding="utf-8") as f:
        written = f.read()
    check(code == 0, "main() exits 0 with non-cp1252 output on a cp1252 console")
    check(written.startswith("earlier step\n") and "caf\u00e9 \u2713" in written, "the summary is appended after earlier text")

print("ci_test_summary: " + ("FAILED" if failures else "ok"))
sys.exit(1 if failures else 0)
