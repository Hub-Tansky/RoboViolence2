#!/usr/bin/env python3
"""tools/token-expiry.py (ROBO-252): the expiry header in its two formats, an unreadable one, the 14-day line, a missing header, a
failed request, and the CLI's `alert=` output (ADR ROBO-252)."""
import datetime
import importlib.util
import os
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TOOL = os.path.join(ROOT, "tools", "token-expiry.py")
spec = importlib.util.spec_from_file_location("token_expiry", TOOL)
tool = importlib.util.module_from_spec(spec)
spec.loader.exec_module(tool)
NOW = datetime.datetime(2026, 11, 19, 8, 0, 0, tzinfo=datetime.timezone.utc)
failures = 0


def check(name, cond, detail=""):
    global failures
    if not cond:
        print(f"FAIL {name} {detail}")
        failures += 1


def headers(value):
    return f"HTTP/2 200\r\ncontent-type: application/json\r\ngithub-authentication-token-expiration: {value}\r\nx-github-request-id: 1\r\n\r\n"


# (case, status, headers, alert?, text in the line)
CASES = [
    ("90 days left", 200, headers("2027-02-17 08:00:00 UTC"), False, "expires 2027-02-17 08:00 UTC, in 90 days"),
    ("15 days left", 200, headers("2026-12-04 09:00:00 UTC"), False, "in 15 days"),
    ("exactly 14 days", 200, headers("2026-12-03 08:00:00 UTC"), True, "in 14 days"),
    ("3 days left", 200, headers("2026-11-22 08:00:00 UTC"), True, "in 3 days"),
    ("+0000 offset, header case", 200, headers("2026-11-25 08:00:00 +0000").replace("github-authentication", "GitHub-Authentication"), True, "in 6 days"),
    ("+0200 offset", 200, headers("2026-11-25 10:00:00 +0200"), True, "2026-11-25 08:00 UTC"),
    ("14.5 days: shown and compared as 14", 200, headers("2026-12-03 20:00:00 UTC"), True, "in 14 days"),
    ("ISO format: unreadable", 200, headers("2026-12-31T08:00:00Z"), True, "unreadable expiry header (2026-12-31T08:00:00Z)"),
    ("impossible date: unreadable", 200, headers("2026-13-45 08:00:00 UTC"), True, "unreadable expiry header"),
    ("no header: never expires", 200, "HTTP/2 200\r\ncontent-type: application/json\r\n\r\n", False, "no expiry date"),
    ("401", 401, "HTTP/2 401\r\n\r\n", True, "HTTP 401"),
    ("no answer", 0, "", True, "HTTP nothing"),
]
for name, status, hdr, alert, text in CASES:
    got_alert, line = tool.verdict("SYMBOLS_TOKEN", status, hdr, NOW)
    check(name, bool(got_alert) == alert and text in line, f"alert={got_alert!r} line={line!r}")

with tempfile.TemporaryDirectory() as tmp:
    h = os.path.join(tmp, "h")
    out = os.path.join(tmp, "out")
    for status, value, want in [("200", "2026-11-22 08:00:00 UTC", "alert=SYMBOLS_TOKEN expires 2026-11-22 08:00 UTC, in 3 days: renew it and update the secret\n"),
                                ("200", "2027-11-22 08:00:00 UTC", "alert=\n"), ("000", "2027-11-22 08:00:00 UTC", "alert=SYMBOLS_TOKEN: GitHub answered HTTP nothing; the token is missing, expired or revoked\n")]:
        open(h, "w").write(headers(value))
        open(out, "w").close()
        r = subprocess.run([sys.executable, "-B", TOOL, "SYMBOLS_TOKEN", status, h, "2026-11-19T08:00:00Z"], capture_output=True, text=True,
                           env={**os.environ, "GITHUB_OUTPUT": out})
        check(f"cli {status} {value}", r.returncode == 0 and open(out).read() == want, (r.stderr, open(out).read()))

print("FAILED" if failures else "token expiry: ok")
sys.exit(1 if failures else 0)
