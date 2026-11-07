# Robo Violence 2 code review

The language of reviewing a pull request. Backlog terms such as scope item are in `CONTEXT.md` in rv2-docs.

## Language

**Axis**:
One independent question a review answers: Standards, Spec or Risk. Each has its own isolated worker and report.
_Avoid_: Reviewer, rubric, pass

**Standards**:
The axis that asks whether the diff keeps the repository's documented rules and introduces no direct defect.
_Avoid_: Code quality, thermo-nuclear

**Spec**:
The axis that asks whether the diff does what its spec source asked for, and nothing else.
_Avoid_: Scope check

**Risk**:
The axis that asks whether the diff is safe for security, wire protocol and concurrency.
_Avoid_: Security review

**Spec source**:
The scope item's acceptance criteria, then its linked issue, then the PR goal, at a recorded revision.
_Avoid_: Spec file, ticket

**Not assessable**:
The Spec result when the spec source can't be retrieved or has no acceptance criteria.
_Avoid_: Skipped, no spec

**Blocker**:
A finding that stops a PR being ready until it has an accepted outcome.
_Avoid_: Critical, must-fix

**Suggestion**:
A finding that improves the code but doesn't stop a PR being ready.
_Avoid_: Nit, non-blocker

**Review tier**:
The set of axes a PR needs, worked out from its changed paths: docs or code, with Risk added for listed paths.
_Avoid_: Trivial change

**Review record**:
The PR description section that holds a review's diff, spec source, raw reports and outcomes.
_Avoid_: Review report
