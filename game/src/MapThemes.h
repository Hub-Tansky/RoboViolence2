// Map theme ids, the four map looks they alias, and the textures each look folder holds (ROBO-243, ROBO-259). Map.cpp builds a theme texture's asset id from them,
// and tests/test_asset_registry.cpp checks every such id is in the base pack manifest.
#ifndef BV2_MAP_THEMES_H
#define BV2_MAP_THEMES_H

#include <string>

namespace bv2
{
	// Theme ids as map files store them (THEME_GRASS = 0 ... THEME_WOODEN = 22 in Map.h). Since ROBO-259 they are aliases:
	// each one picks one of the four map looks below, and map files are never rewritten.
	const char* const THEME_NAMES[] = {"grass", "snow", "sand", "city", "modern", "lava", "animal", "orange", "core",
									   "frozen", "grain", "medieval", "metal", "rainy", "real", "road", "rock", "savana", "soft", "street", "tropical",
									   "winter", "wooden"};
	const int THEME_NAME_COUNT = (int)(sizeof(THEME_NAMES) / sizeof(THEME_NAMES[0]));

	// The four map looks (ROBO-259, owner 2026-11-24): folder under textures/themes/ and the name the game and editor show
	enum MapLook
	{
		LOOK_WASTELAND = 0,
		LOOK_FOUNDRY,
		LOOK_WINTER_COMING,
		LOOK_SCRAP_DECK,
		LOOK_COUNT
	};
	const char* const LOOK_FOLDERS[LOOK_COUNT] = {"wasteland", "foundry", "winter-coming", "scrap-deck"};
	const char* const LOOK_LABELS[LOOK_COUNT] = {"Wasteland", "Foundry", "Winter Coming", "Scrap Deck"};
	// A theme id the table does not know (a newer or broken map) gets this look
	const MapLook LOOK_FALLBACK = LOOK_WASTELAND;

	// Look for each theme id, in THEME_NAMES order
	const MapLook THEME_LOOKS[] = {
		LOOK_SCRAP_DECK, // grass
		LOOK_WINTER_COMING, // snow
		LOOK_WASTELAND, // sand
		LOOK_SCRAP_DECK, // city
		LOOK_SCRAP_DECK, // modern
		LOOK_FOUNDRY, // lava
		LOOK_SCRAP_DECK, // animal
		LOOK_SCRAP_DECK, // orange
		LOOK_SCRAP_DECK, // core
		LOOK_WINTER_COMING, // frozen
		LOOK_WASTELAND, // grain
		LOOK_FOUNDRY, // medieval
		LOOK_SCRAP_DECK, // metal
		LOOK_SCRAP_DECK, // rainy
		LOOK_SCRAP_DECK, // real
		LOOK_SCRAP_DECK, // road
		LOOK_WASTELAND, // rock
		LOOK_WASTELAND, // savana
		LOOK_WASTELAND, // soft
		LOOK_SCRAP_DECK, // street
		LOOK_SCRAP_DECK, // tropical
		LOOK_WINTER_COMING, // winter
		LOOK_FOUNDRY, // wooden
	};
	static_assert(sizeof(THEME_LOOKS) / sizeof(THEME_LOOKS[0]) == sizeof(THEME_NAMES) / sizeof(THEME_NAMES[0]),
				  "every theme id has a look");

	// Theme id the map editor writes for a look: an old id, so map files keep their format and older builds still load them
	const int LOOK_THEME_IDS[LOOK_COUNT] = {2 /* sand */, 5 /* lava */, 21 /* winter */, 0 /* grass */};

	// The look a map's theme id shows; unknown ids fall back to LOOK_FALLBACK
	inline MapLook themeLook(int themeId)
	{
		return themeId >= 0 && themeId < THEME_NAME_COUNT ? THEME_LOOKS[themeId] : LOOK_FALLBACK;
	}

	// The look after this one, for the editor's Theme button
	inline MapLook nextLook(MapLook look)
	{
		return (MapLook)((look + 1) % LOOK_COUNT);
	}

	// The textures Map loads from a theme folder, in this order: floor, floor dirt, wall
	const char* const THEME_TEXTURES[] = {"tex_floor", "tex_floor_dirt", "tex_wall_center"};

	// Asset id of a theme texture: textures/themes/<look folder>/<texture>
	inline std::string themeTextureId(const char* theme, const char* texture)
	{
		return std::string("textures/themes/") + theme + "/" + texture;
	}
}

#endif
