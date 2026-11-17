// ROBO-232: asset id to file, from the base pack manifest compiled into the program (BasePackIds.cpp, generated
// from content/base-pack.tsv by game/CMakeLists.txt), so the original BV2 data, which has no pack.tsv, keeps working.
#include "AssetRegistry.h"

#include <cstdio>
#include <map>
#include <string>

namespace bv2
{
	extern const char* const kBasePackIds[][2];
	extern const int kBasePackIdCount;
}

namespace
{
	const char* const DATA_FOLDER = "main/";

	struct Registry
	{
		std::map<std::string, std::string> paths; // id -> "main/<file>"; std::map keeps value pointers stable
		std::map<std::string, std::string> unknown; // id -> "main/<id>", reported once
		Registry()
		{
			for (int i = 0; i < bv2::kBasePackIdCount; ++i)
				paths[bv2::kBasePackIds[i][0]] = std::string(DATA_FOLDER) + bv2::kBasePackIds[i][1];
		}
	};

	Registry& registry()
	{
		static Registry r; // built on first use: game globals load assets from their constructors
		return r;
	}
}

namespace bv2
{
	char* assetPath(const char* id)
	{
		Registry& r = registry();
		const std::string key = id ? id : "";
		auto it = r.paths.find(key);
		if (it != r.paths.end())
			return const_cast<char*>(it->second.c_str());
		auto u = r.unknown.find(key);
		if (u == r.unknown.end())
		{
			fprintf(stderr, "asset: unknown id \"%s\" (not in content/base-pack.tsv)\n", key.c_str());
			u = r.unknown.emplace(key, std::string(DATA_FOLDER) + key).first;
		}
		return const_cast<char*>(u->second.c_str());
	}

	char* assetPath(const CString& id)
	{
		return assetPath(id.s);
	}

	bool assetKnown(const char* id)
	{
		return id && registry().paths.count(id) != 0;
	}

	int assetCount()
	{
		return (int)registry().paths.size();
	}

	int assetUnknownCount()
	{
		return (int)registry().unknown.size();
	}
}
