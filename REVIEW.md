# REVIEW.md: when delivered code is up to standard

Agents check every change against this file before calling it done.

## Every change

- Stays inside the step's **Allowed paths** and scope ([AGENTS.md](AGENTS.md) Agent rules).
- Builds warning-clean on all three presets; `ctest` passes; `tools/check-architecture.sh` and `python3 tools/check-hygiene.py` pass.
- Follows the roadmap ground rules ([docs/roadmap/README.md](docs/roadmap/README.md#ground-rules-every-phase)): BV2 assets still load, no unapproved **[GUI]** change, renames only as separate mechanical commits.
- New behaviour has a test; fixes have a test that failed before.
- No secrets, real hosts or IPs; no `--no-verify`.
- `ARCHITECTURE.md` and any affected README or ADR are updated; non-obvious decisions have an ADR.
- Mechanical rewrites (encoding, renames, formatting) are not mixed with functional changes in one commit.

## Big changes: thermo-nuclear review

Run the `/thermo-nuclear-code-quality-review` skill when a chunk of work is done that is either:

- a whole step (`docs/roadmap/<phase>/stepN-*.md`), or
- a PR that changes the code's architecture: new or moved modules, libraries or interfaces, changed ownership or data flow, protocol or file-format changes.

Run it on the full diff against `main`, before marking the PR ready. If the skill isn't available, stop and tell the owner; don't substitute another review silently.

## Acting on findings

For each finding, either fix it or ask the owner. Never drop one silently.

| Implement directly | Confirm with the owner first |
|---|---|
| Inside the step's scope and Allowed paths | Outside the scope or Allowed paths |
| Fixes a real defect, or removes duplication or dead code without changing behaviour | Changes gameplay, protocol, file formats or anything the player sees (**[GUI]**) |
| Doesn't contradict an ADR or the plan | Reverses or bends an ADR or the plan, or needs a new ADR |
| Small and verifiable with existing checks and tests | Large rewrite, new dependency, or a matter of taste with no clear gain |

Implemented fixes go in their own commits (`stepN.M: review: <summary>`). List the open questions in one message to the owner, each with the finding, the proposed change and a recommendation. Record rejected findings and the reason in the PR description.
