# REVIEW.md: when delivered work is done

Work is done when `tools/review.sh` passes and a ready PR has a review record for its head in the description. A claim without that evidence doesn't count. Terms are in [CONTEXT.md](CONTEXT.md).

**Stop rule:** if any item below can't be met, stop. Don't mark the PR ready or move its scope item to Done. Write what's missing under **Not done / unsure** in the review record and tell the owner.

## 1. Mechanical checks: `tools/review.sh`

Run `tools/review.sh` while working and before the PR is ready. CI runs it on every PR (required `review` check). Scope, anti-gaming, secrets and the tier read committed changes (`<merge-base>...HEAD`); the whole-tree checks read the working tree, so a dirty tree prints a NOTE. Green required CI on the PR head counts as `--full <preset>`, which stays available locally.

| Check | Fails when |
|---|---|
| architecture, hygiene, content-case, original-assets | the existing `tools/check-*` scripts fail |
| secrets | gitleaks finds a secret in `merge-base..HEAD` (skipped locally if gitleaks is missing; CI `secret-scan` runs it) |
| scope | a changed file is outside the PR's **Allowed paths** row (from the PR description: `REVIEW_PR_BODY` in CI, `gh pr view` locally, or `--scope <file>`) |
| anti-gaming | an added line switches a check off: `#pragma warning`/`diagnostic`, `NOLINT`, `-Wno-`, `DISABLED_`, `--no-verify`, `continue-on-error: true`. A justified line carries `review: allow <reason>` |
| protocol | the `Protocol:` line in `.claude/skills/rv2-code-review/SKILL.md` doesn't match the file's hash (the message prints the right line) |
| review record | a ready non-bot PR has no Allowed paths row, or its record breaks section 5 (`tools/check-review-record.py`) |
| `--full` | configure, build (warnings are errors) or `ctest` fails |

In a draft or bot PR without an Allowed paths row, changes to guard files (`tools/check-*`, `tools/review.sh`, `.githooks/`, `.github/`, `.gitleaks.toml`, `.gitignore`, this file) print a NOTE: the owner must approve them.

## 2. Judged by the Standards axis

**Blockers:**

