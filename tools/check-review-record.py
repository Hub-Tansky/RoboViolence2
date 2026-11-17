#!/usr/bin/env python3
"""Protocol hash and review record gate (REVIEW.md sections 1 and 5). tools/review.sh runs it.

Env: REVIEW_BASE (merge-base), REVIEW_HEAD (PR head), REVIEW_BODY (PR description),
REVIEW_DRAFT and REVIEW_BOT ("true" skips the record checks).
"""
import hashlib
import importlib.util
import os
import re
import subprocess
import sys

SKILL = ".claude/skills/rv2-code-review/SKILL.md"
DOC_PATHS = re.compile(r"^docs/|\.md$")
BEHAVIOUR_DOCS = re.compile(r"^(REVIEW\.md$|AGENTS\.md$|\.claude/|\.github/|docs/agents/)")
RISK_PATHS = re.compile(r"^(game/src/(netPacket\.h|ServerRecv\.cpp|ClientRecv\.cpp)$|engine/(babonet|credentials)/)")

SPEC_SOURCE = re.compile(r"^Spec source: (ROBO-\d{3}) rv2-docs@[0-9a-f]{7,40} blob:[0-9a-f]{12}( \(uncommitted\))?[ \t]*$", re.M)
DIFF = re.compile(r"^Diff: [0-9a-f]{7,40}\.\.\.([0-9a-f]{7,40})[ \t]*$", re.M)
PATCH_ID = re.compile(r"^Patch-id: ([0-9a-f]{40})[ \t]*$", re.M)
POST_DEPLOY_TAG = "(post-deploy)"
# The diff behind a Patch-id, with every config that changes its text pinned, so a record made on the Mac matches CI.
# The skill's Pin step runs the same command.
PATCH_DIFF = ["-c", "diff.noprefix=false", "-c", "diff.mnemonicPrefix=false", "-c", "diff.renames=true",
              "diff", "--text", "--no-ext-diff", "--no-color", "--no-textconv"]
FIXED = r"fixed in `?[0-9a-f]{7,40}"
OWNER = r"confirmed with owner:\s*\S"
DEFERRED = r"deferred\s*(?:→|->)\s*\[?(?:ROBO-\d{3}\b|#\d+|https://github\.com/Hub-Tansky/RoboViolence2/issues/\d+)"
REJECTED_WITH_EVIDENCE = r"rejected:.*(?:[\w./-]+:\d+|https?://)"
BLOCKER_OUTCOME = re.compile(f"(?i){FIXED}|{OWNER}|{DEFERRED}|{REJECTED_WITH_EVIDENCE}")
SUGGESTION_OUTCOME = re.compile(f"(?i){FIXED}|{OWNER}|{DEFERRED}|{REJECTED_WITH_EVIDENCE}|not taken")


def protocol_line(skill_text):
    """The line SKILL.md must carry: sha256 of the file without its first Protocol line, first 12 hex digits."""
    lines = skill_text.splitlines(True)
    first = next((i for i, l in enumerate(lines) if l.startswith("Protocol: ")), len(lines))
    kept = "".join(lines[:first] + lines[first + 1:])
    return "Protocol: rv2-code-review " + hashlib.sha256(kept.encode()).hexdigest()[:12]


def patch_id(base, head, cwd=None):
    """`git diff --text <base>...<head> | git patch-id --verbatim`, first field: the same for the same change after a merge
    from main that leaves the diff alone (ROBO-224). --verbatim, not --stable alone: a whitespace-only change (Python or
    YAML indentation) must alter it. None when git fails or the diff is empty."""
    try:
        diff = subprocess.run(["git", *PATCH_DIFF, f"{base}...{head}"], cwd=cwd, capture_output=True, check=True).stdout
        out = subprocess.run(["git", "patch-id", "--verbatim"], cwd=cwd, input=diff, capture_output=True, check=True).stdout
    except (OSError, subprocess.CalledProcessError):
        return None
    return out.split()[0].decode() if out.split() else None


def review_tier(changed):
    """'docs' when every path is documentation that doesn't steer agents or the gate, else 'code'."""
    docs = changed and all(DOC_PATHS.search(f) and not BEHAVIOUR_DOCS.match(f) for f in changed)
    return "docs" if docs else "code"


def risk_required(changed):
    return any(RISK_PATHS.match(f) for f in changed)


