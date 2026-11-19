#!/usr/bin/env python3
"""tools/workflow-metrics.py (ROBO-253): the week window, the numbers on fixed PRs, commits and runs, the nearest-rank p90,
an empty week, and a second append for the same week adding nothing (ADR ROBO-253)."""
import csv
import datetime
import importlib.util
import os
import sys
import tempfile

sys.dont_write_bytecode = True
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
spec = importlib.util.spec_from_file_location("workflow_metrics", os.path.join(ROOT, "tools", "workflow-metrics.py"))
tool = importlib.util.module_from_spec(spec)
spec.loader.exec_module(tool)
failures = 0


def check(name, cond, detail=""):
    global failures
    if not cond:
        print(f"FAIL {name} {detail}")
        failures += 1


def pr(number, created, merged):
    return {"number": number, "created_at": created, "merged_at": merged, "updated_at": merged or created}


def commit(subject, parents=1):
    return {"commit": {"message": subject + "\n\nbody with review: in it"}, "parents": [{}] * parents}


# Window: the last full week, Monday to Monday
start, end = tool.week(today=datetime.date(2026, 11, 19))
check("default week", (start.isoformat(), end.isoformat()) == ("2026-11-09T00:00:00+00:00", "2026-11-16T00:00:00+00:00"), (start, end))
start, end = tool.week(today=datetime.date(2026, 11, 16))
check("run on a Monday takes the week that just ended", start.date().isoformat() == "2026-11-09", start)
try:
    tool.week(datetime.date(2026, 11, 17))
    check("end not a Monday is refused", False)
except ValueError:
    pass

start, end = tool.week(datetime.date(2026, 11, 16))
PRS = [
    pr(1, "2026-11-09T08:00:00Z", "2026-11-09T10:00:00Z"),  # 2 h
    pr(2, "2026-11-08T20:00:00Z", "2026-11-09T04:00:00Z"),  # 8 h, opened before the week
    pr(3, "2026-11-10T00:00:00Z", "2026-11-11T00:00:00Z"),  # 24 h
    pr(4, "2026-11-01T00:00:00Z", "2026-11-08T23:59:59Z"),  # merged before the week
    pr(5, "2026-11-15T00:00:00Z", "2026-11-16T00:00:00Z"),  # merged at week end: next week
    pr(6, "2026-11-12T00:00:00Z", None),  # closed unmerged
]
COMMITS = {
    1: [commit("feat: x (ROBO-1)"), commit("fix: review: y")],
    2: [commit("ci: z"), commit("Merge branch 'main' into b", 2), commit("docs: review: w"), commit("test: review: v")],
    3: [commit("chore: a"), commit("Merge branch 'main' into c", 2)],
    4: [commit("fix: review: not counted")],
}
RUNS = [
    {"created_at": "2026-11-09T01:00:00Z", "conclusion": "success", "run_attempt": 1},
    {"created_at": "2026-11-10T01:00:00Z", "conclusion": "failure", "run_attempt": 1},
    {"created_at": "2026-11-11T01:00:00Z", "conclusion": "success", "run_attempt": 2},
    {"created_at": "2026-11-12T01:00:00Z", "conclusion": "cancelled", "run_attempt": 1},
    {"created_at": "2026-11-13T01:00:00Z", "conclusion": None, "run_attempt": 1},  # still running
    {"created_at": "2026-11-16T00:00:00Z", "conclusion": "failure", "run_attempt": 3},  # next week
]
got = tool.row(start, end, PRS, COMMITS, RUNS)
want = {"week_start": "2026-11-09", "week_end": "2026-11-16", "prs_merged": 3, "merge_hours_median": "8.0", "merge_hours_p90": "24.0",
        "review_commits_per_pr": "1.00", "main_merges_per_pr": "0.67", "build_runs": 5, "build_success": 2, "build_failure": 1,
        "build_cancelled": 1, "build_other": 1, "build_reruns": 1}
check("row", got == want, got)
check("row has every column in order", list(got) == tool.COLUMNS, list(got))

check("p90 of 10", tool.p90(list(range(1, 11))) == 9)
check("p90 of 11", tool.p90(list(range(1, 12))) == 10)
check("p90 of 1", tool.p90([5]) == 5)

empty = tool.row(start, end, [], {}, [])
check("empty week", empty["prs_merged"] == 0 and empty["merge_hours_median"] == "" and empty["review_commits_per_pr"] == "" and empty["build_runs"] == 0, empty)

with tempfile.TemporaryDirectory() as tmp:
    path = os.path.join(tmp, "workflow-metrics.csv")
    check("first append adds", tool.append(path, got))
    check("same week adds nothing", not tool.append(path, got))
    later = dict(got, week_start="2026-11-16", week_end="2026-11-23")
    check("next week adds", tool.append(path, later))
    with open(path, newline="") as f:
        lines = f.read().splitlines()
    check("one header, two rows", lines[0] == ",".join(tool.COLUMNS) and len(lines) == 3, lines)
    with open(path, newline="") as f:
        check("rows read back", [r["week_start"] for r in csv.DictReader(f)] == ["2026-11-09", "2026-11-16"])

print("ok" if failures == 0 else f"{failures} failed")
sys.exit(1 if failures else 0)