- The ground rules hold ([AGENTS.md](AGENTS.md#ground-rules)): BV2 assets still load, no unapproved **[GUI]** change, renames only in separate mechanical commits.
- New behaviour has a test. A fix has a test with **fix proof**. A missing test in tooling (`tools/**`, `.githooks/**`, `.github/**`, `.claude/**`) is a suggestion instead, unless the scope item asks for it.
- `ARCHITECTURE.md`, every affected README and the ADRs are updated. Every non-obvious decision has an ADR.
- Mechanical rewrites (encoding, renames, formatting) are not mixed with functional changes in one commit.
- The diff introduces no direct defect: a regression, or a bug in new code, shown by a traced path.

**Suggestions:**

- A file grows past 1,000 lines when it could be split.
- New logic duplicates logic that already exists.

## 3. Defined terms

| Term | Means |
|---|---|
| New behaviour | Anything a player, operator, script or other module can trigger that didn't exist before: code paths, config variables, console commands, packets, CI jobs, scripts |
| Fix proof | Revert the fix locally, run the new test, paste its failing assertion into the review record, restore the fix |
| Non-obvious decision | One with a rejected alternative, one a later agent might reverse, or one that changes a format, protocol, dependency or ground rule |
| Affected README | The README of any module (`engine/*`, `game`, `masterserver`, `docs/build`, …) whose files, public API, build or run steps the diff changes |

## 4. Forbidden ways to get green

- Suppressing warnings or lint findings, or lowering warning levels.
- Disabling, skipping or loosening a test, or changing expected values to match broken output.
- Editing `tools/check-*`, `tools/review.sh`, hooks or CI to make your own change pass.
- `--no-verify`, `continue-on-error`, `|| true` around a check.
- Widening a PR's **Allowed paths** without the owner's approval written next to the path.
- Marking a PR ready, moving its item to Done, or writing a review record, before the review has run.
- Changing a worker's severity or editing its report.

## 5. PR close: isolated review

Every ready PR needs an Allowed paths row and this review; only bot-authored PRs are exempt.

The tier comes from the changed paths, and `tools/review.sh` prints it:

| Tier | When | Axes |
|---|---|---|
| docs | every path is `docs/**` or `*.md`, none of them `REVIEW.md`, `AGENTS.md`, `.claude/**`, `.github/**` or `docs/agents/**` | Standards |
| code | anything else | Standards, Spec |
| plus Risk | a path is `game/src/netPacket.h`, `game/src/ServerRecv.cpp`, `game/src/ClientRecv.cpp`, `engine/babonet/**` or `engine/credentials/**`, or the owner asks | those, plus Risk |

1. `tools/review.sh` passes on a clean tree.
2. Run the `rv2-code-review` workflow (`.claude/skills/rv2-code-review/SKILL.md`, the only copy). It pins the diff, resolves the spec source and runs one fresh, isolated worker per axis in parallel. No worker sees another's report or the author's context. In a tool without that skill, follow the file by hand. If workers can't run, stop and tell the owner.
3. Spec needs a spec source: the scope item's acceptance criteria, then its linked issue, then the PR goal, read from the local vault first. If it can't be retrieved or has no acceptance criteria, Spec is `not assessable` and the PR can't be ready.
4. Paste each report unedited. Aggregate without merging or re-ranking. The reports are evidence, not proof: the owner may spot-check them against the session.
5. Give every finding an outcome:

   | Severity | Accepted outcomes |
   |---|---|
   | blocker | `fixed in <commit>`; `confirmed with owner: <decision>`; `deferred → <ROBO-NNN or issue link>`; `rejected: <reason citing path:line or a URL>` |
   | suggestion | any of those, or `not taken` (no backlog item needed) |

   Confirm with the owner first when a fix is outside the scope or Allowed paths, changes gameplay, protocol, file formats or anything the player sees (**[GUI]**), bends an ADR or the scope item, or is a large rewrite or new dependency. Otherwise fix directly, in its own commit (`<kind>: review: <summary>`). A deferred finding becomes a Proposed scope item in [rv2-docs](https://github.com/Hub-Tansky/rv2-docs) `40-backlog/20-proposed/` or an issue. Ask the owner all open questions in one message, each with the finding, the proposed change and a recommendation.
6. After fixes, rerun every required axis on the full diff at the new head. Stop when a round finds no new blocker.
7. Write the record. In the final round, Spec `fail` and `not assessable` block; `partial` blocks unless Outcomes says `confirmed with owner`, with the criterion's text in the Finding cell.
8. Mark the PR ready only with required CI green on the head named in `Diff:`. Any later commit makes the record stale. A PR that changes the skill needs a review on the changed skill.

## Review record template

Keep only the final round's raw reports, one `<details>` block per axis, its `<details>` and `</details>` lines at the start of the line. Findings from every round stay in Outcomes.

```markdown
## Review record

Diff: <merge-base>...<head>
Spec source: <ROBO-NNN> rv2-docs@<sha> blob:<12 hex> [(uncommitted)]

### review.sh

<pasted summary of tools/review.sh, and the CI run for the head>

### Standards

<details><summary>Raw report</summary>

<report, unedited>

</details>

### Spec

<details><summary>Raw report</summary>

<report with its Criterion | Diff/test evidence | Status matrix, unedited; omitted in the docs tier>

</details>

### Risk

<when required; same form>

### Outcomes

| # | Axis | Finding | Severity | Outcome |
|---|---|---|---|---|
| 1 | Standards | <finding> | blocker | fixed in <commit> |

### Fix proof

<per fix: test name and the failing assertion with the fix reverted>

### Not done / unsure

<anything unverified, or "none">
```

Records of Phases A–C and T steps 1–3 stay at the [last commit with docs/roadmap](https://github.com/Hub-Tansky/RoboViolence2/tree/58c91b2a660db2b290b0b5f504381980a6aedba2/docs/roadmap).
