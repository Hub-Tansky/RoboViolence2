#!/usr/bin/env python3
"""Days until a GitHub token expires, from one API response (ROBO-252, ADR ROBO-252).

Usage: tools/token-expiry.py <name> <HTTP status> <response headers file> [<now, YYYY-MM-DDTHH:MM:SSZ>]
GitHub sends `github-authentication-token-expiration: 2026-12-31 08:00:00 UTC` (some responses `+0000`) for tokens
with an expiry. Writes the verdict to stdout and, under GitHub Actions, `alert=<text>` to $GITHUB_OUTPUT (empty when
fine). Alert: status other than 200, or 14 days or fewer left. No header on a 200: the token never expires, no alert.
Always exits 0; the workflow decides what an alert does. Python 3, stdlib only.
"""
import datetime
import os
import re
import sys

WARN_DAYS = 14
HEADER = re.compile(r"^github-authentication-token-expiration:\s*(\d{4}-\d\d-\d\d \d\d:\d\d:\d\d)\s*(UTC|[+-]\d{4})\s*$", re.I | re.M)


def expiry(headers):
    m = HEADER.search(headers.replace("\r\n", "\n"))
    if not m:
        return None
    zone = datetime.timezone.utc if m.group(2).upper() == "UTC" else datetime.datetime.strptime(m.group(2), "%z").tzinfo
    return datetime.datetime.strptime(m.group(1), "%Y-%m-%d %H:%M:%S").replace(tzinfo=zone)


def verdict(name, status, headers, now):
    """(alert text or "", summary line)"""
    if status != 200:
        text = f"{name}: GitHub answered HTTP {status or 'nothing'}; the token is missing, expired or revoked"
        return text, text
    when = expiry(headers)
    if when is None:
        return "", f"{name}: no expiry date (the token does not expire)"
    days = (when - now).total_seconds() / 86400
    line = f"{name} expires {when.astimezone(datetime.timezone.utc):%Y-%m-%d %H:%M} UTC, in {int(days)} days"
    return (f"{line}: renew it and update the secret" if days <= WARN_DAYS else ""), line


def main():
    if len(sys.argv) not in (4, 5):
        sys.exit(__doc__)
    name, status = sys.argv[1], sys.argv[2]
    with open(sys.argv[3], encoding="utf-8", errors="replace") as f:
        headers = f.read()
    now = (datetime.datetime.strptime(sys.argv[4], "%Y-%m-%dT%H:%M:%SZ").replace(tzinfo=datetime.timezone.utc)
           if len(sys.argv) == 5 else datetime.datetime.now(datetime.timezone.utc))
    alert, line = verdict(name, int(status) if status.isdigit() else 0, headers, now)
    print(line)
    if os.environ.get("GITHUB_OUTPUT"):
        with open(os.environ["GITHUB_OUTPUT"], "a", encoding="utf-8") as f:
            f.write(f"alert={alert}\n")


if __name__ == "__main__":
    main()
