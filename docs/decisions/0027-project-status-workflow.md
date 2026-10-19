# 0027: A workflow moves the PR's Project item

- Status: Accepted
- Date: 2026-10-19
- Affects: `.github/workflows/project-status.yml`, `tools/project-status.py`, `.github/pull_request_template.md`, [ADR 0024](0024-github-project-is-the-backlog.md)

## Context
- Built-in Project workflows only move items that are the PR or a closed issue. Most work is draft items, which the PR links by title, so they were moved by hand.
- ADR 0024 rejected an Actions workflow because it needs a stored PAT. The owner approved one on 2026-10-19 in the project thread.

## Decision
- `project-status.yml` runs on `pull_request_target` events of same-repo PRs: draft PR -> In progress, ready PR -> Testing, merged -> Done.
- The item comes from the description's Item row: `Closes #n`, or `draft item "<exact title>"`.
- `pull_request_target` runs the script from the base branch and never checks out the PR, so a PR branch can't change the code that holds the token. The description is only matched by regexes.
- Token: secret `PROJECT_TOKEN`, a classic PAT of `Hub-Tansky` with only the `project` scope. Without it the job skips. Fork PRs are skipped.

## Consequences
- Supersedes ADR 0024's rejection of a status-sync workflow.
- A renamed draft item no longer matches its PR; the job prints "no Project item" and changes nothing.
- The PAT can write every Project of the account, not only this one; a classic PAT is needed because fine-grained PATs can't reach user-owned Projects.
- The PAT expires; renew it in repository secrets.
- The workflow runs only once it is on `main`; the PR that adds it is not moved.
