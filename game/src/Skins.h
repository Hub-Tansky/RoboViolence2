// Ball skins (ROBO-258, ADR ROBO-258): full-colour skins from the art pack with a team mask, beside the BV2 64x32
// colour-mask skins. Names, picker order, bot faces, the fallback a game without the art pack uses, and the team
// recolour. No OpenGL here, so tests/test_skins.cpp runs it headless.
#ifndef BV2_SKINS_H
#define BV2_SKINS_H

#include <cmath>
#include <string>
#include <vector>

namespace bv2
{
	namespace skins
	{
		// Network packets carry a skin name in 7 bytes, so a name has at most 6 characters
		const size_t SKIN_NAME_MAX = 6;
		// The BV2 skins skin01..skin23 (64x32 colour masks in the player's data)
		const int LEGACY_COUNT = 23;
		// The default skin (cl_skin). With the art pack it is face 08, Ivory Serenity (owner 2026-11-24).
		const char* const DEFAULT_SKIN = "skin10";

		// The 17 approved faces (owner 2026-11-24), in picker order. Face 08 keeps the name of the default skin, so
		// old configs and older clients that show skin10 stay on it.
		const char* const FACES[] = {"face01", "face02", "face03", "face05", "face06", "face07", "skin10", "face09",
									 "face14", "face15", "face16", "face18", "face20", "face21", "face22", "face23", "face25"};
		const int FACE_COUNT = (int)(sizeof(FACES) / sizeof(FACES[0]));

		// Full-colour skin files are art pack ids: "skins/<name>" (RGB, 512x256 equirectangular) and "skins/<name>_mask"
		// (white where team colour goes)
		inline std::string colourId(const std::string& name)
		{
			return "skins/" + name;
		}
		inline std::string maskId(const std::string& name)
		{
			return "skins/" + name + "_mask";
		}

		// Base pack id of a BV2 skin (main/skins/skinNN.tga, ADR ROBO-258): its own prefix, so it never meets the art
		// pack's full-colour skins/<name>
		inline std::string legacyId(const std::string& name)
		{
			return "skins/legacy/" + name;
		}

		// A name a packet may carry: 1 to 6 lower-case letters and digits. Anything else (a path, an empty name) is
		// drawn as the default skin, so a peer can't make the game open a file outside main/skins/.
		inline bool validName(const std::string& name)
		{
			if (name.empty() || name.size() > SKIN_NAME_MAX)
				return false;
			for (char c : name)
				if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')))
					return false;
			return true;
		}

		// The BV2 skin drawn when a full-colour skin is missing (no art pack, or --placeholders): faceNN shows the
		// BV2 skin of the same number (wrapping past 23), a BV2 name shows itself, anything else the default.
		inline std::string legacyName(const std::string& name)
		{
			if (!validName(name))
				return DEFAULT_SKIN;
			if (name.size() == 6 && name.compare(0, 4, "face") == 0 && name[4] >= '0' && name[4] <= '9' && name[5] >= '0' &&
				name[5] <= '9')
			{
				const int n = (name[4] - '0') * 10 + (name[5] - '0');
				if (n < 1)
					return DEFAULT_SKIN;
				const int legacy = (n - 1) % LEGACY_COUNT + 1;
				return std::string("skin") + (char)('0' + legacy / 10) + (char)('0' + legacy % 10);
			}
			return name;
		}

		// Face of a new server bot: a random one (roll, e.g. rand()) among the faces no other bot wears (taken holds
		// their FACES indexes), or among all once every face is worn
		inline int pickFace(const std::vector<int>& taken, int roll)
		{
			std::vector<int> free;
			for (int i = 0; i < FACE_COUNT; ++i)
			{
				bool used = false;
				for (int t : taken)
					used = used || t == i;
				if (!used)
					free.push_back(i);
			}
			if (roll < 0)
				roll = -(roll + 1);
			return free.empty() ? roll % FACE_COUNT : free[(size_t)roll % free.size()];
		}

		// Skin name of a FACES index (wraps, so any int is safe)
		inline const char* faceSkin(int face)
		{
			return FACES[(face % FACE_COUNT + FACE_COUNT) % FACE_COUNT];
		}

