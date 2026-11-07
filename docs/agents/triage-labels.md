# Triage labels

The five roles use their default names as labels on GitHub issues, the public intake. If a label is missing, create it with `gh label create <name>` before applying it. Scope items carry no labels; see [issue-tracker.md](issue-tracker.md).

| Label | Meaning | Effect in rv2-docs |
|---|---|---|
| `needs-triage` | Owner needs to evaluate the issue | none yet |
| `needs-info` | Waiting on the reporter | none yet |
| `ready-for-agent` | Fully specified, an agent can do it | Proposed scope item with `issue:` set to the issue URL; the owner moves it to Next |
| `ready-for-human` | Needs the owner (e.g. `gui: true`, hosts, accounts) | Proposed scope item with `issue:` set to the issue URL |
| `wontfix` | Will not be done | close the issue as not planned; no scope item |
