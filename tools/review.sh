#!/usr/bin/env bash
# Mechanical review gate (REVIEW.md section 1). Exit 0 only if every check passed.
# Usage: tools/review.sh [--base <ref>] [--scope <file>] [--full <preset>]
#   --base   compare against this ref (default origin/main); the diff is merge-base..working tree
#   --scope  file holding the PR description (default: $REVIEW_PR_BODY in CI, else `gh pr view` of this branch)
#   --full   also configure, build and ctest this preset (required before the PR is ready)
set -u
cd "$(dirname "$0")/.."

base=origin/main scope="" full=""
while [ $# -gt 0 ]; do
  case "$1" in
    --base) base=$2; shift 2 ;;
    --scope) scope=$2; shift 2 ;;
    --full) full=$2; shift 2 ;;
    *) sed -n '2,6p' "$0" >&2; exit 2 ;;
  esac
done

mb=$(git merge-base "$base" HEAD 2>/dev/null) || { echo "FAIL base: '$base' not found (git fetch origin main)"; exit 1; }
fail=0
check() { # check <name> <command...>: run it, print PASS/FAIL; indent its output on failure, its NOTEs always
  local name=$1 out; shift
  if out=$("$@" 2>&1); then
    echo "PASS $name"; printf '%s\n' "$out" | grep '^NOTE' | sed 's/^/     /'  # NOTEs need the owner even on a pass
  else
    echo "FAIL $name"; printf '%s\n' "$out" | sed 's/^/     /'; fail=1
  fi
}

check architecture tools/check-architecture.sh
check hygiene python3 tools/check-hygiene.py
check content-case python3 tools/check-content-case.py
check original-assets python3 tools/check-original-assets.py
if command -v gitleaks >/dev/null 2>&1; then
  check secrets gitleaks git --config .gitleaks.toml --redact --no-banner --log-opts="$mb..HEAD"
else
  echo "SKIP secrets: gitleaks not installed (CI secret-scan runs it)"
fi

# PR description: --scope file, CI's REVIEW_PR_BODY, or this branch's PR via gh. Empty outside a PR.
if [ -n "$scope" ]; then
  body=$(cat "$scope")
elif [ -n "${REVIEW_PR_BODY+x}" ]; then
  body=$REVIEW_PR_BODY
elif command -v gh >/dev/null 2>&1; then
  body=$(gh pr view --json body -q .body 2>/dev/null || true)
  [ -n "${REVIEW_PR_DRAFT:-}" ] || REVIEW_PR_DRAFT=$(gh pr view --json isDraft -q .isDraft 2>/dev/null || echo true)
else
  body=""
fi

# Scope, anti-gaming and review gate: one Python pass over the diff.
check diff-rules env REVIEW_BASE="$mb" REVIEW_BODY="$body" REVIEW_DRAFT="${REVIEW_PR_DRAFT:-true}" python3 - <<'EOF'
import os, re, subprocess, sys

base, body, draft = os.environ["REVIEW_BASE"], os.environ["REVIEW_BODY"].replace("\r\n", "\n"), os.environ["REVIEW_DRAFT"] == "true"
git = lambda *a: subprocess.run(["git", *a], capture_output=True, text=True, check=True).stdout
changed = [f for f in git("diff", "--name-only", "--diff-filter=ACMR", base).split() if f]
changed += [f for f in git("ls-files", "--others", "--exclude-standard").split() if f]
problems = []

def to_regex(g):
    r = re.escape(g).replace(r"\*\*/", "(?:.*/)?").replace(r"\*\*", ".*").replace(r"\*", "[^/]*").replace(r"\?", "[^/]")
    return re.compile(r + "$")

def expand(g):  # {a,b} alternatives
    mm = re.search(r"\{([^}]*)\}", g)
    return [x for alt in mm.group(1).split(",") for x in expand(g[:mm.start()] + alt + g[mm.end():])] if mm else [g]

GUARDS = re.compile(r"^(tools/check-|tools/review\.sh$|tools/original-assets|\.githooks/|\.github/|\.gitleaks\.toml$|\.gitignore$|REVIEW\.md$)")
row = next((l for l in body.splitlines() if re.match(r"\|\s*Allowed paths\s*\|", l)), None)
if row:
    globs = [to_regex(x) for t in re.findall(r"`([^`]+)`", row) for x in expand(t)]
    for f in changed:
        if not any(g.match(f) for g in globs):
            problems.append(f"scope: {f} is outside the PR's Allowed paths")
else:
    for f in changed:
        if GUARDS.match(f):
            print(f"NOTE {f}: guard file changed in a PR without Allowed paths; the owner must approve it")

# Anti-gaming: added lines that switch checks off. Mark a justified line with 'review: allow <reason>'.
BANNED = re.compile(r"#\s*pragma\s+(warning|clang\s+diagnostic|GCC\s+diagnostic)|NOLINT|-Wno-|\bDISABLED_|--no-verify|continue-on-error:\s*true")
file = None
for line in git("diff", "-U0", "--src-prefix=a/", "--dst-prefix=b/", base).splitlines():
    if line.startswith("+++ "):
        file = line[6:] if line.startswith("+++ b/") else None
    elif file and line.startswith("+") and not file.endswith(".md") and file != "tools/review.sh":
        if BANNED.search(line) and "review: allow" not in line:
            problems.append(f"anti-gaming: {file}: {line[1:].strip()[:120]}")

# Review gate: a ready PR with Allowed paths needs its fresh-context review record in the description.
if row and not draft:
    rec = body.split("## Review record", 1)[1] if "## Review record" in body else ""
    if not rec:
        problems.append("review gate: the PR is ready but its description has no '## Review record'")
    elif "Reviewer: fresh-context subagent" not in rec or "### Findings" not in rec:
        problems.append("review gate: the record lacks 'Reviewer: fresh-context subagent' or '### Findings'")
    elif re.search(r"\|\s*pending\s*\|", rec):
        problems.append("review gate: the record still has pending findings")
    elif "SKILL: Launching skill: rv2-thermo-nuclear-review" not in rec or "SKILL: NOT LOADED" in rec:
        problems.append("review gate: the raw reviewer report lacks the skill's load line, or says NOT LOADED")
    else:
        for r in re.findall(r"^\|.*\bdeferred\b.*\|\s*$", rec, re.M):
            if not re.search(r"#\d+|github\.com/(users/Hub-Tansky/projects/1|Hub-Tansky/RoboViolence2/issues/)", r):
                problems.append(f"review gate: deferred finding without a Project item or issue link: {r.strip()[:100]}")

print(f"scope: {'PR Allowed paths' if row else 'none (no Allowed paths row)'}; {len(changed)} changed files")
for p in problems:
    print(p)
sys.exit(1 if problems else 0)
EOF

if [ -n "$full" ]; then
  check configure cmake --preset "$full"
  check build cmake --build --preset "$full"
  check ctest ctest --test-dir "build/$full" --output-on-failure
fi

[ "$fail" = 0 ] && echo "review.sh: all checks passed" || echo "review.sh: FAILED"
exit "$fail"
