// The art pack download without SDL (ROBO-260, ADR ROBO-260)
#include "ArtPack.h"
#include "DataDownload.h"

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

// Pinned per release by the art drop (ADR ROBO-260): Hub-Tansky/rv2-art release, built by its tools/build-release.sh. URL form:
// https://github.com/Hub-Tansky/rv2-art/releases/download/rv2-art-<version>/rv2-art-<version>.tar.gz
#define RV2_ART_PACK_VERSION "1"
#define RV2_ART_PACK_URL "https://github.com/Hub-Tansky/rv2-art/releases/download/rv2-art-1/rv2-art-1.tar.gz"
#define RV2_ART_PACK_SHA256 "bbad5245969f1d8966ca67a6c56aa116ccebba9dc87fe9e263d53d7109a3f1b6"
#define RV2_ART_PACK_BYTES 8450732.0

namespace
{
	std::string tool(const char* name)
	{
#ifdef BV2_PLATFORM_WINDOWS
		const char* root = std::getenv("SystemRoot");
		return std::string(root ? root : "C:\\Windows") + "\\System32\\" + name + ".exe";
#else
		return std::string("/usr/bin/") + name;
#endif
	}
}

namespace bv2
{
	const char* const ART_PACK_VERSION = RV2_ART_PACK_VERSION;
	const char* const ART_PACK_URL = RV2_ART_PACK_URL;
	const char* const ART_PACK_SHA256 = RV2_ART_PACK_SHA256;
	const double ART_PACK_BYTES = RV2_ART_PACK_BYTES;

	bool artPackInstalled(const std::string& dir, const std::string& version)
	{
		std::error_code ec;
		if (version.empty() || !fs::is_regular_file(fs::path(dir) / "main" / "pack.tsv", ec))
			return false;
		std::ifstream in(fs::path(dir) / "VERSION");
		std::string have;
		std::getline(in, have);
		return have == version;
	}

	bool artDownloadOff(int argc, const char* const* argv, const std::string& prefDir)
	{
		const fs::path off = fs::path(prefDir) / "art-download-off";
		for (int i = 1; i < argc; ++i)
			if (std::strcmp(argv[i], "--placeholders") == 0)
				return true;
		for (int i = 1; i < argc; ++i)
			if (std::strcmp(argv[i], "--no-art-download") == 0)
			{
				std::ofstream(off) << "the art pack download is off (--no-art-download)\n";
				return true;
			}
		std::error_code ec;
		return fs::exists(off, ec);
	}

	std::vector<std::string> artDownloadArgs(const std::string& url, const std::string& file)
	{
		return {tool("curl"), "-fsSL", "--retry", "2", "-o", file, url};
	}

	std::vector<std::string> artExtractArgs(const std::string& file, const std::string& dir)
	{
		return {tool("tar"), "-xzf", file, "-C", dir};
	}

	bool artArchiveValid(const std::string& archive, const std::string& sha256)
	{
		return !sha256.empty() && sha256File(archive) == sha256;
	}

	std::string installArtPack(const std::string& dir, const std::string& version, const std::string& target)
	{
		std::error_code ec;
		if (!fs::is_regular_file(fs::path(dir) / "main" / "pack.tsv", ec))
			return "the archive has no main/pack.tsv";
		std::ofstream(fs::path(dir) / "VERSION") << version << "\n";
		return replaceDir(dir, target);
	}
}
