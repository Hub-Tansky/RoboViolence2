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
#define RV2_ART_PACK_VERSION "8"
#define RV2_ART_PACK_URL "https://github.com/Hub-Tansky/rv2-art/releases/download/rv2-art-8/rv2-art-8.tar.gz"
#define RV2_ART_PACK_SHA256 "756a59c0004b58085e1f56a655d2d00c81c5cc189567d221f2c57487d8148780"
#define RV2_ART_PACK_BYTES 16935700.0

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

	PackStack assetPackStack(const std::string& buildPacks, bool useBuildPacks, const std::string& artDir,
							 const std::string& version, const std::string& dataRoot, const std::string& packageRoot)
	{
		PackStack stack;
		std::error_code ec;
		const bool build = useBuildPacks && !buildPacks.empty();
		if (build && fs::is_regular_file(fs::path(buildPacks) / "url" / "main" / "pack.tsv", ec))
			stack.roots.push_back((fs::path(buildPacks) / "url").generic_string());
		if (build && artPackInstalled((fs::path(buildPacks) / "robo_art").generic_string(), version))
		{
			stack.roots.push_back((fs::path(buildPacks) / "robo_art").generic_string());
			stack.buildArt = true;
		}
		else if (artPackInstalled(artDir, version))
			stack.roots.push_back(artDir);
		stack.cwdPack = stack.roots.size(); // the data root stays the working directory
		stack.roots.push_back(dataRoot);
		stack.roots.push_back(packageRoot);
		return stack;
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

	std::string artArchivePath(const std::string& partial)
	{
		return (fs::path(partial) / "rv2-art.tar.gz").string();
	}

	std::string fetchArtPack(const std::string& partial, const std::string& url, const std::string& sha256,
							 const std::string& version, const std::string& target, const ArtRunner& run,
							 const std::function<bool()>& stopped)
	{
		const fs::path dir(partial), archive(artArchivePath(partial)), pack = fs::path(partial) / "pack";
		std::error_code ec;
		fs::remove_all(dir, ec);
		fs::create_directories(pack, ec);
		std::string error;
		const bool fetched = run(artDownloadArgs(url, archive.string()));
		if (stopped())
			error = "stopped";
		else if (!fetched || !artArchiveValid(archive.string(), sha256))
			error = "the download failed or its SHA-256 is not the pinned one";
		else if (!run(artExtractArgs(archive.string(), pack.string())) || stopped())
			error = stopped() ? "stopped" : "unpacking failed";
		else
			error = installArtPack(pack.string(), version, target);
		fs::remove_all(dir, ec);
		return error;
	}

	std::string installArtPack(const std::string& dir, const std::string& version, const std::string& target)
	{
		std::error_code ec;
		const fs::path main = fs::path(dir) / "main";
		if (!fs::is_regular_file(main / "pack.tsv", ec))
			return "the archive has no main/pack.tsv";
		{ // closed before the rename: Windows can't move a folder holding an open file
			std::ifstream manifest(main / "pack.tsv");
			for (std::string line; std::getline(manifest, line);)
			{
				const size_t tab = line.find('\t');
				if (line.empty() || line[0] == '#' || tab == std::string::npos || line.compare(0, tab, "id") == 0)
					continue;
				const std::string file = line.substr(tab + 1, line.find('\t', tab + 1) - tab - 1);
				if (!fs::is_regular_file(main / file, ec)) // a short unpack (disk full) must not install
					return "the archive lacks " + file;
			}
		}
		std::ofstream(fs::path(dir) / "VERSION") << version << "\n";
		return replaceDir(dir, target);
	}
}
