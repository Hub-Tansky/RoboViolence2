// TrueType fonts baked to the kind of atlas CFont draws (ROBO-238, ADR 0010: new formats beside the old).
// Each font byte (FontCodes.h) gets a cell one line high; widths are in line heights, like the babo.tga glyphs, so text
// keeps its size and place.
#pragma once

#include <vector>

namespace bv2
{
	// Average advance of a-z in babo.tga, in line heights (measured on the original BV2 font). A wider TTF is narrowed to
	// it so strings keep their length and the menus their layout.
	const float BV2_LOWERCASE_ADVANCE = 0.273f;

	struct BakedGlyph
	{
		float u1 = 0, v1 = 0, u2 = 0, v2 = 0; // texture rectangle, v1 at the top of the line (v grows upwards, as in CFont)
		float w = 0; // advance in line heights
		bool present = false; // the font has this character
	};

	struct BakedFont
	{
		int atlasSize = 0; // square, power of two
		std::vector<unsigned char> rgba; // white, coverage in alpha; rows bottom to top like every dkt upload
		BakedGlyph glyphs[256]; // by font byte
	};

	// Bakes every font byte's character from TTF data at `lineHeight` pixels a line. False when the data isn't a font.
	bool bakeTtf(const unsigned char* ttf, size_t size, int lineHeight, BakedFont& out);

	// Reads and bakes a .ttf file
	bool bakeTtfFile(const char* path, int lineHeight, BakedFont& out);
}
