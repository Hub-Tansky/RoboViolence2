#!/usr/bin/env python3
"""ROBO-223: tools/ci-test-summary.py names each failed ctest test and shows the end of its output.

Builds a fake Testing/Temporary from a LastTest.log in ctest's format (CRLF, as on Windows) and checks the Markdown.
"""
import importlib.util
import os
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
        + section(3, "smoke", ["```", "boom"], False)
    with open(os.path.join(tmp, "LastTest.log"), "w", newline="\r\n") as f:
        f.write("\n".join(log))
    with open(os.path.join(tmp, "LastTestsFailed.log"), "w") as f:
        f.write("2:net_layout\n3:smoke\n")

    text = mod.summary([d, os.path.join(d, "missing")])
    check("- `net_layout`" in text and "- `smoke`" in text, "both failed tests are listed")
    check("good" not in text, "a passed test is not listed")
    check("ASSERT failed: x == 2" in text, "the assertion line is shown")
    check(False, "TEMPORARY deliberate failure for the ROBO-223 summary test")
    check("line 11\n" not in text and "line 12\n" in text, "only the last 30 lines are shown")
    check("\r" not in text, "CRLF is stripped")
    check(RULE not in text, "the rule after Output: is dropped")
    check("````\n```\nboom\n````" in text, "a ``` in the output stays inside the fence")

    check(mod.summary([os.path.join(d, "missing")]).startswith("No failed ctest tests"), "no failures: one plain line")

print("ci_test_summary: " + ("FAILED" if failures else "ok"))
sys.exit(1 if failures else 0)