def sections(record):
    """### headings outside <details>, so raw reports can't add or hide a section."""
    found, name, depth = {}, None, 0
    for line in record.splitlines():
        if line.startswith("<details"):
            depth += 1
        elif line.startswith("</details>"):
            depth -= 1
        elif depth == 0 and line.startswith("### "):
            name = line[4:].strip()
            found[name] = []
            continue
        if name:
            found[name].append(line)
    return {k: "\n".join(v) for k, v in found.items()}


def raw_report(section):
    """The text of the section's one <details> block, or None."""
    blocks = re.findall(r"^<details[^\n]*\n(.*?)^</details>", section, re.M | re.S)
    opened = len(re.findall(r"^<details", section, re.M))
    return blocks[0] if len(blocks) == 1 and opened == 1 else None


def table_rows(text):
    """Cells of each Markdown table row, without separator rows."""
    rows = []
    for line in text.splitlines():
        if re.match(r"^\s*\|.*\|\s*$", line):
            cells = [c.strip() for c in line.strip().strip("|").split("|")]
            if not all(re.fullmatch(r":?-+:?", c) for c in cells):
                rows.append(cells)
    return rows


def finding_count(report, heading):
    part = re.search(rf"^## {heading}[ \t]*\n(.*?)(?=^## |\Z)", report, re.M | re.S)
    return len(re.findall(r"^\d+\.", part.group(1), re.M)) if part else 0


def check_outcomes(section):
    """Returns (problems, rows as dicts)."""
    problems, rows = [], []
    table = table_rows(section)
    if table and table[0][:1] == ["#"]:
        table = table[1:]
    for r in table:
        if len(r) != 5:
            problems.append(f"review gate: an Outcomes row needs 5 columns: {'|'.join(r)[:100]}")
            continue
        row = {"axis": r[1], "finding": r[2], "severity": r[3].lower(), "outcome": r[4]}
        rows.append(row)
        if row["severity"] not in ("blocker", "suggestion"):
            problems.append(f"review gate: severity must be blocker or suggestion: {r[3][:40]}")
        elif not (BLOCKER_OUTCOME if row["severity"] == "blocker" else SUGGESTION_OUTCOME).match(row["outcome"]):
            problems.append(f"review gate: {row['severity']} without a valid outcome: {r[2][:80]}")
        elif re.search(r"(?i)\bdeferred\b", row["outcome"]) and not re.search(f"(?i){DEFERRED}", row["outcome"]):
            problems.append(f"review gate: deferred finding without a ROBO-NNN item or issue link: {r[2][:80]}")
    return problems, rows


def check_spec_matrix(report, outcomes):
    problems = []
    rows = table_rows(report)
    start = next((i for i, r in enumerate(rows) if r and r[0] == "Criterion"), None)
    matrix = rows[start + 1:] if start is not None else []
    if not matrix:
        return ["review gate: the Spec report has no Criterion matrix (not assessable)"]
    for r in matrix:
        status = r[-1].lower()
        if status == "post-deploy":
            if not r[0].startswith(POST_DEPLOY_TAG):
                problems.append(f"review gate: 'post-deploy' needs a criterion starting '{POST_DEPLOY_TAG}': {r[0][:80]}")
        elif status == "partial":
            if not any(o["axis"] == "Spec" and r[0] in o["finding"] and re.match(OWNER, o["outcome"], re.I) for o in outcomes):
                problems.append(f"review gate: partial criterion needs an Outcome starting 'confirmed with owner:': {r[0][:80]}")
        elif status != "pass":
            problems.append(f"review gate: Spec says '{r[-1]}' for: {r[0][:80]}")
    return problems


def check_owner_brief(body, tier_line):
    """A ready MEDIUM or HIGH PR needs '## Owner brief' holding tools/merge-risk.py's line verbatim (ROBO-228)."""
    if tier_line.startswith("Tier: LOW "):
        return []
    brief = re.search(r"^## Owner brief[ \t]*\n(.*?)(?=^## |\Z)", body, re.M | re.S)
    if not brief:
        return [f"review gate: a ready MEDIUM or HIGH PR needs '## Owner brief' with '{tier_line}'"]
    if not re.search(rf"^{re.escape(tier_line)}[ \t]*$", brief.group(1), re.M):
        return [f"review gate: the Owner brief's Tier line must be '{tier_line}' (tools/merge-risk.py)"]
    return []


