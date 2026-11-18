#!/usr/bin/env python3
"""Move a merged PR's backlog item in the rv2-docs vault (ROBO-249, ADR ROBO-249).

Usage: tools/backlog-move.py <vault dir> <PR number> [<date YYYY-MM-DD>]   (the PR description in $PR_BODY)
The item is the first ROBO-NNN in the description's Item row. With an unticked criterion starting `(post-deploy)` it
gets `status: Testing` in 40-backlog/30-inprogress/; otherwise `status: Done` in 40-backlog/90-done/ (rv2-docs AGENTS.md
§ Backlog). It edits `status` and `updated`, runs `git mv` in the vault and writes the commit message
`backlog: ROBO-NNN -> <status> (PR #n)` to $GITHUB_OUTPUT as `message=` (empty when nothing changed); the caller commits.
No ROBO id, no item file or two item files: a warning and exit 0, never a failed merge run.
"""
import datetime
import glob
import importlib.util
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ITEM_ID = re.compile(r"\bROBO-\d{3,}\b")
PENDING_POST_DEPLOY = re.compile(r"^\s*- \[ \] \(post-deploy\)", re.M)
TARGETS = {"Testing": "30-inprogress", "Done": "90-done"}


def item_row():
    spec = importlib.util.spec_from_file_location("check_review_record", os.path.join(ROOT, "tools", "check-review-record.py"))
    gate = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(gate)
    return gate.ITEM_ROW


def item_id(body):
    row = item_row().search((body or "").replace("\r\n", "\n"))
    found = ITEM_ID.search(row.group(1)) if row else None
    return found.group(0) if found else None


def criteria(text):
    """The Acceptance criteria section, up to the next ## heading."""
    m = re.search(r"^## Acceptance criteria[ \t]*\n(.*?)(?=^## |\Z)", text, re.M | re.S)
    return m.group(1) if m else ""


def target_status(text):
    return "Testing" if PENDING_POST_DEPLOY.search(criteria(text)) else "Done"


def set_field(text, name, value):
    """Replace a frontmatter field's value; only inside the leading --- block."""
    head = re.match(r"^---\n.*?\n---\n", text, re.S)
    if not head:
        raise ValueError("no frontmatter")
    block = head.group(0)
    new, n = re.subn(rf"^{name}:.*$", f"{name}: {value}", block, count=1, flags=re.M)
    if n == 0:
        new = block[:-4] + f"{name}: {value}\n---\n"
    return new + text[len(block):]


def move(vault, body, pr, today):
    """Returns (commit message or None, note)."""
    rid = item_id(body)
    if not rid:
        return None, "no ROBO-NNN in the Item row: nothing to move"
    files = [f for f in glob.glob(os.path.join(vault, "40-backlog", "*", f"{rid} *.md")) if "/_" not in f.replace(os.sep, "/")]
    if len(files) != 1:
        return None, f"warning: {len(files)} item files for {rid} in {vault}/40-backlog: nothing moved"
    src = files[0]
    text = open(src, encoding="utf-8").read()
    status = target_status(text)
    current = re.search(r"^status:[ \t]*(.*?)[ \t]*$", text, re.M)
    dest_dir = os.path.join(vault, "40-backlog", TARGETS[status])
    dest = os.path.join(dest_dir, os.path.basename(src))
    if current and current.group(1) == status and os.path.dirname(src) == dest_dir:
        return None, f"{rid} is already {status}: nothing to move"
    text = set_field(set_field(text, "status", status), "updated", today)
    with open(src, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)
    if src != dest:
        os.makedirs(dest_dir, exist_ok=True)
        subprocess.run(["git", "mv", os.path.relpath(src, vault), os.path.relpath(dest, vault)], cwd=vault, check=True)
    subprocess.run(["git", "add", "-A", "40-backlog"], cwd=vault, check=True)
    return f"backlog: {rid} -> {status} (PR #{pr})", f"{rid} -> {status} in {TARGETS[status]}/"


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    vault, pr = sys.argv[1], sys.argv[2]
    today = sys.argv[3] if len(sys.argv) > 3 else datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%d")
    message, note = move(vault, os.environ.get("PR_BODY", ""), pr, today)
    print(f"::warning::{note}" if note.startswith("warning") else note)
    if os.environ.get("GITHUB_OUTPUT"):
        with open(os.environ["GITHUB_OUTPUT"], "a", encoding="utf-8") as f:
            f.write(f"message={message or ''}\n")


if __name__ == "__main__":
    main()
