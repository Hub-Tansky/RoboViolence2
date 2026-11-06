# 0045: clang-format and clang-tidy run on commit through pre-commit, on changed lines only

- Status: Accepted
- Date: 2026-11-05
- Affects: `.pre-commit-config.yaml`, `.githooks/pre-commit`, `tools/setup-dev.sh`, `tools/setup-dev.ps1`, [AGENTS.md](../../AGENTS.md), `docs/build/`

## Context
- The owner wants [cpp-linter-hooks](https://github.com/cpp-linter/cpp-linter-hooks) clang-format and clang-tidy on every commit.
- 323 of 334 non-third-party C/C++ files don't match `.clang-format` (2026-11-05, clang-format 18). AGENTS.md says format only touched lines and never mass-reformat; `.git-blame-ignore-revs` exists to keep blame clean.
- `tools/setup-dev.sh` sets `core.hooksPath .githooks`; `pre-commit install` refuses to install while it is set.
- `.clang-tidy` is the CI `tidy` job's config and needs a `compile_commands.json` (every Ninja preset writes one, ADR 0001).

## Decision
- `.githooks/pre-commit` runs `pre-commit run` after its own checks and fails when `pre-commit` is missing. Nobody runs `pre-commit install`.
- Formatting on commit is a local hook running `git-clang-format HEAD` from the `clang-format` wheel: only lines the commit changes are formatted. If it changes anything the commit stops and the result is reviewed and staged.
- cpp-linter-hooks `clang-format` (whole file) stays as a `manual` stage at the owner's request; new files are already formatted in full by the changed-lines hook, and it is never run on legacy files.
- cpp-linter-hooks `clang-tidy` runs on `engine/*/src`, `game/src` and `masterserver/src` sources (the CI `tidy` set) and reports only changed lines. It uses the newest `build/*/compile_commands.json` and skips with a note when no preset is configured.
- `third_party/` is excluded. clang-format 23.1.5 and clang-tidy 23.1.4 (the newest clang-tidy wheel on PyPI) are pinned so every machine formats and lints alike.

## Consequences
- A commit touching a legacy file gets a small, reviewable format diff, never a whole-file rewrite.
- A fresh clone without a configured preset commits without clang-tidy; CI `tidy` still runs, but with Ubuntu's apt clang-tidy 18, so its findings can differ from the commit hook's.
- clang-tidy analyses the whole translation unit, so a commit touching large game files can take tens of seconds.
- Headers aren't clang-tidy'd on commit (no compile command of their own); their problems show up through the sources in CI.
- Rejected: the cpp-linter-hooks `clang-format` hook on commit as suggested in its README (whole-file `-i`: mass reformat of every touched file). Rejected: a one-time mass reformat plus a blame-ignore entry (hides the BV2 history the port depends on, collides with every open branch). Rejected: unsetting `core.hooksPath` to let `pre-commit install` own the hooks (moves the identity, secret and asset checks into a second system).
