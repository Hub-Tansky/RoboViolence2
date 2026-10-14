// Client start-up: finds the player's BaboViolent 2 data, or asks for it on first run (Phase C step 1, ADR 0016).
#pragma once

namespace bv2
{
	// Unless BV2_DATA_DIR is set or argv has --placeholders: the saved folder, else a detected one, else the OS
	// folder dialog. A folder found or chosen becomes the data root and is saved. Call after pathsInit().
	void chooseDataDir(int argc, char ** argv);
}
