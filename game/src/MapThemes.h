// Map theme folders and the textures each one holds (ROBO-243). Map.cpp builds a theme texture's asset id from them,
// and tests/test_asset_registry.cpp checks every such id is in the base pack manifest.
#ifndef BV2_MAP_THEMES_H
#define BV2_MAP_THEMES_H

#include <string>

namespace bv2
{
	// Folder under textures/themes/ by theme number (THEME_GRASS = 0 ... THEME_WOODEN = 22 in Map.h)
	const char* const THEME_NAMES[] = {"grass", "snow", "sand", "city", "modern", "lava", "animal", "orange", "core",
									   "frozen", "grain", "medieval", "metal", "rainy", "real", "road", "rock", "savana", "soft", "street", "tropical",
									   "winter", "wooden"};
	const int THEME_NAME_COUNT = (int)(sizeof(THEME_NAMES) / sizeof(THEME_NAMES[0]));

	// The textures Map loads from a theme folder
	const char* const THEME_TEXTURES[] = {"tex_floor", "tex_floor_dirt", "tex_wall_center"};

	// Asset id of a theme texture: textures/themes/<theme>/<texture>
	inline std::string themeTextureId(const char* theme, const char* texture)
	{
		return std::string("textures/themes/") + theme + "/" + texture;
	}
}

#endif
