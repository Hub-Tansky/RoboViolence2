# ROBO-256: A Claude Code hook refuses forbidden commands and a commit hook refuses Claude trailers

- Status: Accepted
- Date: 2026-11-19
- Affects: `.claude/settings.json`, `.claude/hooks/guard.py`, `.githooks/commit-msg`, `tools/cloud-setup.sh`, `AGENTS.md`, `docs/build/linux.md`; ROBO-256 (workflow audit WP-16, approved 2026-11-11)

## Context
- AGENTS.md forbids `--no-verify` and `gh auth switch`; releases are tagged by the owner (rv2-docs release.md); rulesets and secrets are the owner's; the owner forbids `Co-Authored-By: Claude` and `Claude-Session:` lines in commits. Only text enforced these.
- Tested 2026-11-19 with `claude -p` in a cloud container: hooks in a repo's `.claude/settings.json` load when the session starts in the repo (Mac sessions), not when it starts in `/home/user` (cloud sessions with several repositories).

## Decision
- `.claude/settings.json` runs `.claude/hooks/guard.py` before Bash, Edit and Write. Bash: deny, with the rule in the reason, `--no-verify` (and its prefixes), `git commit -n` and `-c core.hooksPath`; reading `.env*` (not `.env.example`), `~/.ssh` or `/etc/rv2-prod.env`; a force push (`-f`, `--force`, `--force-with-lease`, `+refspec`) to `main`; `git tag v*`; a push of `v*` tags or `--tags`; `gh auth switch`; `gh secret`; `gh api` writes to `rulesets`. Edit and Write: ask before a guard file (the `GUARDS` pattern of `tools/merge-risk.py`, imported, not copied) and before `.claude/hooks/` or `.claude/settings.json` (asked here only, so merge tiers don't change). `permissions.deny`: Read of `**/.env*`, `~/.ssh/**`, `/etc/rv2-prod.env`.
- `tools/cloud-setup.sh` merges the same hook (absolute path) and deny rules into `~/.claude/settings.json`, so cloud sessions get them.
- `.githooks/commit-msg` rejects a commit message with a `Co-Authored-By: Claude` or `Claude-Session:` line.

## Consequences
- An agent that misses a rule gets refused with the rule named; a command split by `;`, `&&`, `||` or `|` is checked part by part. A command that only mentions a forbidden flag in quoted text is refused too.
- The hook fails closed: bad input or an error in it denies the call. It runs `python3`; on Windows that needs a real Python on PATH (`docs/build/windows.md`), or the hook doesn't run.
- Edits through Bash (`sed -i`, `>`, `git checkout -- <file>`) to guard files are not asked about; only the Edit and Write tools are.
- The check is text matching: a determined bypass (a script file, an alias) is not stopped; the rulesets and branch protection remain the real barrier.
- The GSD hooks were removed (owner action O4), so nothing overlaps.
- Rejected: `permissions.deny` patterns for the Bash rules (prefix matching misses `git -c x=y push -f`); a hook in each session's local settings only (lost in every fresh cloud container).
