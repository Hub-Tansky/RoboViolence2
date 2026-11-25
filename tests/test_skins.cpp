// ROBO-258: skin names, fallbacks, bot faces, picker order and the two recolours (game/src/Skins.h)
#include "Skins.h"

#include <cstdio>
#include <string>
#include <vector>

static int g_fail = 0;

static void check(bool ok, const char* what)
{
	if (!ok)
	{
		std::printf("FAIL: %s\n", what);
		g_fail = 1;
	}
}

int main()
{
	using namespace bv2::skins;

	// The 17 approved faces (owner 2026-11-24); face 08 is the default skin's name
	check(FACE_COUNT == 17, "17 faces");
	const char* approved[] = {"01", "02", "03", "05", "06", "07", "08", "09", "14", "15", "16", "18", "20", "21", "22", "23", "25"};
	for (int i = 0; i < FACE_COUNT; ++i)
	{
		const std::string want = std::string(approved[i]) == "08" ? std::string(DEFAULT_SKIN) : std::string("face") + approved[i];
		check(want == FACES[i], ("face order: " + want).c_str());
		check(validName(FACES[i]), "every face name fits a packet");
	}

	// Names a packet may carry
	check(validName("skin10") && validName("face25"), "skin and face names are valid");
	check(!validName("") && !validName("skin100") && !validName("../x") && !validName("Skin1") && !validName("a/b"),
		  "empty, long, path and upper-case names are not");

	// Fallback to the BV2 skins
	check(legacyName("face05") == "skin05", "face05 falls back to skin05");
	check(legacyName("face25") == "skin02", "face25 wraps to skin02");
	check(legacyName("face23") == "skin23", "face23 stays skin23");
	check(legacyName("face00") == DEFAULT_SKIN, "face00 gets the default");
	check(legacyName("skin07") == "skin07", "a BV2 name stays");
	check(legacyName("../../etc") == DEFAULT_SKIN, "a path gets the default");

	// Bot faces: random among the free ones, never one another bot wears while one is free
	std::vector<int> taken;
	for (int n = 0; n < FACE_COUNT; ++n)
	{
		const int face = pickFace(taken, n * 7919 + 13);
		bool repeat = false;
		for (int t : taken)
			repeat = repeat || t == face;
		check(face >= 0 && face < FACE_COUNT && !repeat, "a free face while one is left");
		taken.push_back(face);
	}
	const int full = pickFace(taken, 5);
	check(full >= 0 && full < FACE_COUNT, "any face once all are worn");
	check(pickFace({}, -1) >= 0 && pickFace({}, -2147483647 - 1) >= 0, "negative rolls are safe");
	check(std::string(faceSkin(-1)) == FACES[FACE_COUNT - 1] && std::string(faceSkin(FACE_COUNT)) == FACES[0], "faceSkin wraps");

	// Picker: faces first with the art pack, the default skin once; only BV2 skins without it
	const std::vector<std::string> withArt = choices(true), without = choices(false);
	check(withArt.size() == (size_t)(FACE_COUNT + LEGACY_COUNT - 1), "faces plus 22 BV2 skins");
	check(withArt[0] == "face01" && withArt[FACE_COUNT] == "skin01", "faces first, then skin01");
	int defaults = 0;
	for (const std::string& s : withArt)
		defaults += s == DEFAULT_SKIN;
	check(defaults == 1, "skin10 listed once");
	check(without.size() == (size_t)LEGACY_COUNT && without[9] == "skin10", "without the art pack: skin01..skin23");
	check(indexOf(withArt, "skin10") == 6 && indexOf(without, "skin10") == 9 && indexOf(without, "zzz") == 0, "indexOf");

	// Team mask recolour, as design/tools/export_assets.py recolour() made the blue previews
	{
		std::vector<unsigned char> rgb = {232, 52, 58, 116, 26, 29, 10, 200, 30};
		const std::vector<unsigned char> mask = {255, 255, 255, 255, 255, 255, 0, 0, 0};
		check(paintMask(rgb, 3, mask, 3, 3, 1, TEAM_BLUE), "sizes match");
		check(rgb[0] == 47 && rgb[1] == 123 && rgb[2] == 255, "paint red becomes team blue");
		check(rgb[3] == 23 && rgb[4] == 61 && rgb[5] == 127, "a darker red becomes a darker blue");
		check(rgb[6] == 10 && rgb[7] == 200 && rgb[8] == 30, "unmasked pixels keep their colour");
		std::vector<unsigned char> again = {232, 52, 58};
		paintMask(again, 3, {255}, 1, 1, 1, TEAM_RED);
		check(again[0] >= 230 && again[1] >= 50 && again[1] <= 52 && again[2] >= 56 && again[2] <= 58, "team red keeps the paint");
		std::vector<unsigned char> bright = {255, 255, 255, 9};
		paintMask(bright, 4, {255}, 1, 1, 1, TEAM_BLUE);
		check(bright[2] == 255 && bright[3] == 9, "clamped, alpha untouched");
		std::vector<unsigned char> wrong = {1, 2, 3};
		check(!paintMask(wrong, 3, {255, 255}, 1, 1, 1, TEAM_BLUE) && wrong[0] == 1, "size mismatch changes nothing");
	}

	// BV2 colour-mask recolour, unchanged
	{
		const float red[3] = {1, 0, 0}, green[3] = {0, 1, 0}, blue[3] = {0, 0, 1};
		std::vector<unsigned char> rgb = {255, 0, 0, 0, 0, 0, 128, 128, 0};
		paintLayers(rgb, 3, red, green, blue);
		check(rgb[0] == 255 && rgb[1] == 0 && rgb[2] == 0, "pure red keeps layer 1");
		check(rgb[3] == 0 && rgb[4] == 0 && rgb[5] == 0, "black stays black");
		check(rgb[6] == 127 && rgb[7] == 127 && rgb[8] == 0, "half and half mixes layers 1 and 2");
		const float white[3] = {1, 1, 1};
		std::vector<unsigned char> one = {0, 0, 200};
		paintLayers(one, 3, red, green, white);
		check(one[0] == 255 && one[1] == 255 && one[2] == 255, "blue weight takes layer 3");
	}

	if (!g_fail)
		std::printf("ok: skins\n");
	return g_fail;
}
