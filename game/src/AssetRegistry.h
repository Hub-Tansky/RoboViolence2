// Asset ids (ROBO-232, ADR 0065): game code asks for an asset by its id in the base pack manifest
// (content/base-pack.tsv), and the registry turns it into the path the dk* loaders open.
#pragma once

#include "CString.h"

namespace bv2
{
	// "main/<file>" for an id of the manifest, e.g. assetPath("models/Rocket") is "main/models/Rocket.DKO".
	// An id the manifest lacks is reported on stderr once and gives "main/<id>", which no loader opens, so the game
	// behaves as for a missing file. The pointer stays valid for the life of the program.
	char* assetPath(const char* id);
	char* assetPath(const CString& id);

	// True when the manifest has this id
	bool assetKnown(const char* id);

	// Number of ids the registry holds, and how many unknown ids were asked for (tests)
	int assetCount();
	int assetUnknownCount();
}
