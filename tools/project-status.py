#!/usr/bin/env python3
"""Moves the PR's Project item (ADR 0027): draft PR -> In progress, ready PR -> Testing, merged -> Done.

Finds the item from the Item row of the PR description: `Closes #n` names an issue, `draft item "<title>"` a draft
item by its exact title. Reads the pull_request event from GITHUB_EVENT_PATH and the token from PROJECT_TOKEN.
"""
import json
import os
import re
import sys
import urllib.request

OWNER, NUMBER = "Hub-Tansky", 1


def graphql(query, **variables):
    request = urllib.request.Request(
        "https://api.github.com/graphql",
        data=json.dumps({"query": query, "variables": variables}).encode(),
        headers={"Authorization": "Bearer " + os.environ["PROJECT_TOKEN"], "Content-Type": "application/json"})
    reply = json.load(urllib.request.urlopen(request, timeout=30))
    if "errors" in reply:
        sys.exit(f"GraphQL error: {reply['errors']}")
    return reply["data"]


PROJECT = """query($owner: String!, $number: Int!, $after: String) {
  user(login: $owner) { projectV2(number: $number) {
    id
    field(name: "Status") { ... on ProjectV2SingleSelectField { id options { id name } } }
    items(first: 100, after: $after) {
      pageInfo { hasNextPage endCursor }
      nodes { id content {
        __typename
        ... on DraftIssue { title }
        ... on Issue { number repository { nameWithOwner } } } } } } } }"""

SET_STATUS = """mutation($project: ID!, $item: ID!, $field: ID!, $option: String!) {
  updateProjectV2ItemFieldValue(input: {projectId: $project, itemId: $item, fieldId: $field,
                                        value: {singleSelectOptionId: $option}}) { projectV2Item { id } } }"""


def target(body, repo):
    """(label, key) of the item named in the description's Item row, or None. Keys match item_key()."""
    row = re.search(r"^\| Item \|(.*)$", body, re.MULTILINE)
    issue = row and re.search(r"\bCloses #(\d+)", row.group(1), re.IGNORECASE)
    draft = row and re.search(r'draft item "([^"]+)"', row.group(1))
    if issue:
        return issue.group(0), ("Issue", int(issue.group(1)), repo)
    if draft:
        return draft.group(1), ("DraftIssue", draft.group(1))
    return None


def item_key(content):
    if content["__typename"] == "Issue":
        return "Issue", content["number"], content["repository"]["nameWithOwner"]
    return content["__typename"], content.get("title")


def main():
    with open(os.environ["GITHUB_EVENT_PATH"]) as f:
        event = json.load(f)
    pr = event["pull_request"]
    if event["action"] == "closed":
        if not pr["merged"]:
            return 0
        status = "Done"
    else:
        status = "In progress" if pr["draft"] else "Testing"
    found = target(pr["body"] or "", event["repository"]["full_name"])
    if not found:
        print('no `Closes #n` or `draft item "<title>"` in the Item row; nothing to move')
        return 0
    label, key = found
    after = None
    while True:
        project = graphql(PROJECT, owner=OWNER, number=NUMBER, after=after)["user"]["projectV2"]
        item = next((n for n in project["items"]["nodes"] if n["content"] and item_key(n["content"]) == key), None)
        page = project["items"]["pageInfo"]
        if item or not page["hasNextPage"]:
            break
        after = page["endCursor"]
    if not item:
        print(f"no Project item for {label!r}; nothing to move")
        return 0
    field = project["field"]
    option = next(o["id"] for o in field["options"] if o["name"] == status)
    graphql(SET_STATUS, project=project["id"], item=item["id"], field=field["id"], option=option)
    print(f"moved {label!r} to {status}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
