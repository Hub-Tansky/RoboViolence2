#!/usr/bin/env python3
"""tools/project-status.py moves the item named in the PR's Item row to the status for the event (offline; GraphQL
is faked).

Usage: test_project_status.py <scratch dir>
"""
import importlib.util
import json
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
spec = importlib.util.spec_from_file_location("project_status", os.path.join(ROOT, "tools", "project-status.py"))
project_status = importlib.util.module_from_spec(spec)
spec.loader.exec_module(project_status)

REPO = "Hub-Tansky/RoboViolence2"
FIELD = {"id": "F", "options": [{"id": "o1", "name": "In progress"}, {"id": "o2", "name": "Testing"},
                                {"id": "o3", "name": "Done"}]}
DRAFT = {"__typename": "DraftIssue", "title": "Phase T7: Screenshot regression"}
PAGES = [  # the item sits on the second page
    [{"id": "I0", "content": None}, {"id": "I1", "content": {"__typename": "PullRequest"}},
     {"id": "I2", "content": {"__typename": "Issue", "number": 79, "repository": {"nameWithOwner": "other/repo"}}}],
    [{"id": "I3", "content": DRAFT},
     {"id": "I4", "content": {"__typename": "Issue", "number": 79, "repository": {"nameWithOwner": REPO}}}],
]
ITEM = '| Item | [Project](https://github.com/users/Hub-Tansky/projects/1) draft item "Phase T7: Screenshot regression" |'
# (case, action, merged, draft PR, description, expected (item, option) or None)
CASES = [
    ("draft PR", "opened", False, True, ITEM, ("I3", "o1")),
    ("ready PR", "ready_for_review", False, False, "| Item | Closes #79 |", ("I4", "o2")),
    ("merged", "closed", True, False, "| Item | Closes #79 |", ("I4", "o3")),
    ("closed unmerged", "closed", False, False, "| Item | Closes #79 |", None),
    ("no Item row", "opened", False, False, "Closes #79", None),
    ("closes elsewhere", "opened", False, False, ITEM + "\n<details>closes #79</details>", ("I3", "o2")),
    ("unknown title", "opened", False, False, '| Item | draft item "Missing" |', None),
]


def fake_graphql(query, **variables):
    if query is project_status.SET_STATUS:
        moves.append((variables["item"], variables["option"]))
        return {}
    page = 1 if variables["after"] else 0
    return {"user": {"projectV2": {"id": "P", "field": FIELD, "items": {
        "pageInfo": {"hasNextPage": page == 0, "endCursor": "c1" if page == 0 else None}, "nodes": PAGES[page]}}}}


project_status.graphql = fake_graphql
os.environ["GITHUB_EVENT_PATH"] = os.path.join(sys.argv[1], "project-status-event.json")
failures = 0
for case, action, merged, draft, body, expected in CASES:
    with open(os.environ["GITHUB_EVENT_PATH"], "w") as f:
        json.dump({"action": action, "repository": {"full_name": REPO},
                   "pull_request": {"merged": merged, "draft": draft, "body": body}}, f)
    moves = []
    project_status.main()
    if moves != ([expected] if expected else []):
        print(f"FAIL {case}: moved {moves}, expected {expected}")
        failures += 1
print("project_status:", "FAIL" if failures else "ok")
sys.exit(1 if failures else 0)
