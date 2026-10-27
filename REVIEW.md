# REVIEW.md: when delivered work is done

Work is done when `tools/review.sh` passes and, for a scoped PR, its review record is in the PR description. A claim without that evidence doesn't count.

**Stop rule:** if any item below can't be met, stop. Don't mark the PR ready or move its Project item to Done. Write what's missing under **Not done / unsure** in the review record and tell the owner.

## 1. Mechanical checks: `tools/review.sh`

Run `tools/review.sh` while working, and `tools/review.sh --full <preset>` before the PR is ready. CI runs the fast mode on every PR (required `review` check).

| Check | Fails when |
|---|---|
| architecture, hygiene, content-case, original-assets | the existing `tools/check-*` scripts fail |
| secrets | gitleaks finds a secret in `merge-base..HEAD` (skipped locally if gitleaks is missing; CI `secret-scan` runs it) |
| scope | a changed file is outside the PR's **Allowed paths** row (from the PR description: `REVIEW_PR_BODY` in CI, `gh pr view` locally, or `--scope <file>`) |
| anti-gaming | an added line switches a check off: `#pragma warning`/`diagnostic`, `NOLINT`, `-Wno-`, `DISABLED_`, `--no-verify`, `continue-on-error: true`. A justified line carries `review: allow <reason>` |
| review gate | a ready (non-draft) PR with an Allowed paths row has no `## Review record` naming a fresh-context reviewer, has a finding still `pending`, has a `deferred` finding without a Project item or issue link, or lacks the raw report (`<details>` with the skill's load line and no `SKILL: NOT LOADED`) |
| `--full` | configure, build (warnings are errors) or `ctest` fails |

In a PR without an Allowed paths row, changes to guard files (`tools/check-*`, `tools/review.sh`, `.githooks/`, `.github/`, `.gitleaks.toml`, `.gitignore`, this file) print a NOTE: the owner must approve them.

## 2. Judged by the reviewer

- The ground rules hold ([AGENTS.md](AGENTS.md#ground-rules)): BV2 assets still load, no unapproved **[GUI]** change, renames only in separate mechanical commits.
- New behaviour has a test. A fix has a test with **fix proof**.
- `ARCHITECTURE.md`, every affected README and the ADRs are updated. Every non-obvious decision has an ADR.
- Mechanical rewrites (encoding, renames, formatting) are not mixed with functional changes in one commit.
- Every finding of the PR review is acted on (section 5).

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

## 5. PR close: fresh-context review

Required for every PR with an Allowed paths row, and for an architecture-changing PR (new or moved modules, libraries or interfaces, changed ownership or data flow, protocol or file-format changes).

1. `tools/review.sh --full <preset>` passes.
2. Spawn the **`rv2-thermo-nuclear-review` subagent** (Agent tool, `.claude/agents/`). Its prompt holds only the PR number (or the scope text), the merge-base and links to earlier review records it must not contradict. It loads the `rv2-thermo-nuclear-review` skill (`.claude/skills/`), starts its report with the tool's load line (or `SKILL: NOT LOADED – <reason>`), reviews `git diff <merge-base>..HEAD` against the skill and section 2, and doesn't re-raise findings earlier records rejected or deferred. The author doesn't pre-filter its findings. If the subagent or skill isn't available, stop and tell the owner.
   Paste the report unedited into the PR description inside `<details><summary>Raw reviewer report</summary> … </details>` (later rounds appended under `Round k`). It is evidence, not proof: the author could forge it, so the owner may spot-check it against the session.
3. Act on every finding:

   | Implement directly | Confirm with the owner first |
   |---|---|
   | Inside the PR's scope and Allowed paths | Outside the scope or Allowed paths |
   | Fixes a real defect, or removes duplication or dead code without changing behaviour | Changes gameplay, protocol, file formats or anything the player sees (**[GUI]**) |
   | Doesn't contradict an ADR or the Project item | Reverses or bends an ADR or the item, or needs a new ADR |
   | Small and verifiable with existing checks and tests | Large rewrite, new dependency, or a matter of taste with no clear gain |

   Fixes go in their own commits (`<kind>: review: <summary>`). A finding left for later becomes a Proposed draft item in the [Project](https://github.com/users/Hub-Tansky/projects/1) (or an issue) and the outcome `deferred → <item or issue link>`. Ask the owner all open questions in one message, each with the finding, the proposed change and a recommendation.
4. Write the `## Review record` section of the PR description from the template below.
5. Mark the PR ready only after the last review round covers every commit and `--full` passed on the final head. `tools/review.sh` checks the record and the report.

## Review record template

```markdown
## Review record

Reviewer: fresh-context subagent rv2-thermo-nuclear-review, YYYY-MM-DD
Diff: <merge-base>..<head>

### review.sh

<pasted summary of tools/review.sh --full <preset>>

### Findings

| # | Finding | Outcome |
|---|---|---|
| 1 | <finding> | fixed in <commit> / confirmed with owner: <decision> / deferred → <Project item or issue link> / rejected: <reason> |

### Fix proof

<per fix: test name and the failing assertion with the fix reverted>

### Not done / unsure

<anything unverified, or "none">

<details><summary>Raw reviewer report</summary>

<report, unedited>

</details>
```

Records of Phases A–C and T steps 1–3 stay at the [last commit with docs/roadmap](https://github.com/Hub-Tansky/RoboViolence2/tree/58c91b2a660db2b290b0b5f504381980a6aedba2/docs/roadmap).
