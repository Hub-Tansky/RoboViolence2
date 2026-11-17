#!/usr/bin/env python3
"""Merge risk tier of a PR (ROBO-228, ADR 0060): prints one line `Tier: LOW|MEDIUM|HIGH (<reason>)`.

Usage: tools/merge-risk.py [--base <ref>] [--head <ref>] [--body <file>]
  --base  diff from merge-base(base, head) (default origin/main); --head default HEAD
  --body  the PR description (default $REVIEW_BODY); its Item row marked [GUI] makes the PR HIGH
Rules, first match wins: HIGH, then MEDIUM, then LOW (table in AGENTS.md "Agent rules"). Any error prints HIGH.
"""
import importlib.util
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
# Same pattern as GUARDS in tools/review.sh (tests/test_merge_risk.py checks they match).
GUARDS = re.compile(r"^(tools/check-|tools/review\.sh$|tools/original-assets|\.githooks/|\.github/|\.gitleaks\.toml$|\.gitignore$|REVIEW\.md$)")
PACKAGING = re.compile(r"^packaging/")
GAME_VERSION = re.compile(r"^[+-](?![+-]).*\bGAME_VERSION_(SV|CL)\b", re.M)
GAME_CODE = re.compile(r"^(game|engine|masterserver|content)/")
LOW_PATHS = re.compile(r"^(tests|tools)/")
MAX_ADDED, MAX_FILES = 600, 25


def _gate():
    spec = importlib.util.spec_from_file_location("check_review_record", os.path.join(ROOT, "tools", "check-review-record.py"))
    gate = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(gate)
    return gate


def item_is_gui(body):
    row = re.search(r"^\|\s*Item\s*\|(.*)\|[ \t]*$", body or "", re.M)
    return bool(row and "[GUI]" in row.group(1))


def classify(changed, added, diff_text, body):
    """(tier, reason) from changed paths, added line count, the diff's text and the PR body."""
    gate = _gate()
    high = [f for f in changed if gate.risk_required([f]) or PACKAGING.match(f) or GUARDS.match(f)]
    if high:
        return "HIGH", f"{high[0]}" + (f" and {len(high) - 1} more risk paths" if len(high) > 1 else "")
    if GAME_VERSION.search(diff_text):
        return "HIGH", "GAME_VERSION_SV/CL changes"
    if item_is_gui(body):
        return "HIGH", "Item is [GUI]"
    if added > MAX_ADDED:
        return "HIGH", f"{added} added lines"
    if len(changed) > MAX_FILES:
        return "HIGH", f"{len(changed)} changed files"
    game = [f for f in changed if GAME_CODE.match(f)]
    if game:
        return "MEDIUM", f"{game[0]}" + (f" and {len(game) - 1} more game files" if len(game) > 1 else "")
    if gate.review_tier(changed) == "docs":
        return "LOW", "docs only"
    if all(LOW_PATHS.match(f) or gate.DOC_PATHS.search(f) for f in changed):
        return "LOW", "tests, tooling and docs only"
    return "MEDIUM", "build or repo files"


def tier_line(base, head="HEAD", body="", cwd=None):
    """The line this script prints; HIGH on any error."""
    try:
        git = lambda *a: subprocess.run(["git", *a], cwd=cwd, capture_output=True, text=True, check=True).stdout
        mb = git("merge-base", base, head).strip()
        changed = git("diff", "--name-only", "--no-renames", f"{mb}...{head}").splitlines()
        added = sum(int(a) for a, *_ in (l.split("\t") for l in git("diff", "--numstat", "--no-renames", f"{mb}...{head}").splitlines()) if a.isdigit())
        diff_text = git("diff", "--text", "-U0", "--no-color", "--no-ext-diff", f"{mb}...{head}")
        tier, reason = classify(changed, added, diff_text, body)
    except Exception as e:  # noqa: BLE001: any failure is HIGH by rule
        tier, reason = "HIGH", f"merge-risk error: {type(e).__name__}"
    return f"Tier: {tier} ({reason})"


def main(argv):
    base, head, body = "origin/main", "HEAD", os.environ.get("REVIEW_BODY", "")
    try:
        args = iter(argv)
        for a in args:
            if a == "--base":
                base = next(args)
            elif a == "--head":
                head = next(args)
            elif a == "--body":
                body = open(next(args), encoding="utf-8").read()
            else:
                raise ValueError(a)
    except Exception as e:  # noqa: BLE001
        print(f"Tier: HIGH (merge-risk error: {type(e).__name__})")
        return 0
    print(tier_line(base, head, body))
    return 0


if __name__ == "__main__":
    sys.dont_write_bytecode = True
    sys.exit(main(sys.argv[1:]))
