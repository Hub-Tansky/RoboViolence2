#!/usr/bin/env python3
"""One week of PR and CI numbers as a CSV row (ROBO-253, ADR ROBO-253).

Usage: tools/workflow-metrics.py <csv file> [<week end, YYYY-MM-DD, a Monday>]
The week runs from the Monday 00:00 UTC before <week end> up to <week end> 00:00 UTC; without it, the last full week.
Reads the GitHub REST API for $GITHUB_REPOSITORY (default Hub-Tansky/RoboViolence2) with $GH_TOKEN if set.
Appends one row to <csv file> (writing the header first when the file is new) unless a row for that week is there.
Under GitHub Actions writes `message=<commit message>` to $GITHUB_OUTPUT (empty when no row was added).
Python 3, stdlib only.

Columns:
  week_start, week_end      Monday dates (UTC); week_end is excluded
  prs_merged                PRs merged in the week (any base)
  merge_hours_median/p90    hours from opened to merged (p90: nearest rank)
  review_commits_per_pr     commits whose subject has `review:`, per merged PR
  main_merges_per_pr        merge commits (2+ parents) in the PR, per merged PR; in this repo these are merges from main
                            to stay up to date, a merge from another branch counts too
  build_runs                runs of the `build` workflow created in the week, by their latest attempt's conclusion:
  build_success/failure/cancelled/other
  build_reruns              of those, runs with more than one attempt
"""
import csv
import datetime
import json
import os
import statistics
import sys
import urllib.request

COLUMNS = ["week_start", "week_end", "prs_merged", "merge_hours_median", "merge_hours_p90", "review_commits_per_pr", "main_merges_per_pr",
           "build_runs", "build_success", "build_failure", "build_cancelled", "build_other", "build_reruns"]
UTC = datetime.timezone.utc


def when(text):
    return datetime.datetime.strptime(text, "%Y-%m-%dT%H:%M:%SZ").replace(tzinfo=UTC)


def week(end=None, today=None):
    """(start, end) datetimes of a Monday-to-Monday week; default: the last full one before `today`."""
    if end is None:
        today = today or datetime.datetime.now(UTC).date()
        end = today - datetime.timedelta(days=today.weekday())
    if end.weekday() != 0:
        raise ValueError(f"{end} is not a Monday")
    end = datetime.datetime(end.year, end.month, end.day, tzinfo=UTC)
    return end - datetime.timedelta(days=7), end


def p90(values):
    """Nearest-rank 90th percentile."""
    ordered = sorted(values)
    return ordered[max(0, -(-9 * len(ordered) // 10) - 1)]


def row(start, end, prs, commits, runs):
    """prs: REST pull objects; commits: {PR number: REST commit objects}; runs: REST workflow runs of `build`."""
    merged = [p for p in prs if p.get("merged_at") and start <= when(p["merged_at"]) < end]
    hours = [(when(p["merged_at"]) - when(p["created_at"])).total_seconds() / 3600 for p in merged]
    review = sum("review:" in c["commit"]["message"].split("\n", 1)[0] for p in merged for c in commits.get(p["number"], []))
    merges = sum(len(c["parents"]) > 1 for p in merged for c in commits.get(p["number"], []))
    runs = [r for r in runs if start <= when(r["created_at"]) < end]
    by = {k: sum(r.get("conclusion") == k for r in runs) for k in ("success", "failure", "cancelled")}
    n = len(merged)
    return {
        "week_start": start.date().isoformat(), "week_end": end.date().isoformat(), "prs_merged": n,
        "merge_hours_median": f"{statistics.median(hours):.1f}" if n else "", "merge_hours_p90": f"{p90(hours):.1f}" if n else "",
        "review_commits_per_pr": f"{review / n:.2f}" if n else "", "main_merges_per_pr": f"{merges / n:.2f}" if n else "",
        "build_runs": len(runs), "build_success": by["success"], "build_failure": by["failure"], "build_cancelled": by["cancelled"],
        "build_other": len(runs) - sum(by.values()), "build_reruns": sum(r.get("run_attempt", 1) > 1 for r in runs),
    }


def append(path, values):
    """Adds the row unless one for the same week_start is there. True when added."""
    new = not os.path.exists(path) or os.path.getsize(path) == 0
    if not new:
        with open(path, newline="") as f:
            if any(r.get("week_start") == values["week_start"] for r in csv.DictReader(f)):
                return False
    with open(path, "a", newline="") as f:
        w = csv.DictWriter(f, fieldnames=COLUMNS, lineterminator="\n")
        if new:
            w.writeheader()
        w.writerow(values)
    return True


def get(path):
    req = urllib.request.Request(f"https://api.github.com{path}", headers={"Accept": "application/vnd.github+json", "X-GitHub-Api-Version": "2022-11-28"})
    if os.environ.get("GH_TOKEN"):
        req.add_header("Authorization", f"Bearer {os.environ['GH_TOKEN']}")
    with urllib.request.urlopen(req, timeout=30) as r:
        return json.load(r)


def fetch(repo, start, end):
    prs = []
    for page in range(1, 50):
        batch = get(f"/repos/{repo}/pulls?state=closed&sort=updated&direction=desc&per_page=100&page={page}")
        prs += batch
        # A PR merged in the week was updated at or after its merge, so stop at the first one updated before the week
        if len(batch) < 100 or when(batch[-1]["updated_at"]) < start:
            break
    merged = [p for p in prs if p.get("merged_at") and start <= when(p["merged_at"]) < end]
    commits = {p["number"]: get(f"/repos/{repo}/pulls/{p['number']}/commits?per_page=100") for p in merged}
    runs = []
    span = f"{start.date().isoformat()}..{(end - datetime.timedelta(days=1)).date().isoformat()}"
    for page in range(1, 50):
        batch = get(f"/repos/{repo}/actions/workflows/build.yml/runs?created={span}&per_page=100&page={page}")["workflow_runs"]
        runs += batch
        if len(batch) < 100:
            break
    return prs, commits, runs


def main(argv):
    if len(argv) not in (2, 3):
        sys.exit(__doc__)
    end = datetime.date.fromisoformat(argv[2]) if len(argv) == 3 else None
    last = week()[1]
    start, end = week(end)
    if end > last:
        sys.exit(f"week ending {end.date()} has not ended yet: a row now would stay partial for good")
    repo = os.environ.get("GITHUB_REPOSITORY") or "Hub-Tansky/RoboViolence2"
    values = row(start, end, *fetch(repo, start, end))
    added = append(argv[1], values)
    print(", ".join(f"{k} {v}" for k, v in values.items()))
    print("row added" if added else "row for this week already there: nothing added")
    message = f"metrics: workflow week {values['week_start']}" if added else ""
    if os.environ.get("GITHUB_OUTPUT"):
        with open(os.environ["GITHUB_OUTPUT"], "a") as f:
            f.write(f"message={message}\n")


if __name__ == "__main__":
    main(sys.argv)
