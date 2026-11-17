#!/usr/bin/env python3
"""Failed ctest tests as Markdown for a CI job summary (ROBO-223).

Usage: ci-test-summary.py <ctest build dir>...

Reads Testing/Temporary/LastTestsFailed.log (the failed test names) and LastTest.log (each test's output) in every
directory and prints, per failed test, its name and the last 30 lines of its output, to stdout and, when it is set,
appended to $GITHUB_STEP_SUMMARY. A missing or unreadable directory is skipped; the script always exits 0,
so it can never turn a run red on its own.
"""
import os
import re
import sys

TAIL = 30
FENCE = "````"  # longer than any ``` a test may print


def failed_names(tmp):
    try:
        with open(os.path.join(tmp, "LastTestsFailed.log"), encoding="utf-8", errors="replace") as f:
            return [line.strip().split(":", 1)[-1] for line in f if line.strip()]
    except OSError:
        return []


def outputs(tmp):
    """{test name: output lines} from LastTest.log."""
    result, name, lines = {}, None, None
    try:
        f = open(os.path.join(tmp, "LastTest.log"), encoding="utf-8", errors="replace")
    except OSError:
        return result
    with f:
        for raw in f:
            line = raw.rstrip("\r\n")
            m = re.match(r"^\d+/\d+ Test: (.+)$", line) if lines is None else None  # never inside a test's output
            if m:
                name, lines = m.group(1), None
            elif name and lines is None and line == "Output:":
                lines = []
            elif lines is not None:
                if line == "<end of output>":
                    result[name], name, lines = lines, None, None
                elif not (len(lines) == 0 and set(line) == {"-"}):  # the dash rule right after "Output:"
                    lines.append(line)
    return result


def summary(dirs):
    parts = []
    for d in dirs:
        tmp = os.path.join(d, "Testing", "Temporary")
        names = failed_names(tmp)
        if not names:
            continue
        out = outputs(tmp)
        parts.append(f"### Failed tests in `{d}`\n")
        parts.append("".join(f"- `{n}`\n" for n in names))
        for n in names:
            tail = out.get(n, ["(no output recorded in LastTest.log)"])[-TAIL:]
            parts.append(f"\n**{n}** (last {TAIL} lines)\n\n{FENCE}\n" + "\n".join(tail) + f"\n{FENCE}\n")
    return "\n".join(parts) if parts else "No failed ctest tests recorded (the failure is in another step).\n"


def main(argv):
    text = summary(argv[1:])
    path = os.environ.get("GITHUB_STEP_SUMMARY")
    if path:
        with open(path, "a", encoding="utf-8") as f:
            f.write(text)
    # Also the log's last step, for `gh run view --log-failed`. UTF-8 whatever the console codepage (Windows: cp1252).
    sys.stdout.buffer.write(text.encode("utf-8", "replace"))
    sys.stdout.flush()
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
