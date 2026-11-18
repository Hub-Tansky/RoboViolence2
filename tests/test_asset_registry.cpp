// ROBO-232: the asset registry resolves every base pack id to main/<file>, and an id the manifest lacks is reported
// and resolves to a path no loader opens. ROBO-234: packs stack, an id resolves to the topmost pack holding its file.
#include "AssetRegistry.h"
#include "MapThemes.h"

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>

static int g_fail = 0;

static void check(bool ok, const char* what)
{
	if (!ok)
	{
		std::printf("FAIL: %s\n", what);
		g_fail = 1;
	}
}

static void touch(const std::string& path, const char* text = "x")
{
	std::filesystem::create_directories(std::filesystem::path(path).parent_path());
	std::ofstream(path) << text;
}

static bool same(const char* got, const std::string& want, const char* what)
{
	if (want == got)
		return true;
	std::printf("FAIL: %s: got %s, want %s\n", what, got, want.c_str());
	g_fail = 1;
	return false;
}

// ADR 0069: base pack (compiled-in ids), the BV2 data (no pack.tsv, base file names) and a pack with its own pack.tsv
static void packStacking(const std::string& scratch)
{
	const std::string dir = scratch + "/asset-packs/";
	std::filesystem::remove_all(dir);
	const std::string base = dir + "base/", bv2 = dir + "bv2/", mod = dir + "mod/";
	touch(base + "main/models/Rocket.DKO");
	touch(base + "main/sounds/Siren.WAV");
	touch(base + "main/pack.tsv", "id\tfile\nmodels/Rocket\tmodels/Wrong.DKO\n"); // the base pack's own pack.tsv is not read
	touch(bv2 + "main/models/Rocket.DKO");
	touch(mod + "main/models/Shiny.DKO");
	touch(mod + "main/models/Extra.DKO");
	touch(mod + "main/pack.tsv", "# a pack's manifest\nid\tfile\ttype\nmodels/Rocket\tmodels/Shiny.DKO\tmodel\n"
								 "models/Extra\tmodels/Extra.DKO\tmodel\nsounds/Siren\tsounds/Gone.wav\tsound\n");

	char* before = bv2::assetPath("models/Grenade");
	bv2::setAssetPacks({base, base});
	same(bv2::assetPath("sounds/Siren"), "main/sounds/Siren.WAV", "base pack alone: relative path");
	same(bv2::assetPath("models/Rocket"), "main/models/Rocket.DKO", "base pack alone: compiled-in file, not its pack.tsv");

	bv2::setAssetPacks({bv2, base});
	same(bv2::assetPath("models/Rocket"), "main/models/Rocket.DKO", "BV2 data holds Rocket: top pack, relative");
	same(bv2::assetPath("sounds/Siren"), "main/sounds/Siren.WAV",
		 "BV2 data lacks Siren and the base file is a generated placeholder: it stays missing, as without stacking");
	same(bv2::assetPath("fonts/babo"), "main/fonts/babo.tga", "no pack holds the file: the old missing path");
	check(!bv2::assetKnown("models/Extra"), "an id only in an unstacked pack is known");

	bv2::setAssetPacks({mod, bv2, base});
	same(bv2::assetPath("models/Rocket"), "main/models/Shiny.DKO", "a pack.tsv overrides an id's file");
	same(bv2::assetPath("models/Extra"), "main/models/Extra.DKO", "a pack.tsv adds an id");
	check(bv2::assetKnown("models/Extra"), "an id a stacked pack adds is not known");
	same(bv2::assetPath("sounds/Siren"), "main/sounds/Siren.WAV", "a placeholder stays hidden under the BV2 data");
	touch(bv2 + "main/sounds/Siren.WAV");
	bv2::setAssetPacks({mod, bv2, base});
	same(bv2::assetPath("sounds/Siren"), bv2 + "main/sounds/Siren.WAV", "a pack.tsv file that is missing falls through");

	bv2::setAssetPacks({mod, base});
	same(bv2::assetPath("models/Grenade"), "main/models/Grenade.DKO", "neither pack holds the file");
	touch(base + "main/models/Grenade.DKO");
	bv2::setAssetPacks({mod, base});
	same(bv2::assetPath("models/Grenade"), base + "main/models/Grenade.DKO", "a pack with pack.tsv: placeholders show through");
	check(std::strcmp(before, "main/models/Grenade.DKO") == 0, "a path handed out before stacking changed");
	check(bv2::assetUnknownCount() == 1, "stacking counted a known id as unknown");
}

int main(int argc, char** argv)
{
	check(bv2::assetCount() >= 200, "the compiled-in manifest has fewer than 200 ids");
	check(std::strcmp(bv2::assetPath("models/Rocket"), "main/models/Rocket.DKO") == 0, "models/Rocket");
	check(std::strcmp(bv2::assetPath("fonts/babo"), "main/fonts/babo.tga") == 0, "fonts/babo");
	check(std::strcmp(bv2::assetPath(CString("sounds/%s", "Siren")), "main/sounds/Siren.WAV") == 0,
		  "sounds/Siren keeps the file's own case");
	check(bv2::assetKnown("sounds/Menu"), "a row with source missing is still an id");
	check(bv2::assetUnknownCount() == 0, "a known id counted as unknown");

	// ROBO-243: Map builds theme texture ids at runtime; every theme x texture must be a manifest id
	for (int t = 0; t < bv2::THEME_NAME_COUNT; ++t)
		for (const char* tex : bv2::THEME_TEXTURES)
		{
			const std::string id = bv2::themeTextureId(bv2::THEME_NAMES[t], tex);
			if (!bv2::assetKnown(id.c_str()))
				std::printf("FAIL: theme texture %s is not in the manifest\n", id.c_str()), g_fail = 1;
		}

	check(!bv2::assetKnown("models/NoSuchModel"), "an unknown id is known");
	char* first = bv2::assetPath("models/NoSuchModel");
	check(std::strcmp(first, "main/models/NoSuchModel") == 0, "an unknown id does not resolve to main/<id>");
	check(bv2::assetPath("models/NoSuchModel") == first, "an unknown id's path is not stable");
	check(bv2::assetUnknownCount() == 1, "an unknown id is not counted once");
	check(!bv2::assetKnown(nullptr) && !bv2::assetKnown("models/rocket"), "ids are exact (no null, no case folding)");

	if (argc > 1)
		packStacking(argv[1]);
	else
		std::printf("FAIL: usage: test_asset_registry <scratch dir>\n"), g_fail = 1;

	if (!g_fail)
		std::printf("ok: %d ids, unknown ids reported, packs stack\n", bv2::assetCount());
	return g_fail;
}
