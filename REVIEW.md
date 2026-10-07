# REVIEW.md: when delivered work is done

Work is done when `tools/review.sh` passes and, for a step, its review record exists. A claim without that evidence doesn't count.

**Stop rule:** if any item below can't be met, stop. Don't set the step to DONE or mark the PR ready. Write what's missing under **Not done / unsure** in the review record and tell the owner.

## 1. Mechanical checks: `tools/review.sh`

Run `tools/review.sh` while working, and `tools/review.sh --full <preset>` before a step is DONE. CI runs the fast mode on every PR (required `review` check).

| Check | Fails when |
|---|---|
| architecture, hygiene, content-case, original-assets | the existing `tools/check-*` scripts fail |
| secrets | gitleaks finds a secret in `merge-base..HEAD` (skipped locally if gitleaks is missing; CI `secret-scan` runs it) |
| scope | a changed file is outside the step's **Allowed paths** (step from the `refactor/<phase>-stepN-*` branch, or `--step`) |
| anti-gaming | an added line switches a check off: `#pragma warning`/`diagnostic`, `NOLINT`, `-Wno-`, `DISABLED_`, `--no-verify`, `continue-on-error: true`. A justified line carries `review: allow <reason>` |
| DONE gate | a step file says DONE without `reviews/stepN.md` naming a fresh-context reviewer, with a finding still `pending`, or with a `deferred` finding that doesn't name an existing `PNS-n`; or without `reviews/stepN-report.md` containing the skill's load line and no `SKILL: NOT LOADED` |
| `--full` | configure, build (warnings are errors) or `ctest` fails |

Outside a step branch, changes to guard files (`tools/check-*`, `tools/review.sh`, `.githooks/`, `.github/`, `.gitleaks.toml`, `.gitignore`, this file) print a NOTE: the owner must approve them.

## 2. Judged by the reviewer

- The roadmap ground rules hold ([docs/roadmap/README.md](docs/roadmap/README.md#ground-rules-every-phase)): BV2 assets still load, no unapproved **[GUI]** change, renames only in separate mechanical commits.
- New behaviour has a test. A fix has a test with **fix proof**.
- `ARCHITECTURE.md`, every affected README and the ADRs are updated. Every non-obvious decision has an ADR.
- Mechanical rewrites (encoding, renames, formatting) are not mixed with functional changes in one commit.
- Every finding of the step review is acted on (section 5).

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
- Widening a step's **Allowed paths** without the owner's approval written next to the path.
- Marking a step DONE, or writing a review record, before the review has run.

## 5. Step close: fresh-context review

Also required for an architecture-changing PR outside a step (new or moved modules, libraries or interfaces, changed ownership or data flow, protocol or file-format changes); its record goes in the PR description.

1. `tools/review.sh --full <preset>` passes.
2. Spawn a **fresh subagent** (Agent tool). Its prompt holds only the step file path, this file, the merge-base and the paths of the phase's earlier records (`docs/roadmap/<phase>/reviews/step*.md`). It first calls the Skill tool with `thermo-nuclear-code-quality-review` and starts its report with `SKILL: <the tool's result, verbatim>`, or `SKILL: NOT LOADED – <reason>`. It then reads `git diff <merge-base>..HEAD`, applies the skill and checks section 2. From the earlier records it doesn't re-raise findings already rejected or deferred, and it flags a finding that repeats an earlier one. The author doesn't pre-filter its findings. If the skill isn't available, stop and tell the owner.
   Save the report unedited as `docs/roadmap/<phase>/reviews/stepN-report.md` (later rounds are appended under `## Round k`). It is evidence, not proof: the author could forge it, so the owner may spot-check it against the session.
3. Act on every finding:

   | Implement directly | Confirm with the owner first |
   |---|---|
   | Inside the step's scope and Allowed paths | Outside the scope or Allowed paths |
   | Fixes a real defect, or removes duplication or dead code without changing behaviour | Changes gameplay, protocol, file formats or anything the player sees (**[GUI]**) |
   | Doesn't contradict an ADR or the plan | Reverses or bends an ADR or the plan, or needs a new ADR |
   | Small and verifiable with existing checks and tests | Large rewrite, new dependency, or a matter of taste with no clear gain |

   Fixes go in their own commits (`stepN.M: review: <summary>`). A finding left for later gets a `possible-new-scope.md` entry and the outcome `deferred → PNS-n`. Ask the owner all open questions in one message, each with the finding, the proposed change and a recommendation.
4. Write `docs/roadmap/<phase>/reviews/stepN.md` from the template below.
5. Set the step's Status to DONE only after the last review round covers every commit of the step and `--full` passed on the final head. `tools/review.sh` checks the record and the report.

## Review record template

```markdown
# Step N review

Reviewer: fresh-context subagent, /thermo-nuclear-code-quality-review, YYYY-MM-DD
Diff: <merge-base>..<head>

## review.sh

<pasted summary of tools/review.sh --full <preset>>

## Findings

| # | Finding | Outcome |
|---|---|---|
| 1 | <finding> | fixed in <commit> / confirmed with owner: <decision> / deferred → PNS-n / rejected: <reason> |

## Fix proof

<per fix: test name and the failing assertion with the fix reverted>

## Not done / unsure

<anything unverified, or "none">
```
