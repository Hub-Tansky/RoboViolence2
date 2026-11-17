// ROBO-232: the asset registry resolves every base pack id to main/<file>, and an id the manifest lacks is reported
// and resolves to a path no loader opens.
#include "AssetRegistry.h"

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

int main()
{
	check(bv2::assetCount() >= 200, "the compiled-in manifest has fewer than 200 ids");
	check(std::strcmp(bv2::assetPath("models/Rocket"), "main/models/Rocket.DKO") == 0, "models/Rocket");
	check(std::strcmp(bv2::assetPath("fonts/babo"), "main/fonts/babo.tga") == 0, "fonts/babo");
	check(std::strcmp(bv2::assetPath(CString("sounds/%s", "Siren")), "main/sounds/Siren.WAV") == 0,
		  "sounds/Siren keeps the file's own case");
	check(bv2::assetKnown("sounds/Menu"), "a row with source missing is still an id");
	check(bv2::assetUnknownCount() == 0, "a known id counted as unknown");

	check(!bv2::assetKnown("models/NoSuchModel"), "an unknown id is known");
	char* first = bv2::assetPath("models/NoSuchModel");
	check(std::strcmp(first, "main/models/NoSuchModel") == 0, "an unknown id does not resolve to main/<id>");
	check(bv2::assetPath("models/NoSuchModel") == first, "an unknown id's path is not stable");
	check(bv2::assetUnknownCount() == 1, "an unknown id is not counted once");
	check(!bv2::assetKnown(nullptr) && !bv2::assetKnown("models/rocket"), "ids are exact (no null, no case folding)");

	if (!g_fail)
		std::printf("ok: %d ids, unknown ids reported\n", bv2::assetCount());
	return g_fail;
}
