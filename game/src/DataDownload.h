// Installs the original BaboViolent 2 data downloaded on first run: checks it against known hashes, then moves it
// to originalDataDir() (ADR 0040).
#pragma once

#include <string>
#include <vector>

namespace bv2
{
	struct AssetHash
	{
		const char * sha256; // lowercase hex
		const char * path;   // relative to the folder holding main/, e.g. "main/textures/a.tga"
	};

	// The Content/main entries of tools/original-assets.sha256, generated at build time
	const std::vector<AssetHash> & originalMainHashes();

	// Lowercase hex SHA-256 of a file; "" when it can't be read
	std::string sha256File(const std::string & file);

	// When every listed file under dir (a folder holding main/) has its hash, renames dir to originalDataDir(),
	// replacing an old one. Returns "" on success, else what failed (the first wrong or missing file).
	std::string installOriginalData(const std::string & dir, const std::vector<AssetHash> & hashes);
}
