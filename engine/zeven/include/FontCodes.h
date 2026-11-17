// Which character each font byte draws (ROBO-238). Text is one byte per glyph: 32-127 are ASCII, 128-159 the 32
// Latin-1 letters of the original BV2 font in its order, and 160-255 the 96 letters below (owner decision 2026-11-17:
// Central European, Baltic, Turkish and Romanian letters plus the Latin-1 ones BV2 lacked, such as o-acute and n-tilde).
// lang::utf8ToFont maps UTF-8 to these bytes and a TTF font bakes these code points, so both agree on one table.
// babo.tga has no glyphs past 159: CFont draws the plain letter given here instead.
#pragma once

#include <string>

namespace bv2
{
	const int FONT_FIRST_BYTE = 32;
	const int FONT_BYTE_COUNT = 224; // 32-255
	const int FONT_BV2_BYTE_END = 160; // babo.tga has glyphs for 32-159

	// clang-format off: one row per 8 letters, so each row's comment shows them
	// The code points of bytes 128-159
	const unsigned int FONT_LATIN1[32] = {199, 252, 233, 226, 228, 224, 229, 231, 234, 235, 232, 239, 238, 236, 196, 197, 201, 230,
		198, 244, 246, 242, 251, 249, 255, 214, 220, 248, 163, 216, 215, 170};

	// Bytes 160-255: code point, and the ASCII letter a font without it (babo.tga) draws instead
	struct FontExtraLetter
	{
		unsigned int codepoint;
		char plain;
	};
	const FontExtraLetter FONT_EXTRA[96] = {
		{0x0E1, 'a'}, {0x0ED, 'i'}, {0x0F3, 'o'}, {0x0FA, 'u'}, {0x0FD, 'y'}, {0x0F1, 'n'}, {0x0E3, 'a'}, {0x0F5, 'o'}, // áíóúýñãõ
		{0x0DF, 's'}, {0x0C1, 'A'}, {0x0CD, 'I'}, {0x0D3, 'O'}, {0x0DA, 'U'}, {0x0DD, 'Y'}, {0x0D1, 'N'}, {0x104, 'A'}, // ßÁÍÓÚÝÑĄ
		{0x105, 'a'}, {0x106, 'C'}, {0x107, 'c'}, {0x118, 'E'}, {0x119, 'e'}, {0x141, 'L'}, {0x142, 'l'}, {0x143, 'N'}, // ąĆćĘęŁłŃ
		{0x144, 'n'}, {0x15A, 'S'}, {0x15B, 's'}, {0x179, 'Z'}, {0x17A, 'z'}, {0x17B, 'Z'}, {0x17C, 'z'}, {0x10C, 'C'}, // ńŚśŹźŻżČ
		{0x10D, 'c'}, {0x10E, 'D'}, {0x10F, 'd'}, {0x11A, 'E'}, {0x11B, 'e'}, {0x147, 'N'}, {0x148, 'n'}, {0x158, 'R'}, // čĎďĚěŇňŘ
		{0x159, 'r'}, {0x160, 'S'}, {0x161, 's'}, {0x164, 'T'}, {0x165, 't'}, {0x16E, 'U'}, {0x16F, 'u'}, {0x17D, 'Z'}, // řŠšŤťŮůŽ
		{0x17E, 'z'}, {0x150, 'O'}, {0x151, 'o'}, {0x170, 'U'}, {0x171, 'u'}, {0x102, 'A'}, {0x103, 'a'}, {0x15E, 'S'}, // žŐőŰűĂăŞ
		{0x15F, 's'}, {0x162, 'T'}, {0x163, 't'}, {0x11E, 'G'}, {0x11F, 'g'}, {0x130, 'I'}, {0x131, 'i'}, {0x110, 'D'}, // şŢţĞğİıĐ
		{0x111, 'd'}, {0x100, 'A'}, {0x101, 'a'}, {0x112, 'E'}, {0x113, 'e'}, {0x122, 'G'}, {0x123, 'g'}, {0x12A, 'I'}, // đĀāĒēĢģĪ
		{0x12B, 'i'}, {0x136, 'K'}, {0x137, 'k'}, {0x13B, 'L'}, {0x13C, 'l'}, {0x145, 'N'}, {0x146, 'n'}, {0x16A, 'U'}, // īĶķĻļŅņŪ
		{0x16B, 'u'}, {0x116, 'E'}, {0x117, 'e'}, {0x12E, 'I'}, {0x12F, 'i'}, {0x172, 'U'}, {0x173, 'u'}, {0x152, 'O'}, // ūĖėĮįŲųŒ
		{0x153, 'o'}, {0x13D, 'L'}, {0x13E, 'l'}, {0x139, 'L'}, {0x13A, 'l'}, {0x154, 'R'}, {0x155, 'r'}, {0x0BF, '?'}, // œĽľĹĺŔŕ¿
	};
	// clang-format on

	// Code point a font byte draws, or 0 when it draws none
	inline unsigned int fontByteCodepoint(int b)
	{
		if (b >= 32 && b < 128)
			return (unsigned int)b;
		if (b >= 128 && b < FONT_BV2_BYTE_END)
			return FONT_LATIN1[b - 128];
		if (b >= FONT_BV2_BYTE_END && b < FONT_FIRST_BYTE + FONT_BYTE_COUNT)
			return FONT_EXTRA[b - FONT_BV2_BYTE_END].codepoint;
		return 0;
	}

	// Font byte for a code point, or -1 (colour codes and newline are not glyphs: see lang::utf8ToFont)
	inline int fontByteFor(unsigned int c)
	{
		// Romanian's comma-below letters Ș ș Ț ț draw as the cedilla ones in the table
		if (c >= 0x218 && c <= 0x21B)
		{
			const unsigned int cedilla[4] = {0x15E, 0x15F, 0x162, 0x163};
			c = cedilla[c - 0x218];
		}
		for (int b = FONT_FIRST_BYTE; b < FONT_FIRST_BYTE + FONT_BYTE_COUNT; ++b)
			if (fontByteCodepoint(b) == c)
				return b;
		return -1;
	}

	// Font byte for a typed code point, or 0 when it is a control character or has no glyph
	inline unsigned int typedFontByte(unsigned int c)
	{
		if (c < 32 || c == 127)
			return 0;
		const int b = fontByteFor(c);
		return b < 0 ? 0 : (unsigned int)b;
	}

	// Font-byte text (what Writting holds) as Latin-1 bytes, for text that leaves the game, such as a typed folder path
	// (ADR 0042); a letter outside Latin-1 becomes '?'
	inline std::string fontTextToLatin1(const std::string& text)
	{
		std::string out;
		for (unsigned char b : text)
		{
			const unsigned int c = b < 128 ? b : fontByteCodepoint(b);
			out += c > 0 && c <= 255 ? (char)c : '?';
		}
		return out;
	}
}
