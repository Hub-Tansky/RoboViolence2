// Glyph widths of BV2's babo.tga font, in 64ths of a line height, for font bytes 32 to 159 (ROBO-264). Measured from the
// original font (Daivuk/BaboViolent2 Content/main/fonts/babo.tga) the way CFont::loadTGAFile measures them; the font
// itself is not in this repository (ADR 0010). Their a-z average is bv2::BV2_LOWERCASE_ADVANCE.
#pragma once

// clang-format off: 16 bytes a row
const int BABO_WIDTH_64THS[128] = {
	16, 8, 18, 18, 18, 18, 18, 8, 10, 11, 17, 18, 12, 18, 8, 17,
	18, 18, 18, 18, 16, 17, 18, 18, 18, 18, 8, 12, 18, 18, 17, 17,
	18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 14, 18, 18, 18, 18, 18,
	18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 17, 18, 17, 25,
	11, 18, 18, 18, 18, 18, 16, 18, 18, 14, 14, 18, 14, 18, 18, 18,
	18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 34, 6, 34, 18, 25,
	18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 17, 18, 16, 18, 18,
	18, 22, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 17, 15, 18,
};
// clang-format on
