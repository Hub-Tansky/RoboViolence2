#!/bin/sh
# Fails when C/C++ lines changed since the merge-base with <base> don't match .clang-format.
# Changed lines only, like the commit hook (ADR 0045). Needs git-clang-format on PATH.
# Usage: tools/check-format.sh [base]   (default origin/main)
set -e
base=${1:-origin/main}
mb=$(git merge-base "$base" HEAD)
files=$(git diff --name-only --diff-filter=ACMR "$mb" HEAD -- '*.c' '*.cc' '*.cpp' '*.h' '*.hpp' '*.inl' | grep -v '/third_party/' || true)
if [ -z "$files" ]; then
  echo "format: no C/C++ changes"
  exit 0
fi
# shellcheck disable=SC2086 # file names have no spaces (check-hygiene.py)
out=$(git-clang-format --style=file --diff "$mb" HEAD -- $files) || true
case "$out" in
  "" | "no modified files to format" | *"did not modify any files"*)
    echo "format: ok ($(echo "$files" | wc -l | tr -d ' ') files)"
    exit 0
    ;;
esac
printf '%s\n' "$out"
echo "format: changed lines don't match .clang-format. Fix with: git clang-format $mb (then commit), or apply the diff above." >&2
exit 1
