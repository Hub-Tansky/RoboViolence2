// ROBO-264, ROBO-265: text fit for a replacement TTF font (Teko, the new art's font). Every English menu and HUD string in
// en.lang, plus the numbers the HUD draws, may be at most 20 % wider than with BV2's babo.tga (owner 2026-11-24: accept
// longer text, the layout stays), and every font byte must bake a glyph, its own or the plain letter babo.tga draws for
// it (the characters translations may use). No GL context needed.
// Usage: test_text_fit <en.lang> <font.ttf>...
#include "FontCodes.h"
#include "babo_widths.h"
#include "dkf_ttf.h"

#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

static int g_fail = 0;
static const float WIDTH_BUDGET = 1.20f; // most a string may grow over its babo.tga width (ROBO-265)

// Width in line heights of the printable bytes of `text` with babo.tga's widths (font bytes 32 to 159)
static float baboWidth(const std::string& text)
{
	float w = 0;
	for (unsigned char c : text)
		if (c >= 32 && c < bv2::FONT_BV2_BYTE_END)
			w += BABO_WIDTH_64THS[c - 32] / 64.0f;
	return w;
}

static float bakedWidth(const bv2::BakedFont& f, const std::string& text)
{
	float w = 0;
	for (unsigned char c : text)
		if (c >= 32 && c < bv2::FONT_BV2_BYTE_END)
			w += f.glyphs[c].w;
	return w;
}

// The values of en.lang (`key<TABs>value`), plus strings the HUD builds itself
static std::vector<std::string> strings(const char* lang)
{
	std::vector<std::string> out = {"0123456789", "100", "-999", "99:59"};
	std::ifstream in(lang);
	for (std::string line; std::getline(in, line);)
	{
		const size_t tab = line.find('\t');
		const size_t value = line.find_first_not_of('\t', tab);
		if (line.rfind("lang_", 0) == 0 && tab != std::string::npos && value != std::string::npos)
			out.push_back(line.substr(value));
	}
	return out;
}

int main(int argc, char** argv)
{
	if (argc < 3)
	{
		std::printf("usage: test_text_fit <en.lang> <font.ttf>...\n");
		return 2;
	}
	const std::vector<std::string> texts = strings(argv[1]);
	if (texts.size() < 100)
	{
		std::printf("FAIL: only %zu strings read from %s\n", texts.size(), argv[1]);
		return 1;
	}
	for (int i = 2; i < argc; ++i)
	{
		bv2::BakedFont f;
		if (!bv2::bakeTtfFile(argv[i], 64, f))
		{
			std::printf("FAIL: %s didn't bake\n", argv[i]);
			g_fail = 1;
			continue;
		}
		for (int b = 33; b < bv2::FONT_FIRST_BYTE + bv2::FONT_BYTE_COUNT; ++b)
			if (b != 127 && bv2::fontByteCodepoint(b) && !f.glyphs[b].present) // 127 is DEL, no glyph
			{
				std::printf("FAIL: %s has no glyph or plain letter for U+%04X (font byte %d)\n", argv[i], bv2::fontByteCodepoint(b), b);
				g_fail = 1;
			}
		// Teko has no Ľ (U+013D): it draws the plain L, as babo.tga does
		const bv2::BakedGlyph &lCaron = f.glyphs[bv2::fontByteFor(0x13D)], &l = f.glyphs['L'];
		if (!lCaron.present || lCaron.w != l.w)
		{
			std::printf("FAIL: %s: U+013D doesn't draw as plain L\n", argv[i]);
			g_fail = 1;
		}
		float widest = 0;
		for (const std::string& t : texts)
		{
			const float baked = bakedWidth(f, t), babo = baboWidth(t);
			widest = baked / babo > widest ? baked / babo : widest;
			if (baked > babo * WIDTH_BUDGET + 0.01f)
			{
				std::printf("FAIL: %s: \"%s\" is %.2f line heights wide, %.2f with babo.tga (budget %.0f %%)\n", argv[i], t.c_str(),
							baked, babo, WIDTH_BUDGET * 100);
				g_fail = 1;
			}
		}
		std::printf("%s: %zu strings, widest %.0f %% of the babo.tga width\n", argv[i], texts.size(), widest * 100);
	}
	if (!g_fail)
		std::printf("ok: every string fits the budget\n");
	return g_fail;
}
