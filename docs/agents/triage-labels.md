# Triage labels

Labels apply to GitHub issues, the public intake. Scope items in rv2-docs carry no labels; their folder is their status (see [issue-tracker.md](issue-tracker.md)).

| Role in mattpocock/skills | Label on GitHub issues | Meaning | Effect in rv2-docs |
|---|---|---|---|
| `needs-triage` | `needs-triage` | Owner needs to evaluate the issue | none yet |
| `needs-info` | `needs-info` | Waiting on the reporter | none yet |
| `ready-for-agent` | `ready-for-agent` | Fully specified, an agent can do it | create a Proposed scope item with `issue: #n`; the owner moves it to Next |
| `ready-for-human` | `ready-for-human` | Needs the owner (e.g. `gui: true`, hosts, accounts) | create a Proposed scope item with `issue: #n` |
| `wontfix` | `wontfix` | Will not be done | close the issue as not planned; no scope item |
