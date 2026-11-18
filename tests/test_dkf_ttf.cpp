// ROBO-238: a TTF bakes to a CFont atlas: every font byte's glyph is present with sane metrics, ink lands inside its
// cell, and widths are in line heights like the babo.tga glyphs. No GL context needed.
#include "FontCodes.h"
#include "dkf_ttf.h"

#include <cstdio>

static int g_fail = 0;

static void check(bool ok, const char* what)
{
	if (!ok)
	{
		std::printf("FAIL: %s\n", what);
		g_fail = 1;
	}
}

// Alpha at a texture coordinate (u right, v up, as CFont uses them)
static int alphaAt(const bv2::BakedFont& f, float u, float v)
{
	int x = (int)(u * f.atlasSize), y = (int)(v * f.atlasSize); // y counts rows from the bottom, as stored
	if (x < 0 || y < 0 || x >= f.atlasSize || y >= f.atlasSize)
		return -1;
	return f.rgba[((size_t)y * f.atlasSize + x) * 4 + 3];
}

static int inkIn(const bv2::BakedFont& f, const bv2::BakedGlyph& g)
{
	int ink = 0;
	for (int y = (int)(g.v2 * f.atlasSize); y < (int)(g.v1 * f.atlasSize); ++y)
		for (int x = (int)(g.u1 * f.atlasSize); x < (int)(g.u2 * f.atlasSize); ++x)
			ink += alphaAt(f, (x + 0.5f) / f.atlasSize, (y + 0.5f) / f.atlasSize) > 128;
	return ink;
}

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		std::printf("usage: test_dkf_ttf <font.ttf>\n");
		return 2;
	}
	bv2::BakedFont f;
	check(bv2::bakeTtfFile(argv[1], 64, f), "the TTF didn't bake");
	check(f.atlasSize >= 256 && (f.atlasSize & (f.atlasSize - 1)) == 0, "atlas size is not a power of two");
	check(f.rgba.size() == (size_t)f.atlasSize * f.atlasSize * 4, "atlas byte count");

	for (int b = bv2::FONT_FIRST_BYTE; b < bv2::FONT_FIRST_BYTE + bv2::FONT_BYTE_COUNT; ++b)
	{
		if (b == 127)
			continue; // DEL: fonts have no glyph for it
		const bv2::BakedGlyph& g = f.glyphs[b];
		char what[96];
		std::snprintf(what, sizeof what, "byte %d (U+%04X) missing or out of the atlas", b, bv2::fontByteCodepoint(b));
		check(g.present && g.w > 0.1f && g.w < 1.5f && g.u1 >= 0 && g.u2 <= 1 && g.u2 > g.u1 && g.v2 >= 0 && g.v1 <= 1, what);
		// one line high: 64 pixels
		std::snprintf(what, sizeof what, "byte %d cell is not one line high", b);
		check((int)((g.v1 - g.v2) * f.atlasSize + 0.5f) == 64, what);
		if (b != ' ')
		{
			std::snprintf(what, sizeof what, "byte %d (U+%04X) has no ink in its cell", b, bv2::fontByteCodepoint(b));
			check(inkIn(f, g) > 10, what);
		}
	}
	check(inkIn(f, f.glyphs[' ']) == 0, "space has ink");
	check(f.glyphs['i'].w < f.glyphs['W'].w, "i is not narrower than W");
	check(f.glyphs['W'].w > 0.3f && f.glyphs['W'].w < 1.0f, "W is not wider than an average letter and under a line");
	check(f.glyphs[bv2::fontByteFor(0xE9)].w == f.glyphs['e'].w, "e acute is not as wide as e");
	check(bv2::fontByteFor(0x142) >= bv2::FONT_BV2_BYTE_END && f.glyphs[bv2::fontByteFor(0x142)].present, "l-stroke has no byte or glyph");
	float lower = 0;
	for (int c = 'a'; c <= 'z'; ++c)
		lower += f.glyphs[c].w / 26.0f;
	check(lower > bv2::BV2_LOWERCASE_ADVANCE - 0.005f && lower < bv2::BV2_LOWERCASE_ADVANCE + 0.005f,
		  "a-z don't average the babo.tga width, so text would change length");

	// Nothing outside the cells: total ink equals the ink inside them
	int total = 0, inside = 0;
	for (size_t i = 3; i < f.rgba.size(); i += 4)
		total += f.rgba[i] > 128;
	for (int b = bv2::FONT_FIRST_BYTE; b < bv2::FONT_FIRST_BYTE + bv2::FONT_BYTE_COUNT; ++b)
		inside += inkIn(f, f.glyphs[b]);
	check(inside >= total * 98 / 100, "ink outside the glyph cells (overlapping or misplaced glyphs)");

	bv2::BakedFont none;
	const unsigned char junk[16] = {1, 2, 3};
	check(!bv2::bakeTtf(junk, sizeof junk, 64, none), "junk baked");
	check(!bv2::bakeTtfFile("no-such-font.ttf", 64, none), "a missing file baked");

	if (!g_fail)
		std::printf("ok: %d glyphs in a %d px atlas\n", bv2::FONT_BYTE_COUNT, f.atlasSize);
	return g_fail;
}
