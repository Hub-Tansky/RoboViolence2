---
name: rv2-code-review
description: Robo Violence 2's PR review workflow (REVIEW.md section 5). Pins the diff, resolves the scope item, runs isolated Standards and Spec workers (plus Risk when required) and aggregates their reports without re-ranking. Use before marking a PR ready, or when asked to review a branch or PR of this repo.
---

# rv2-code-review

Protocol: rv2-code-review 4f8cf28349d9

Adapted from `skills/engineering/code-review` in [mattpocock/skills](https://github.com/mattpocock/skills) at `4ae2317ddb22b046e26455e4f9c9b3aaaf62212d` (MIT; notice in `UPSTREAM-LICENSE`). Changes: a missing spec blocks instead of skipping the axis, no smell baseline, a matrix for Spec, a Risk worker, and the record in REVIEW.md. Upgrade only by a reviewed PR that updates this SHA.

## Orchestrate

You are the parent. You pin, launch and aggregate. You don't review, filter or re-rank.

1. **Pin.** `mb=$(git merge-base origin/main HEAD)`, `head=$(git rev-parse HEAD)`. Stop if `git diff $mb...$head` is empty or the tree is dirty.
2. **Run** `tools/review.sh`. Its NOTE gives the tier and whether Risk is required. Stop on a failure.
3. **Resolve the spec source.** Take `ROBO-NNN` from the PR's Item row. Read `40-backlog/*/ROBO-NNN *.md` in the local vault (`~/Dev/RoboViolence2/Robo2-docs`) first, since it can be ahead of GitHub; without a local vault, use `gh api repos/Hub-Tansky/rv2-docs/contents/<path>`. Never commit or move changes you didn't make.
   - `Spec source: ROBO-NNN rv2-docs@<vault HEAD> blob:<first 12 hex of git hash-object FILE>`
   - Append ` (uncommitted)` when `git status --porcelain -- FILE` in the vault lists the item.
   - Also fetch the item's linked issue, if any. Precedence: acceptance criteria, then linked issue, then PR goal.
4. **Launch the workers in parallel**, each with a fresh context and none of this conversation:

   | Tier | Workers |
   |---|---|
   | docs | Standards |
   | code | Standards, Spec |
   | Risk required, or the owner asked | those, plus Risk |

   Each prompt is: "Read `.claude/skills/rv2-code-review/SKILL.md` and follow only its section `<Worker> worker`", then that section's inputs. Add earlier outcomes of the same axis in a later round. Never give a worker another worker's report or your own opinion.
5. **Aggregate.** Paste each report unedited into its section of the record (REVIEW.md template). Write a one-line summary per axis, naming the axis's worst finding. Don't pick a winner across axes, and don't change a severity.
6. **Act.** Follow REVIEW.md section 5. After fixes, rerun every required worker on the full diff at the new head. Stop when a round finds no new blocker.

## Rules for every worker

- You are read-only. Review `git diff <merge-base>...<head>`, and read each changed file in full. Don't spawn workers.
- Don't re-raise a finding the earlier outcomes for your axis rejected or deferred.
- Skip what `tools/review.sh` and CI enforce.
- Cite each finding as `path:line`, source rule or criterion, evidence, and the smallest in-scope remedy.
- Don't demand perfection: a change that improves the code's health may pass with suggestions left over.
- Stay under 500 words, not counting the Spec matrix. Don't write a `<details>` line.

Report format, starting with the `Protocol:` line of this file, verbatim:

```markdown
Protocol: rv2-code-review <hash>

Scope: <diff range and files read, one sentence>

<Spec worker only: the matrix>

## Blockers

1. `path:line`: <source>. <evidence>. Remedy: <smallest in-scope fix>.

## Suggestions

1. `path:line`: <source>. <evidence>. Remedy: <smallest in-scope fix>.
```

Number each finding as one list item. Write `none` under a heading with no findings.

## Standards worker

Inputs: diff range, `tools/review.sh` output.

Read `AGENTS.md`, `REVIEW.md`, and the ADRs and module READMEs for the changed areas. Report only:

- **Blockers:** a breach of a documented rule (cite file and rule), including REVIEW.md section 2; a direct defect the diff introduces, meaning a regression or a bug in new code, shown by a traced path.
- **Suggestions:** REVIEW.md's maintainability rules, and improvements a documented rule supports.

Don't judge whether the diff meets its goal or scope item. In the docs tier, check only the documented rules for documentation and whether the text is correct against the code.

## Spec worker

Inputs: diff range, PR goal, Allowed paths, the scope item's text or path, the linked issue if any.

Check traceability and scope creep only. Don't report code style or standards. Give one row per acceptance criterion, its text verbatim:

| Criterion | Diff/test evidence | Status |
|---|---|---|
| <criterion> | `path:line`, test | pass / partial / fail |

- **Blockers:** a criterion that fails; behaviour in the diff the spec didn't ask for, quoting the nearest spec line.
- **Suggestions:** a spec gap that the diff doesn't make worse.

If you can't retrieve the scope item, or it has no acceptance criteria, write `not assessable` and why instead of the matrix, and stop.

## Risk worker

Inputs: diff range, the PR's Item row and goal.

Check only security, wire protocol and concurrency: untrusted wire data past the checks named in `AGENTS.md`, struct layout and `GAME_VERSION_*` changes, credentials, shared state across threads. Blockers are exploitable or demonstrably incorrect paths, traced from their input. Everything else is a suggestion.
