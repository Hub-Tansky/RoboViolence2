// The first-run download of the original BaboViolent 2 data without SDL: what to fetch, the commands, the hash check
// and the install (ADR 0040).
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

	// The pinned upstream commit, whose Content/main matches tools/original-assets.sha256, and its zip
	extern const char * const ORIGINAL_COMMIT;
	extern const char * const ORIGINAL_ZIP_URL;
	// The browser fallback: master.zip, which unzips to BaboViolent2-master, a folder detectDataDir looks for
	extern const char * const BROWSER_ZIP_URL;

	// curl's argv: download ORIGINAL_ZIP_URL to zip
	std::vector<std::string> downloadArgs(const std::string & zip);
	// tar's (bsdtar, Windows and macOS) or unzip's (Linux) argv: extract only the zip's main/ into dir
	std::vector<std::string> extractArgs(const std::string & zip, const std::string & dir);
	// The folder holding main/ after extractArgs ran into dir
	std::string extractedDataDir(const std::string & dir);

	// The Content/main entries of tools/original-assets.sha256, generated at build time
	const std::vector<AssetHash> & originalMainHashes();

	// Lowercase hex SHA-256 of a file; "" when it can't be read
	std::string sha256File(const std::string & file);

	// The first listed file under dir (a folder holding main/) that is missing or has another hash; "" when all match
	std::string firstMismatch(const std::string & dir, const std::vector<AssetHash> & hashes);

	// Renames from to to, replacing an old to, which is kept if the rename fails. "" on success, else the error.
	std::string replaceDir(const std::string & from, const std::string & to);
}
