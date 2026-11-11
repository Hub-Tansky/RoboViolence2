#!/bin/sh
# Fails when C/C++ lines changed since the merge-base with <base> don't match .clang-format.
# Changed lines only, like the commit hook (ADR 0045). Needs git-clang-format and clang-format on PATH.
# Usage: tools/check-format.sh [base]   (default origin/main)
set -e
base=${1:-origin/main}
command -v git-clang-format >/dev/null && command -v clang-format >/dev/null || {
  echo "format: git-clang-format or clang-format not found (pipx install clang-format==23.1.5)" >&2
  exit 2
}
mb=$(git merge-base "$base" HEAD) || { echo "format: no merge-base with $base" >&2; exit 2; }
files=$(git diff --name-only --diff-filter=ACMR "$mb" HEAD -- '*.c' '*.cc' '*.cpp' '*.h' '*.H' '*.hpp' '*.inl' | grep -Ev '(^|/)third_party/' || true)
if [ -z "$files" ]; then
  echo "format: no C/C++ changes since $mb"
  exit 0
fi
# Exit 0: nothing to change. Exit 1 with a diff: lines to fix. Anything else is a tool failure.
rc=0
# shellcheck disable=SC2086 # file names have no spaces (check-hygiene.py)
out=$(git-clang-format --style=file --diff "$mb" HEAD -- $files 2>&1) || rc=$?
if [ "$rc" -eq 0 ]; then
  echo "format: ok ($(echo "$files" | wc -l | tr -d ' ') files, $(clang-format --version))"
  exit 0
fi
printf '%s\n' "$out"
case "$out" in
  *"diff --git"*)
    echo "format: changed lines don't match .clang-format. Fix with: git clang-format $mb (then commit), or apply the diff above." >&2
    exit 1
    ;;
esac
echo "format: git-clang-format failed (exit $rc)" >&2
exit 2
