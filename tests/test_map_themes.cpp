// ROBO-259: the 23 theme ids map files store resolve to four map looks; unknown ids get the Wasteland fallback
#include "MapThemes.h"

#include <cstdio>
#include <cstring>

static int g_fail = 0;

static void check(bool ok, const char* what)
{
	if (!ok)
	{
		std::printf("FAIL: %s\n", what);
		g_fail = 1;
	}
}

static int themeId(const char* name)
{
	for (int t = 0; t < bv2::THEME_NAME_COUNT; ++t)
		if (std::strcmp(bv2::THEME_NAMES[t], name) == 0)
			return t;
	std::printf("FAIL: no theme id %s\n", name);
	g_fail = 1;
	return -1;
}

int main()
{
	struct
	{
		const char* look;
		bv2::MapLook value;
		const char* ids[12];
	} table[] = {
		{"Scrap Deck", bv2::LOOK_SCRAP_DECK, {"grass", "metal", "core", "modern", "city", "street", "orange", "road", "real", "rainy", "tropical", "animal"}},
		{"Wasteland", bv2::LOOK_WASTELAND, {"sand", "savana", "grain", "soft", "rock"}},
		{"Foundry", bv2::LOOK_FOUNDRY, {"lava", "medieval", "wooden"}},
		{"Winter Coming", bv2::LOOK_WINTER_COMING, {"frozen", "snow", "winter"}},
	};

	// The owner's alias table (2026-11-24), id by id, and every id is in it exactly once
	int covered = 0;
	for (const auto& row : table)
		for (const char* id : row.ids)
		{
			if (!id)
				break;
			++covered;
			if (bv2::themeLook(themeId(id)) != row.value)
				std::printf("FAIL: %s is not %s\n", id, row.look), g_fail = 1;
		}
	check(covered == bv2::THEME_NAME_COUNT, "the alias table covers all 23 theme ids");

	// Unknown ids (newer or broken maps) fall back to Wasteland
	check(bv2::LOOK_FALLBACK == bv2::LOOK_WASTELAND, "the fallback look is Wasteland");
	check(bv2::themeLook(-1) == bv2::LOOK_WASTELAND, "id -1 falls back to Wasteland");
	check(bv2::themeLook(bv2::THEME_NAME_COUNT) == bv2::LOOK_WASTELAND, "id 23 falls back to Wasteland");
	check(bv2::themeLook(255) == bv2::LOOK_WASTELAND, "id 255 falls back to Wasteland");

	// Folders and names
	const char* folders[] = {"wasteland", "foundry", "winter-coming", "scrap-deck"};
	const char* labels[] = {"Wasteland", "Foundry", "Winter Coming", "Scrap Deck"};
	for (int l = 0; l < bv2::LOOK_COUNT; ++l)
	{
		check(std::strcmp(bv2::LOOK_FOLDERS[l], folders[l]) == 0, "look folder name");
		check(std::strcmp(bv2::LOOK_LABELS[l], labels[l]) == 0, "look label");
	}
	check(bv2::themeTextureId(bv2::LOOK_FOLDERS[bv2::LOOK_WINTER_COMING], "tex_floor") ==
			  "textures/themes/winter-coming/tex_floor",
		  "texture id uses the look folder");

	// Weather per look (owner 2026-11-24): Wasteland wind, Foundry lava sound, Winter Coming snow, Scrap Deck none
	check(bv2::lookWeather(bv2::themeLook(themeId("rock"))) == bv2::LOOK_WEATHER_SANDSTORM, "Wasteland has wind");
	check(bv2::lookWeather(bv2::themeLook(themeId("wooden"))) == bv2::LOOK_WEATHER_LAVA, "Foundry has the lava sound");
	check(bv2::lookWeather(bv2::themeLook(themeId("frozen"))) == bv2::LOOK_WEATHER_SNOW, "Winter Coming has snow");
	check(bv2::lookWeather(bv2::themeLook(themeId("rainy"))) == bv2::LOOK_WEATHER_NONE, "Scrap Deck has none (no rain)");
	check(bv2::lookWeather(bv2::themeLook(99)) == bv2::LOOK_WEATHER_SANDSTORM, "an unknown id gets Wasteland's wind");

	// The editor cycles exactly the four looks and writes an old id that resolves back to the same look
	bv2::MapLook look = bv2::LOOK_WASTELAND;
	for (int i = 0; i < bv2::LOOK_COUNT; ++i)
	{
		check(bv2::themeLook(bv2::LOOK_THEME_IDS[look]) == look, "editor id resolves to its look");
		look = bv2::nextLook(look);
	}
	check(look == bv2::LOOK_WASTELAND, "the editor cycle has four steps");

	if (!g_fail)
		std::printf("map themes: all checks passed\n");
	return g_fail;
}
