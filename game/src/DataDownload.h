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

	// The upstream zip of one pinned commit, whose Content/main matches tools/original-assets.sha256
	extern const char * const ORIGINAL_ZIP_URL;

	// curl's argv: download ORIGINAL_ZIP_URL to zip
	std::vector<std::string> downloadArgs(const std::string & zip);
	// tar's (bsdtar, Windows and macOS) or unzip's (Linux) argv: extract only the zip's main/ into dir
	std::vector<std::string> extractArgs(const std::string & zip, const std::string & dir);
	// The folder holding main/ after extractArgs ran into dir: the one installOriginalData takes
	std::string extractedDataDir(const std::string & dir);

	// The Content/main entries of tools/original-assets.sha256, generated at build time
	const std::vector<AssetHash> & originalMainHashes();

	// Lowercase hex SHA-256 of a file; "" when it can't be read
	std::string sha256File(const std::string & file);

	// When every listed file under dir (a folder holding main/) has its hash, renames dir to originalDataDir(),
	// replacing an old one (kept if the rename fails). Returns "" on success, else what failed.
	std::string installOriginalData(const std::string & dir, const std::vector<AssetHash> & hashes);
}
