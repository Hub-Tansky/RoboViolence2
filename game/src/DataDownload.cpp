// First-run data download, the part without SDL: hash check and install (ADR 0040).
#include "DataDownload.h"
#include "Paths.h"

#include <sodium.h>

#include <filesystem>
#include <fstream>
#include <iterator>

#include "OriginalHashes.h"

namespace fs = std::filesystem;

namespace bv2
{
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
		const fs::path target(originalDataDir());
		fs::remove_all(target, ec);
		fs::create_directories(target.parent_path(), ec);
		fs::rename(dir, target, ec);
		return ec ? "move to " + target.string() + ": " + ec.message() : "";
	}
}
