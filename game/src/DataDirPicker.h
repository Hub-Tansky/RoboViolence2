// Client start-up: finds the player's BaboViolent 2 data, or offers to download it on first run (ADR 0016, ADR 0040).
#pragma once

namespace bv2
{
	// Unless BV2_DATA_DIR held data or argv has --placeholders: the saved folder, else a detected one, else (unless
	// the player chose placeholders before) a box offering to download the original data into originalDataDir().
	// A folder found or installed becomes the data root and is saved. Call after pathsInit().
	void chooseDataDir(int argc, char ** argv);
}