		// The skins the Appearance tab offers: the faces first when the art pack has them, then the BV2 skins (the
		// default skin only once)
		inline std::vector<std::string> choices(bool withFaces)
		{
			std::vector<std::string> list;
			if (withFaces)
				for (int i = 0; i < FACE_COUNT; ++i)
					list.push_back(FACES[i]);
			for (int i = 1; i <= LEGACY_COUNT; ++i)
			{
				const std::string name = std::string("skin") + (char)('0' + i / 10) + (char)('0' + i % 10);
				if (!withFaces || name != DEFAULT_SKIN)
					list.push_back(name);
			}
			return list;
		}

		// Index of name in list, 0 when it isn't there
		inline int indexOf(const std::vector<std::string>& list, const std::string& name)
		{
			for (size_t i = 0; i < list.size(); ++i)
				if (list[i] == name)
					return (int)i;
			return 0;
		}

		// Team red the skins are painted in (design/tools/export_assets.py RED); its brightness is the recolour's 1.0
		const float PAINT_RED[3] = {232.0f, 52.0f, 58.0f};
		// Team colours on the mask, 0..1 (the export preview's blue; the red the art is painted in)
		const float TEAM_RED[3] = {232.0f / 255.0f, 52.0f / 255.0f, 58.0f / 255.0f};
		const float TEAM_BLUE[3] = {47.0f / 255.0f, 123.0f / 255.0f, 255.0f / 255.0f};

		// The BV2 colour-mask recolour, as Player::updateSkin always did it: each pixel's red, green and blue weights
		// mix the three layer colours (0..1). A black pixel stays black. rgb is width*height*bpp bytes, bpp 3 or 4.
		inline void paintLayers(std::vector<unsigned char>& rgb, int bpp, const float red[3], const float green[3],
								const float blue[3])
		{
			if (bpp < 3)
				return;
			for (size_t k = 0; k + (size_t)bpp <= rgb.size(); k += (size_t)bpp)
			{
				const float r = rgb[k] / 255.0f, g = rgb[k + 1] / 255.0f, b = rgb[k + 2] / 255.0f;
				const float sum = r + g + b;
				for (int c = 0; c < 3; ++c)
				{
					const float v = sum > 0.0f ? (red[c] * r + green[c] * g + blue[c] * b) / sum : 0.0f;
					rgb[k + (size_t)c] = (unsigned char)(v <= 0.0f ? 0.0f : (v >= 1.0f ? 255.0f : v * 255.0f));
				}
			}
		}

		// Recolours a full-colour skin where its mask is set, as the export tool made the blue previews: each masked
		// pixel keeps its brightness relative to the paint red and takes colour (0..1). rgb is width*height*bpp bytes
		// (bpp 3 or 4); mask is width*height*maskBpp bytes, set where its first channel is above half. False, and
		// rgb unchanged, when the sizes don't match.
		inline bool paintMask(std::vector<unsigned char>& rgb, int bpp, const std::vector<unsigned char>& mask, int maskBpp,
							  int width, int height, const float colour[3])
		{
			const size_t pixels = (size_t)width * (size_t)height;
			if (width <= 0 || height <= 0 || bpp < 3 || maskBpp < 1 || rgb.size() != pixels * (size_t)bpp ||
				mask.size() != pixels * (size_t)maskBpp)
				return false;
			const double paint = PAINT_RED[0] * 0.3 + PAINT_RED[1] * 0.59 + PAINT_RED[2] * 0.11;
			for (size_t p = 0; p < pixels; ++p)
			{
				if (mask[p * (size_t)maskBpp] <= 127)
					continue;
				unsigned char* px = &rgb[p * (size_t)bpp];
				const double lum = (px[0] * 0.3 + px[1] * 0.59 + px[2] * 0.11) / paint;
				for (int c = 0; c < 3; ++c)
				{
					// colour went through 0..1 floats: the epsilon keeps 47/255*255 at 47, as the export's 0..255 maths
					const double v = std::floor(lum * colour[c] * 255.0 + 1e-3);
					px[c] = (unsigned char)(v < 0.0 ? 0.0 : (v > 255.0 ? 255.0 : v));
				}
			}
			return true;
		}
	}
}

#endif
