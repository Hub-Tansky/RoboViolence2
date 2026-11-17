#!/bin/bash
# Test for tools/check-pack.py (ROBO-231, ADR 0065): author, licence, unique ids and files present.
# Usage: test_check_pack.sh <scratch dir>
set -u
script=$(cd "$(dirname "$0")/../tools" && pwd)/check-pack.py
scratch=${1:?scratch dir}
rm -rf "$scratch" && mkdir -p "$scratch/textures" && cd "$scratch" || exit 1
echo x > textures/A.tga

failures=0
expect() { # expect <name> <expected exit> <expected output fragment>
	out=$(python3 "$script" . 2>&1); rc=$?
	if [ "$rc" = "$2" ] && printf '%s' "$out" | grep -qF -- "$3"; then echo "ok   $1"; else echo "FAIL $1: exit $rc, output: $out"; failures=$((failures + 1)); fi
}
manifest() { # manifest <row>...: pack.tsv with the header and these rows (fields separated by |)
	{ echo "# comment"; printf 'id\tfile\ttype\tformat\tauthor\ttool\tlicence\tsource\n'; for r in "$@"; do printf '%s\n' "$r" | tr '|' '\t'; done; } > pack.tsv
}
good="textures/A|textures/A.tga|texture|TGA|Ann|Krita|CC0-1.0|own"

manifest "$good"
expect "complete pack passes" 0 "ok: pack"
manifest "textures/A|textures/A.tga|texture|TGA||Krita|CC0-1.0|own"
expect "empty author fails" 1 "empty author"
manifest "textures/A|textures/A.tga|texture|TGA|Ann|Krita||own"
expect "empty licence fails" 1 "empty licence"
manifest "$good" "textures/B|textures/B.tga|texture|TGA|Ann|Krita|CC0-1.0|own"
expect "file missing from the pack fails" 1 "textures/B.tga is not in the pack"
manifest "$good" "sounds/M|sounds/M.ogg|sound|OGG|Ann||CC0-1.0|missing"
expect "a missing-source row may lack its file" 0 "NOTE sounds/M: no file yet"
manifest "$good" "TEXTURES/a|textures/A.tga|texture|TGA|Ann|Krita|CC0-1.0|own"
expect "duplicate id (case-insensitive) fails" 1 "duplicate id"
manifest "textures/A|textures/A.tga|picture|TGA|Ann|Krita|CC0-1.0|own"
expect "unknown type fails" 1 "unknown type"
manifest "$good" "textures/C|../outside.tga|texture|TGA|Ann|Krita|CC0-1.0|own"
expect "a file outside the pack root fails" 1 "file must stay inside the pack root"
manifest "$good" "/etc/x|textures/A.tga|texture|TGA|Ann|Krita|CC0-1.0|own"
expect "an absolute id fails" 1 "id must stay inside the pack root"
manifest "$good" "textures/D|textures/D.tga|texture|TGA|Ann|Krita|CC0-1.0"
expect "a row with too few columns fails" 1 "7 columns, header has 8"
printf 'id\tfile\n' > pack.tsv
expect "header without a column fails" 1 "header lacks"

[ "$failures" = 0 ] && echo "all passed"
exit "$failures"
