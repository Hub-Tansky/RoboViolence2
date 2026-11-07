#!/usr/bin/env python3
"""tools/check-review-record.py: tier, protocol hash and every way a ready PR's record can fail."""
import importlib.util
import os
import sys

sys.dont_write_bytecode = True
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
spec = importlib.util.spec_from_file_location("gate", os.path.join(ROOT, "tools", "check-review-record.py"))
gate = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gate)

HEAD = "a" * 40
PROTOCOL = gate.protocol_line("# skill\n")
CODE = ["game/src/Game.cpp"]
DOCS = ["docs/build/README.md"]
RISKY = ["game/src/netPacket.h"]
MATRIX = "| Criterion | Diff/test evidence | Status |\n|---|---|---|\n| It works | `a.cpp:1`, test_a | pass |\n"


def report(extra="", protocol=PROTOCOL):
    return f"<details><summary>Raw report</summary>\n\n{protocol}\n{extra}\n</details>\n"


def body(standards=None, spec=None, risk="", outcomes="", diff=f"Diff: {'b' * 7}...{HEAD[:7]}",
         source="Spec source: ROBO-181 rv2-docs@cccc123 blob:0123456789ab", head="| Item | ROBO-181 |\n| Allowed paths | `a` |\n"):
    standards = report() if standards is None else standards
    spec = report(MATRIX) if spec is None else spec
    risk_part = f"### Risk\n\n{risk}\n" if risk else ""
    return (f"{head}\n## Review record\n\n{diff}\n{source}\n\n### review.sh\n\npassed\n\n### Standards\n\n{standards}\n"
            f"### Spec\n\n{spec}\n{risk_part}### Outcomes\n\n| # | Axis | Finding | Severity | Outcome |\n|---|---|---|---|---|\n{outcomes}\n"
            "### Fix proof\n\nnone\n\n### Not done / unsure\n\nnone\n")


def row(axis="Standards", finding="x", severity="blocker", outcome="fixed in abc1234"):
    return f"| 1 | {axis} | {finding} | {severity} | {outcome} |\n"


