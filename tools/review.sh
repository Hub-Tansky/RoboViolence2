#!/usr/bin/env bash
# Mechanical review gate (REVIEW.md section 1). Exit 0 only if every check passed.
# Usage: tools/review.sh [--base <ref>] [--step <step file>] [--full <preset>]
#   --base  compare against this ref (default origin/main); the diff is merge-base..working tree
#   --step  step file to check scope against (default: from a refactor/<phase>-stepN-* branch name)
#   --full  also configure, build and ctest this preset (required before a step is DONE)
set -u
cd "$(dirname "$0")/.."

base=origin/main step="" full=""
while [ $# -gt 0 ]; do
  case "$1" in
    --base) base=$2; shift 2 ;;
    --step) step=$2; shift 2 ;;
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

# Scope, anti-gaming and DONE gate: one Python pass over the diff.
check diff-rules env REVIEW_BASE="$mb" REVIEW_STEP="$step" REVIEW_BRANCH="${GITHUB_HEAD_REF:-${GITHUB_REF_NAME:-$(git rev-parse --abbrev-ref HEAD)}}" python3 - <<'EOF'
import glob, os, re, subprocess, sys

base, step, branch = os.environ["REVIEW_BASE"], os.environ["REVIEW_STEP"], os.environ["REVIEW_BRANCH"]
git = lambda *a: subprocess.run(["git", *a], capture_output=True, text=True, check=True).stdout
changed = [f for f in git("diff", "--name-only", "--diff-filter=ACMR", base).split() if f]
changed += [f for f in git("ls-files", "--others", "--exclude-standard").split() if f]
problems = []

# Step file: given, or derived from refactor/<phase>-stepN-<slug>
m = re.match(r"refactor/(phase-[a-z]+)-step(\d+)-", branch)
if not step and m:
    hits = glob.glob(f"docs/roadmap/{m.group(1)}-*/step{m.group(2)}-*.md")
    if len(hits) != 1:
        problems.append(f"branch {branch}: expected one step file, found {hits}")
    step = hits[0] if len(hits) == 1 else ""

def to_regex(g):
    r = re.escape(g).replace(r"\*\*/", "(?:.*/)?").replace(r"\*\*", ".*").replace(r"\*", "[^/]*").replace(r"\?", "[^/]")
    return re.compile(r + "$")

def expand(g):  # {a,b} alternatives
    mm = re.search(r"\{([^}]*)\}", g)
    return [x for alt in mm.group(1).split(",") for x in expand(g[:mm.start()] + alt + g[mm.end():])] if mm else [g]

GUARDS = re.compile(r"^(tools/check-|tools/review\.sh$|tools/original-assets|\.githooks/|\.github/|\.gitleaks\.toml$|\.gitignore$|REVIEW\.md$)")
if step:
    row = next((l for l in open(step, encoding="utf-8") if l.startswith("| Allowed paths |")), None)
    if row is None:
        problems.append(f"{step}: no '| Allowed paths |' row")
    else:
        globs = [to_regex(x) for t in re.findall(r"`([^`]+)`", row) for x in expand(t)]
        for f in changed:
            if not any(g.match(f) for g in globs):
                problems.append(f"scope: {f} is outside {step} Allowed paths")
else:
    for f in changed:
        if GUARDS.match(f):
            print(f"NOTE {f}: guard file changed outside a step; the owner must approve it")

# Anti-gaming: added lines that switch checks off. Mark a justified line with 'review: allow <reason>'.
BANNED = re.compile(r"#\s*pragma\s+(warning|clang\s+diagnostic|GCC\s+diagnostic)|NOLINT|-Wno-|\bDISABLED_|--no-verify|continue-on-error:\s*true")
file = None
for line in git("diff", "-U0", "--src-prefix=a/", "--dst-prefix=b/", base).splitlines():
    if line.startswith("+++ "):
        file = line[6:] if line.startswith("+++ b/") else None
    elif file and line.startswith("+") and not file.endswith(".md") and file != "tools/review.sh":
        if BANNED.search(line) and "review: allow" not in line:
            problems.append(f"anti-gaming: {file}: {line[1:].strip()[:120]}")

# DONE gate: a step set to DONE needs its fresh-context review record.
for f in changed:
    sm = re.match(r"(docs/roadmap/[^/]+)/step(\d+)-[^/]*\.md$", f)
    if not sm or not os.path.exists(f) or "**Status:** DONE" not in open(f, encoding="utf-8").read():
        continue
    rec = f"{sm.group(1)}/reviews/step{sm.group(2)}.md"
    text = open(rec, encoding="utf-8").read() if os.path.exists(rec) else ""
    if not text:
        problems.append(f"DONE gate: {f} is DONE but {rec} is missing")
    elif "Reviewer: fresh-context subagent" not in text or "## Findings" not in text:
        problems.append(f"DONE gate: {rec} lacks 'Reviewer: fresh-context subagent' or '## Findings'")
    elif re.search(r"\|\s*pending\s*\|", text):
        problems.append(f"DONE gate: {rec} still has pending findings")
    elif not os.path.exists(rep := f"{sm.group(1)}/reviews/step{sm.group(2)}-report.md"):
        problems.append(f"DONE gate: {rec} has no raw reviewer report {rep}")
    elif "SKILL: Launching skill: thermo-nuclear-code-quality-review" not in (rtext := open(rep, encoding="utf-8").read()) or "SKILL: NOT LOADED" in rtext:
        problems.append(f"DONE gate: {rep} lacks the skill's load line, or says NOT LOADED")
    else:
        pns = open("docs/roadmap/possible-new-scope.md", encoding="utf-8").read()
        for row in re.findall(r"^\|.*\bdeferred\b.*\|\s*$", text, re.M):
            ids = re.findall(r"PNS-(\d+)", row)
            if not ids:
                problems.append(f"DONE gate: {rec}: deferred finding without a PNS-n: {row.strip()[:100]}")
            for n in ids:
                if f"### PNS-{n}:" not in pns:
                    problems.append(f"DONE gate: {rec}: PNS-{n} is not in possible-new-scope.md")

print(f"step: {step or 'none (not a step branch)'}; {len(changed)} changed files")
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
