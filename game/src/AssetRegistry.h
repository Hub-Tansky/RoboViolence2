// Asset ids (ROBO-232, ADR 0065): game code asks for an asset by its id in the base pack manifest
// (content/base-pack.tsv), and the registry turns it into the path the dk* loaders open.
#pragma once

#include "CString.h"

#include <string>
#include <vector>

namespace bv2
{
	// The path of an id's file, e.g. assetPath("models/Rocket") is "main/models/Rocket.DKO" (see setAssetPacks).
	// An id the manifest lacks is reported on stderr once and gives "main/<id>", which no loader opens, so the game
	// behaves as for a missing file. The pointer stays valid for the life of the program.
	char* assetPath(const char* id);
	char* assetPath(const CString& id);

	// Pack stacking (ADR 0069): roots are folders holding main/, top first; the last is the base pack, whose ids are
	// the compiled-in manifest. An id resolves to the first pack holding its file: a pack's main/pack.tsv names its
	// files (and may add ids); a pack without one (the BV2 data) uses the base pack's file names. The top pack is the
	// working directory, so its paths stay "main/<file>"; lower packs give "<root>main/<file>". When no pack holds
	// the file, the path is "main/<file>" as before stacking. Until this is called every id gives "main/<file>".
	void setAssetPacks(const std::vector<std::string>& roots);

	// True when the base manifest or a stacked pack's pack.tsv has this id
	bool assetKnown(const char* id);

	// Number of ids the registry holds, and how many unknown ids were asked for (tests)
	int assetCount();
	int assetUnknownCount();
}
