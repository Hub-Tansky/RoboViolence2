// ROBO-238: TrueType to a CFont atlas with stb_truetype, apart from OpenGL so it can be tested headless.
#include "dkf_ttf.h"

#include "FontCodes.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

// static so it can't clash with another copy in a linked library
#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

namespace bv2
{
	// The code point a font byte bakes: its own, or when the font lacks it, the plain letter babo.tga draws for it (ROBO-265)
	static int bakedCodepoint(const stbtt_fontinfo& info, int b)
	{
		const int cp = (int)fontByteCodepoint(b);
		if (stbtt_FindGlyphIndex(&info, cp) == 0 && b >= FONT_BV2_BYTE_END && b < FONT_FIRST_BYTE + FONT_BYTE_COUNT)
			return (unsigned char)FONT_EXTRA[b - FONT_BV2_BYTE_END].plain;
		return cp;
	}

	bool bakeTtf(const unsigned char* ttf, size_t size, int lineHeight, BakedFont& out)
	{
		stbtt_fontinfo info;
		if (!ttf || size < 12 || lineHeight <= 0 || stbtt_GetFontOffsetForIndex(ttf, 0) < 0 || !stbtt_InitFont(&info, ttf, stbtt_GetFontOffsetForIndex(ttf, 0)))
			return false;
		int ascent = 0, descent = 0, gap = 0;
		stbtt_GetFontVMetrics(&info, &ascent, &descent, &gap);
		if (ascent - descent <= 0)
			return false;
		// The whole ascent-to-descent span fills one line, as the babo.tga letters fill their 64-pixel rows
		const float scaleY = (float)lineHeight / (float)(ascent - descent);
		const int baseline = (int)std::lround((float)ascent * scaleY); // from the top of the line
		// Narrow the letters, never widen them, so a-z average the babo.tga width and text keeps its length (D1)
		float lowerAdvance = 0;
		for (int c = 'a'; c <= 'z'; ++c)
		{
			int advance = 0, lsb = 0;
			stbtt_GetCodepointHMetrics(&info, c, &advance, &lsb);
			lowerAdvance += (float)advance * scaleY / (float)lineHeight / 26.0f;
		}
		const float scaleX = lowerAdvance > BV2_LOWERCASE_ADVANCE ? scaleY * BV2_LOWERCASE_ADVANCE / lowerAdvance : scaleY;
		const int pad = 2;

		// Lay the cells out in rows; grow the atlas until they fit
		int atlas = 256;
		std::vector<int> cellX(256, 0), cellY(256, 0), cellW(256, 0);
		for (;;)
		{
			int x = 0, y = 0;
			bool fits = true;
			for (int b = FONT_FIRST_BYTE; b < FONT_FIRST_BYTE + FONT_BYTE_COUNT && fits; ++b)
			{
				int advance = 0, lsb = 0;
				stbtt_GetCodepointHMetrics(&info, bakedCodepoint(info, b), &advance, &lsb);
				int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
				stbtt_GetCodepointBitmapBox(&info, bakedCodepoint(info, b), scaleX, scaleY, &x0, &y0, &x1, &y1);
				// the cell holds the advance and any ink outside it
				int left = x0 < 0 ? x0 : 0;
				int right = std::max((int)std::ceil((float)advance * scaleX), x1);
				int w = right - left + pad;
				if (x + w > atlas)
				{
					x = 0;
					y += lineHeight + pad;
				}
				if (y + lineHeight > atlas)
					fits = false;
				cellX[b] = x - left;
				cellY[b] = y;
				cellW[b] = w;
				x += w;
			}
			if (fits)
				break;
			atlas *= 2;
			if (atlas > 4096)
				return false;
		}

		out.atlasSize = atlas;
		out.rgba.assign((size_t)atlas * atlas * 4, 0);
		for (size_t i = 0; i < out.rgba.size(); i += 4)
			out.rgba[i] = out.rgba[i + 1] = out.rgba[i + 2] = 255;
		for (BakedGlyph& g : out.glyphs)
			g = BakedGlyph();

		std::vector<unsigned char> bitmap;
		for (int b = FONT_FIRST_BYTE; b < FONT_FIRST_BYTE + FONT_BYTE_COUNT; ++b)
		{
			const int cp = bakedCodepoint(info, b);
			BakedGlyph& g = out.glyphs[b];
			g.present = stbtt_FindGlyphIndex(&info, cp) != 0;
			int advance = 0, lsb = 0;
			stbtt_GetCodepointHMetrics(&info, cp, &advance, &lsb);
			const float advancePx = (float)advance * scaleX;
			g.w = advancePx / (float)lineHeight;
			// texture rectangle: the advance box, top of the line to the bottom; rows are stored bottom-up. Ink left of
			// the pen (a negative left bearing, as in j) is baked but not drawn, as babo.tga glyphs have none
			const int originX = cellX[b], top = cellY[b];
			g.u1 = (float)originX / (float)atlas;
			g.u2 = ((float)originX + advancePx) / (float)atlas;
			g.v1 = 1.0f - (float)top / (float)atlas;
			g.v2 = 1.0f - (float)(top + lineHeight) / (float)atlas;

			int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
			stbtt_GetCodepointBitmapBox(&info, cp, scaleX, scaleY, &x0, &y0, &x1, &y1);
			const int bw = x1 - x0, bh = y1 - y0;
			if (!g.present || bw <= 0 || bh <= 0)
				continue;
			bitmap.assign((size_t)bw * bh, 0);
			stbtt_MakeCodepointBitmap(&info, bitmap.data(), bw, bh, bw, scaleX, scaleY, cp);
			for (int j = 0; j < bh; ++j)
			{
				const int py = top + baseline + y0 + j; // from the top of the atlas
				if (py < top || py >= top + lineHeight || py >= atlas)
					continue;
				for (int i = 0; i < bw; ++i)
				{
					const int px = originX + x0 + i;
					if (px < 0 || px >= atlas)
						continue;
					const size_t row = (size_t)(atlas - 1 - py); // bottom-up storage
					unsigned char& a = out.rgba[(row * atlas + px) * 4 + 3];
					a = std::max(a, bitmap[(size_t)j * bw + i]);
				}
			}
		}
		return true;
	}

	bool bakeTtfFile(const char* path, int lineHeight, BakedFont& out)
	{
		FILE* f = path ? fopen(path, "rb") : nullptr;
		if (!f)
			return false;
		std::vector<unsigned char> data;
		unsigned char chunk[65536];
		for (;;)
		{
			const size_t got = fread(chunk, 1, sizeof(chunk), f);
			data.insert(data.end(), chunk, chunk + got);
			if (got < sizeof(chunk))
				break; // end of file or a read error: stop either way
		}
		fclose(f);
		return bakeTtf(data.data(), data.size(), lineHeight, out);
	}
}
