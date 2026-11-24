#!/bin/bash
# Test for tools/check-pack.py (ROBO-231, ADR 0065): author, licence, unique ids and files present; art pack rules (ROBO-264).
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

manifest "textures/A|textures/A.tga|texture|TGA|Ann||LicenseRef-RV2-Art|own"
expect "an art licence row without a tool fails" 1 "empty tool"
manifest "textures/A|textures/A.tga|texture|TGA|Ann|Krita|LicenseRef-RV2-Art|own"
expect "an art licence row with author, tool and licence passes" 0 "ok: pack"

# ROBO-264: an art pack against the base pack (--base)
img() { # img <file> <width> <height> <alpha 0|1>: a PNG header (or a TGA for *.tga) with that size and alpha
	python3 - "$@" <<'PY'
import struct, sys, zlib
path, w, h, alpha = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4] == "1"
if path.endswith(".tga"):
    open(path, "wb").write(struct.pack("<BBBHHBHHHHBB", 0, 0, 2, 0, 0, 0, 0, 0, w, h, 32 if alpha else 24, 0))
else:
    ihdr = struct.pack(">IIBBBBB", w, h, 8, 6 if alpha else 2, 0, 0, 0)
    open(path, "wb").write(b"\x89PNG\r\n\x1a\n" + struct.pack(">I", 13) + b"IHDR" + ihdr + struct.pack(">I", zlib.crc32(b"IHDR" + ihdr)))
PY
}
printf 'id\tfile\ttype\tformat\n' > base.tsv
printf 'textures/Sky\ttextures/sky.tga\ttexture\tTGA 256×256×24\ntextures/Halo\ttextures/Halo.tga\ttexture\tTGA 64×64×32\nfonts/babo\tfonts/babo.tga\tfont\tTGA 512×512×32\n' >> base.tsv
expect_base() { # expect_base <name> <expected exit> <expected output fragment>
	out=$(python3 "$script" . --base base.tsv 2>&1); rc=$?
	if [ "$rc" = "$2" ] && printf '%s' "$out" | grep -qF -- "$3"; then echo "ok   $1"; else echo "FAIL $1: exit $rc, output: $out"; failures=$((failures + 1)); fi
}
art() { echo "$1|$2|$3|PNG|Ann|Krita|LicenseRef-RV2-Art|own"; } # art <id> <file> <type>: an art pack row
img textures/sky.png 512 512 0; img textures/halo.png 128 128 1; img textures/flat.png 300 256 0; img textures/Old.tga 64 64 0
manifest "$(art textures/Sky textures/sky.png texture)" "$(art textures/Halo textures/halo.png texture)"
expect_base "an art pack of power-of-two PNGs with alpha where the base has it passes" 0 "ok: pack"
manifest "$(art textures/Sky textures/flat.png texture)"
expect_base "a texture that is not a power of two fails" 1 "300x256 is not a power of two"
manifest "$(art textures/Halo textures/sky.png texture)"
expect_base "a texture without alpha where the base is 32-bit fails" 1 "no alpha, the base texture is 32-bit"
manifest "$(art textures/Halo textures/Old.tga texture)"
expect_base "a 24-bit TGA where the base is 32-bit fails" 1 "no alpha"
manifest "$(art textures/sky textures/sky.png texture)"
expect_base "an id differing from a base id only in case fails" 1 "differs from base id textures/Sky only in case"
manifest "$(art fonts/babo textures/sky.png texture)"
expect_base "a type other than the base row's fails" 1 "type texture, base pack has font"
manifest "$(art textures/themes/wasteland/tex_floor textures/sky.png texture)"
expect_base "a new id passes and is listed" 0 "NOTE textures/themes/wasteland/tex_floor: new id"
python3 - <<'PY' # an RGB PNG whose pixel data holds the bytes tRNS, and one with a real tRNS chunk before IDAT
import struct, zlib
def chunk(kind, body): return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body))
ihdr = chunk(b"IHDR", struct.pack(">IIBBBBB", 64, 64, 8, 2, 0, 0, 0))
open("textures/fake.png", "wb").write(b"\x89PNG\r\n\x1a\n" + ihdr + chunk(b"IDAT", b"..tRNS..") + chunk(b"IEND", b""))
open("textures/key.png", "wb").write(b"\x89PNG\r\n\x1a\n" + ihdr + chunk(b"tRNS", b"\0\0\0\0\0\0") + chunk(b"IDAT", b"") + chunk(b"IEND", b""))
PY
manifest "$(art textures/Halo textures/fake.png texture)"
expect_base "tRNS bytes inside the pixel data are not alpha" 1 "no alpha"
manifest "$(art textures/Halo textures/key.png texture)"
expect_base "a tRNS chunk before the pixel data is alpha" 0 "ok: pack"
manifest "$(art textures/Sky textures/A.tga texture)"
expect_base "a texture that is not a PNG or TGA picture fails" 1 "is not a PNG or TGA"

[ "$failures" = 0 ] && echo "all passed"
exit "$failures"