def check_record(body, changed, head, protocol, current_patch_id=None):
    """Problems of a ready PR's description; an empty list means it may be ready.
    current_patch_id: patch_id() of the PR's diff now, compared with the record's Patch-id when the head moved."""
    item = re.search(r"^\|\s*Item\s*\|(.*)\|[ \t]*$", body, re.M)
    if not re.search(r"^\|\s*Allowed paths\s*\|", body, re.M):
        return ["review gate: a ready PR needs an Allowed paths row"]
    start = re.search(r"^## Review record[ \t]*$", body, re.M)
    if not start:
        return ["review gate: the PR is ready but its description has no '## Review record'"]
    record = body[start.end():]
    problems = []
    if review_tier(changed) == "docs":
        # Docs tier runs no worker (REVIEW.md section 5): the gate and the author's notes are the review.
        parts = sections(record)
        return [f"review gate: the record lacks '### {name}'" for name in ("review.sh", "Not done / unsure") if name not in parts]

    diff = DIFF.search(record)
    if not diff:
        problems.append("review gate: the record lacks 'Diff: <merge-base>...<head>'")
    elif not head.startswith(diff.group(1)):
        recorded = PATCH_ID.search(record)
        if not (recorded and current_patch_id and recorded.group(1) == current_patch_id):
            problems.append(f"review gate: the record's Diff ends in {diff.group(1)}, but the PR head is {head} and the diff's "
                            "Patch-id changed (or the record has none); review the new commits")

    spec_source = SPEC_SOURCE.search(record)
    if not spec_source:
        problems.append("review gate: the record lacks 'Spec source: ROBO-NNN rv2-docs@<sha> blob:<12 hex>'")
    elif not item or spec_source.group(1) not in item.group(1):
        problems.append(f"review gate: Spec source {spec_source.group(1)} isn't in the PR's Item row")

    parts = sections(record)
    for name in ("review.sh", "Outcomes", "Fix proof", "Not done / unsure"):
        if name not in parts:
            problems.append(f"review gate: the record lacks '### {name}'")
    outcome_problems, outcomes = check_outcomes(parts.get("Outcomes", ""))
    problems += outcome_problems

    required = ["Standards", "Spec"]
    if risk_required(changed) or "Risk" in parts:
        required.append("Risk")
    for axis in required:
        report = raw_report(parts[axis]) if axis in parts else None
        if report is None:
            problems.append(f"review gate: '### {axis}' needs its raw report in one <details> block")
            continue
        if not re.search(rf"^[ \t]*{re.escape(protocol)}[ \t]*$", report, re.M):
            problems.append(f"review gate: the {axis} report lacks '{protocol}' (the protocol at this head)")
        for severity, heading in (("blocker", "Blockers"), ("suggestion", "Suggestions")):
            listed = sum(o["axis"] == axis and o["severity"] == severity for o in outcomes)
            if listed < finding_count(report, heading):
                problems.append(f"review gate: the {axis} report has {finding_count(report, heading)} {heading.lower()} but Outcomes lists {listed}")
        if axis == "Spec":
            problems += check_spec_matrix(report, outcomes)
    return problems


def main():
    base, head = os.environ["REVIEW_BASE"], os.environ["REVIEW_HEAD"]
    body = os.environ["REVIEW_BODY"].replace("\r\n", "\n")
    skill = open(SKILL, encoding="utf-8").read()
    protocol = protocol_line(skill)
    changed = subprocess.run(["git", "diff", "--name-only", "--no-renames", f"{base}...HEAD"],
                             capture_output=True, text=True, check=True).stdout.splitlines()

    problems = []
    if protocol not in skill.splitlines():
        problems.append(f"protocol: the skill's Protocol line is stale; set it to '{protocol}'")
    if os.environ.get("REVIEW_DRAFT") != "true" and os.environ.get("REVIEW_BOT") != "true":
        problems += check_record(body, changed, head, protocol, patch_id(base, head))
        risk = importlib.util.spec_from_file_location("merge_risk", os.path.join(os.path.dirname(os.path.abspath(__file__)), "merge-risk.py"))
        merge_risk = importlib.util.module_from_spec(risk)
        risk.loader.exec_module(merge_risk)
        problems += check_owner_brief(body, merge_risk.tier_line(base, head, body))

    print(f"NOTE review tier: {review_tier(changed)}; Risk report required: {'yes' if risk_required(changed) else 'no'}")
    for p in problems:
        print(p)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.dont_write_bytecode = True
    sys.exit(main())
