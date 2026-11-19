#!/usr/bin/env python3
"""Claude Code PreToolUse guard (ROBO-256, ADR ROBO-256). Reads the hook's JSON on stdin.

Bash: denies skipping hooks, a force push to main, a `v*` release tag or its push, `gh auth switch`, `gh secret` and
write calls to `rulesets`, naming the rule. Edit and Write: asks before a change to a guard file (the GUARDS pattern
of tools/merge-risk.py). Anything else: no output, so the normal permission flow applies. Python 3, stdlib only.
"""
import importlib.util
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
_spec = importlib.util.spec_from_file_location("merge_risk", os.path.join(ROOT, "tools", "merge-risk.py"))
_risk = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_risk)
GUARDS = _risk.GUARDS

RULES = [
    ("no --no-verify: hooks are never bypassed (AGENTS.md)", r"\bgit\b.*\s--no-verify\b"),  # review: allow this rule refuses --no-verify
    ("no force push to main (AGENTS.md)", r"\bgit\b.*\bpush\b(?=.*\s(-f|--force(-with-lease)?(=\S*)?|\+\S+)(\s|$))(?=.*(\s|:|\+)main(\s|$))"),
    ("no release tags: the owner tags releases (rv2-docs release.md)", r"\bgit\b.*\btag\b(\s+-\S+)*\s+v\d"),
    ("no push of release tags: the owner pushes them (rv2-docs release.md)", r"\bgit\b.*\bpush\b.*(\srefs/tags/v|\sv\d|\s--tags\b)"),
    ("no gh auth switch: other projects rely on the active account (AGENTS.md, Git identity)", r"\bgh\s+auth\s+switch\b"),
    ("no gh secret: secrets are the owner's (AGENTS.md)", r"\bgh\s+secret\b"),
    ("no writes to rulesets: only the owner changes them (AGENTS.md)",
     r"\bgh\s+api\b(?=.*rulesets)(?=.*(\s(-X|--method)\s*(POST|PUT|PATCH|DELETE)\b|\s(-f|-F|--field|--raw-field|--input)\s))"),
]
SPLIT = re.compile(r"&&|\|\||[;|\n]")


def bash_verdict(command):
    """The broken rule's text, or None"""
    for part in SPLIT.split(command):
        part = " " + part.strip()
        for rule, pattern in RULES:
            if re.search(pattern, part, re.I):
                return rule
    return None


def decide(event):
    """(decision, reason) or None"""
    tool, data = event.get("tool_name", ""), event.get("tool_input", {}) or {}
    if tool == "Bash":
        rule = bash_verdict(data.get("command", ""))
        return ("deny", f"ROBO-256 guard: {rule}") if rule else None
    if tool in ("Edit", "Write", "MultiEdit", "NotebookEdit"):
        path = data.get("file_path") or data.get("notebook_path") or ""
        rel = os.path.relpath(os.path.abspath(path), ROOT).replace(os.sep, "/")
        if not rel.startswith("../") and GUARDS.match(rel):
            return "ask", f"ROBO-256 guard: {rel} is a guard file; the owner approves changes to it (REVIEW.md)"
    return None


def main():
    verdict = decide(json.load(sys.stdin))
    if verdict:
        print(json.dumps({"hookSpecificOutput": {"hookEventName": "PreToolUse", "permissionDecision": verdict[0], "permissionDecisionReason": verdict[1]}}))


if __name__ == "__main__":
    main()