# (case, description, changed paths, text the single problem must contain; None means it passes)
CASES = [
    ("good record", body(), CODE, None),
    ("template comment isn't a record", "| Allowed paths | `a` |\n<!-- add \"## Review record\" -->\n", CODE, "no '## Review record'"),
    ("no Allowed paths row", "## Review record\n", CODE, "Allowed paths row"),
    ("no record", "| Allowed paths | `a` |\n", CODE, "no '## Review record'"),
    ("old two-dot diff", body(diff=f"Diff: bbbbbbb..{HEAD[:7]}"), CODE, "lacks 'Diff"),
    ("stale head", body(diff="Diff: bbbbbbb...ddddddd"), CODE, "review the new commits"),
    ("no spec source", body(source="Spec source: not assessable"), CODE, "lacks 'Spec source"),
    ("uncommitted spec source", body(source="Spec source: ROBO-181 rv2-docs@cccc123 blob:0123456789ab (uncommitted)"), CODE, None),
    ("spec source isn't the item", body(source="Spec source: ROBO-999 rv2-docs@cccc123 blob:0123456789ab"), CODE, "Item row"),
    ("standards outside details", body(standards=f"{PROTOCOL}\n"), CODE, "'### Standards' needs its raw report"),
    ("two rounds of raw reports", body(standards=report() + report()), CODE, "'### Standards' needs its raw report"),
    ("stale protocol", body(standards=report(protocol="Protocol: rv2-code-review 000000000000")), CODE, "the Standards report lacks"),
    ("no spec on code", body(spec="none"), CODE, "'### Spec' needs its raw report"),
    ("heading in a raw report isn't a section", body(standards=report("### Outcomes"), spec=report(MATRIX + "### Risk")), CODE, None),
    ("docs tier needs no spec", body(spec="none"), DOCS, None),
    ("risk path needs Risk", body(), RISKY, "'### Risk' needs its raw report"),
    ("risk path with Risk", body(risk=report()), RISKY, None),
    ("spec not assessable", body(spec=report("no matrix")), CODE, "not assessable"),
    ("spec fail", body(spec=report(MATRIX.replace("pass", "fail"))), CODE, "Spec says 'fail'"),
    ("spec partial alone", body(spec=report(MATRIX.replace("pass", "partial"))), CODE, "partial criterion"),
    ("spec partial confirmed", body(spec=report(MATRIX.replace("pass", "partial")),
                                    outcomes=row("Spec", "It works", "suggestion", "confirmed with owner: later")), CODE, None),
    ("blocker fixed", body(outcomes=row()), CODE, None),
    ("blocker pending", body(outcomes=row(outcome="pending")), CODE, "blocker without a valid outcome"),
    ("blocker rejected without evidence", body(outcomes=row(outcome="rejected: disagree")), CODE, "blocker without a valid outcome"),
    ("blocker rejected with evidence", body(outcomes=row(outcome="rejected: see game/src/Game.cpp:12")), CODE, None),
    ("suggestion not taken", body(outcomes=row(severity="suggestion", outcome="not taken")), CODE, None),
    ("blocker not taken", body(outcomes=row(outcome="not taken")), CODE, "blocker without a valid outcome"),
    ("unknown severity", body(outcomes=row(severity="major")), CODE, "severity must be"),
    ("deferred without item", body(outcomes=row(severity="suggestion", outcome="not taken, deferred")), CODE, "deferred finding"),
    ("deferred with item", body(outcomes=row(severity="suggestion", outcome="deferred → ROBO-123")), CODE, None),
    ("report blocker dropped from outcomes", body(standards=report("## Blockers\n1. `a.cpp:1` bad\n")), CODE, "has 1 blockers but Outcomes lists 0"),
    ("report blocker downgraded", body(standards=report("## Blockers\n1. `a.cpp:1` bad\n"),
                                       outcomes=row(severity="suggestion", outcome="not taken")), CODE, "has 1 blockers but Outcomes lists 0"),
    ("report blocker listed", body(standards=report("## Blockers\n1. `a.cpp:1` bad\n## Suggestions\n1. y\n"), outcomes=row()), CODE, None),
]

failed = 0
for name, text, changed, expected in CASES:
    problems = gate.check_record(text, changed, HEAD, PROTOCOL)
    good = not problems if expected is None else len(problems) == 1 and expected in problems[0]
    if not good:
        failed += 1
        print(f"FAIL {name}: {problems}")

TIERS = [
    (["docs/a.md", "README.md"], "docs"),
    (["docs/img.png"], "docs"),
    (["engine/dko/README.md"], "docs"),
    (["docs/a.md", "game/src/a.cpp"], "code"),
    (["REVIEW.md"], "code"),
    (["AGENTS.md"], "code"),
    ([".claude/skills/x/SKILL.md"], "code"),
    ([".github/pull_request_template.md"], "code"),
    (["docs/agents/domain.md"], "code"),
    ([], "code"),
]
for changed, expected in TIERS:
    if gate.review_tier(changed) != expected:
        failed += 1
        print(f"FAIL tier {changed}: expected {expected}")

for path, expected in [("game/src/ServerRecv.cpp", True), ("engine/credentials/src/Credentials.cpp", True), ("game/src/Server.cpp", False)]:
    if gate.risk_required([path]) != expected:
        failed += 1
        print(f"FAIL risk {path}: expected {expected}")

a = gate.protocol_line("x\nProtocol: rv2-code-review old\ny\n")
if a != gate.protocol_line("x\nProtocol: rv2-code-review new\ny\n") or a == gate.protocol_line("x\nProtocol: same\nz\n"):
    failed += 1
    print("FAIL protocol hash: must ignore its own line and cover the rest")

print("review record: FAILED" if failed else "review record: ok")
sys.exit(1 if failed else 0)
