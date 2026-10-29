// First-run data download, the part without SDL: hash check and install (ADR 0040).
#include "DataDownload.h"
#include "Paths.h"

#include <sodium.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>

#include "OriginalHashes.h"

namespace fs = std::filesystem;

namespace
{
	// Must match tools/original-assets.sha256 (test_data_download checks the layout, the hashes check the data)
#define BV2_UPSTREAM_COMMIT "7171c84d9b4d02bb74e846d5617fbb527bfba358"
	const char * ZIP_MAIN = "BaboViolent2-" BV2_UPSTREAM_COMMIT "/BaboViolent2/Content/main";

	std::string tool(const char * name)
	{
#ifdef BV2_PLATFORM_WINDOWS
		const char * root = std::getenv("SystemRoot");
		return std::string(root ? root : "C:\\Windows") + "\\System32\\" + name + ".exe";
#else
		return std::string("/usr/bin/") + name;
#endif
	}
}

namespace bv2
{
	const char * const ORIGINAL_ZIP_URL = "https://github.com/Daivuk/BaboViolent2/archive/" BV2_UPSTREAM_COMMIT ".zip";

	std::vector<std::string> downloadArgs(const std::string & zip)
	{
		return { tool("curl"), "-fsSL", "--retry", "2", "-o", zip, ORIGINAL_ZIP_URL };
	}

	std::vector<std::string> extractArgs(const std::string & zip, const std::string & dir)
	{
#if defined(BV2_PLATFORM_WINDOWS) || defined(BV2_PLATFORM_MACOS)
		return { tool("tar"), "-xf", zip, "-C", dir, ZIP_MAIN };
#else
		return { tool("unzip"), "-q", "-o", zip, std::string(ZIP_MAIN) + "/*", "-d", dir };
#endif
	}

	std::string extractedDataDir(const std::string & dir)
	{
		return (fs::path(dir) / ZIP_MAIN).parent_path().string();
	}

	const std::vector<AssetHash> & originalMainHashes()
	{
		static const std::vector<AssetHash> hashes(std::begin(g_originalMainHashes), std::end(g_originalMainHashes));
		return hashes;
	}

	std::string sha256File(const std::string & file)
	{
		std::ifstream in(fs::path(file), std::ios::binary);
		if (!in)
			return "";
		crypto_hash_sha256_state state;
		crypto_hash_sha256_init(&state);
		char buf[65536];
		while (in.read(buf, sizeof(buf)) || in.gcount() > 0)
			crypto_hash_sha256_update(&state, (const unsigned char *)buf, (unsigned long long)in.gcount());
		unsigned char digest[crypto_hash_sha256_BYTES];
		crypto_hash_sha256_final(&state, digest);
		char hex[crypto_hash_sha256_BYTES * 2 + 1];
		sodium_bin2hex(hex, sizeof(hex), digest, sizeof(digest));
		return hex;
	}

	std::string installOriginalData(const std::string & dir, const std::vector<AssetHash> & hashes)
	{
		for (size_t i = 0; i < hashes.size(); ++i)
			if (sha256File((fs::path(dir) / hashes[i].path).string()) != hashes[i].sha256)
				return hashes[i].path;
		std::error_code ec;
		const fs::path target(originalDataDir()), old(originalDataDir() + ".old");
		fs::remove_all(old, ec);
		fs::rename(target, old, ec); // fails harmlessly when there is no old install
		fs::rename(dir, target, ec);
		if (ec)
		{
			std::error_code ignored;
			fs::rename(old, target, ignored);
			return "move to " + target.string() + ": " + ec.message();
		}
		fs::remove_all(old, ec);
		return "";
	}
}
