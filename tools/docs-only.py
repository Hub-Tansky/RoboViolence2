#!/usr/bin/env python3
"""Is this CI run a docs-only pull request (ROBO-246, ADR 0067)? Prints `docs=true` or `docs=false`.

Usage: tools/docs-only.py <event name>
Only a pull_request can be docs-only: push (main, release/*, tags), schedule and workflow_dispatch always run
everything. Its changed paths are the checkout's merge commit against its first parent (the base branch), so the
checkout needs fetch-depth 2. Docs-only is the review gate's docs tier (review_tier() in tools/check-review-record.py):
every path under docs/ or a .md file, none of them AGENTS.md, REVIEW.md, .claude/, .github/ or docs/agents/.
Writes `docs=` to $GITHUB_OUTPUT and DOCS_ONLY to $GITHUB_ENV when they are set. Any error prints false.
"""
import importlib.util
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def review_tier(changed):
    spec = importlib.util.spec_from_file_location("check_review_record", os.path.join(ROOT, "tools", "check-review-record.py"))
    gate = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(gate)
    return gate.review_tier(changed)


def docs_only(event, changed):
    return event == "pull_request" and bool(changed) and review_tier(changed) == "docs"


def changed_paths(cwd=ROOT):
    out = subprocess.run(["git", "diff", "--name-only", "--no-renames", "HEAD^1", "HEAD"], cwd=cwd,
                         capture_output=True, text=True, check=True).stdout
    return [line for line in out.splitlines() if line]


def main():
    event = sys.argv[1] if len(sys.argv) > 1 else ""
    try:
        changed = changed_paths() if event == "pull_request" else []
        docs = docs_only(event, changed)
        print(f"{len(changed)} changed paths; docs only: {docs}", file=sys.stderr)
    except Exception as e:  # noqa: BLE001  any doubt runs everything
        print(f"docs-only check failed, running everything: {e}", file=sys.stderr)
        docs = False
    value = "true" if docs else "false"
    for name, line in (("GITHUB_OUTPUT", f"docs={value}\n"), ("GITHUB_ENV", f"DOCS_ONLY={value}\n")):
        if os.environ.get(name):
            with open(os.environ[name], "a", encoding="utf-8") as f:
                f.write(line)
    print(f"docs={value}")


if __name__ == "__main__":
    main()
