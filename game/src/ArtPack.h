// The Robo Violence 2 art pack (ROBO-260, ADR ROBO-260): a proprietary pack downloaded beside the GPL game into
// artPackDir() and stacked over the base pack and the BV2 data. This part has no SDL: what to fetch, the commands,
// the check and the install.
#pragma once

#include <string>
#include <vector>

namespace bv2
{
	// The pinned art pack of this build: its version, the URL of its .tar.gz and the archive's SHA-256. An empty
	// version means this build pins none: nothing is downloaded and no art pack is stacked.
	extern const char* const ART_PACK_VERSION;
	extern const char* const ART_PACK_URL;
	extern const char* const ART_PACK_SHA256;
	// The archive's size in bytes, for the progress line
	extern const double ART_PACK_BYTES;

	// True when dir holds main/pack.tsv and a VERSION file equal to version (and version isn't empty)
	bool artPackInstalled(const std::string& dir, const std::string& version);

	// True when the player turned the download off: --no-art-download in argv (remembered) or the remembered file
	// <prefDir>/art-download-off. Writes that file when the flag is given.
	bool artDownloadOff(int argc, const char* const* argv, const std::string& prefDir);

	// curl's argv: url to file
	std::vector<std::string> artDownloadArgs(const std::string& url, const std::string& file);
	// tar's argv: unpack the .tar.gz file into dir (the archive holds main/)
	std::vector<std::string> artExtractArgs(const std::string& file, const std::string& dir);

	// True when the downloaded archive has the pinned SHA-256; checked before unpacking
	bool artArchiveValid(const std::string& archive, const std::string& sha256);

	// Writes VERSION into dir (the unpacked pack, holding main/pack.tsv) and moves it to target. "" on success, else
	// the reason.
	std::string installArtPack(const std::string& dir, const std::string& version, const std::string& target);
}
