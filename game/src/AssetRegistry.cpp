// ROBO-232: asset id to file, from the base pack manifest compiled into the program (BasePackIds.cpp, generated
// from content/base-pack.tsv by game/CMakeLists.txt), so the original BV2 data, which has no pack.tsv, keeps working.
// ROBO-234 (ADR 0069): packs stack; an id resolves to the first pack, from the top, that holds its file.
#include "AssetRegistry.h"
#include "BasePackIds.h"

#include <cstdio>
#include <deque>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

namespace
{
	const char* const DATA_FOLDER = "main/";

	struct Pack
	{
		std::string root; // folder holding main/, with a trailing '/'
		bool hasManifest = false; // main/pack.tsv was read; without one, the pack uses the base pack's file names
		std::map<std::string, std::string> files; // id -> file under main/, from pack.tsv
	};

	struct Registry
	{
		std::map<std::string, std::string> base; // id -> file under main/, from the compiled-in base manifest
		std::map<std::string, bool> placeholder; // id -> its base file is a generated placeholder
		std::vector<Pack> packs; // top first; empty until setAssetPacks: every id is "main/<file>" (ADR 0066)
		std::map<std::string, const char*> resolved; // id -> path, cleared when the stack changes
		std::deque<std::string> paths; // every path handed out; never shrinks, so returned pointers stay valid
		std::map<std::string, const char*> unknown; // id -> "main/<id>", reported once
		Registry()
		{
			for (int i = 0; i < bv2::kBasePackIdCount; ++i)
			{
				base[bv2::kBasePackIds[i].id] = bv2::kBasePackIds[i].file;
				placeholder[bv2::kBasePackIds[i].id] = bv2::kBasePackIds[i].placeholder;
			}
		}
		const char* keep(const std::string& path)
		{
			paths.push_back(path);
			return paths.back().c_str();
		}
	};

	Registry& registry()
	{
		static Registry r; // built on first use: game globals load assets from their constructors
		return r;
	}

	std::string withSlash(std::string dir)
	{
		if (!dir.empty() && dir.back() != '/' && dir.back() != '\\')
			dir += '/';
		return dir;
	}

	// The id and file columns of a pack's main/pack.tsv (ADR 0065); false when the pack has none. Reads only those two
	// columns and skips malformed rows (a player's pack must not stop the game); tools/check-pack.py and the CMake
	// step fail on them instead.
	bool readManifest(const std::string& root, std::map<std::string, std::string>& files)
	{
		std::ifstream in(std::filesystem::path(root + DATA_FOLDER + "pack.tsv"));
		if (!in)
			return false;
		std::string line;
		int idCol = -1, fileCol = -1;
		while (std::getline(in, line))
		{
			if (!line.empty() && line.back() == '\r')
				line.pop_back();
			if (line.empty() || line[0] == '#')
				continue;
			std::vector<std::string> cols(1);
			for (char c : line)
			{
				if (c == '\t')
					cols.emplace_back();
				else
					cols.back().push_back(c);
			}
			if (idCol < 0)
			{
				for (int i = 0; i < (int)cols.size(); ++i)
				{
					if (cols[i] == "id")
						idCol = i;
					if (cols[i] == "file")
						fileCol = i;
				}
				if (idCol < 0 || fileCol < 0)
					return false;
				continue;
			}
			if ((int)cols.size() > fileCol && (int)cols.size() > idCol && !cols[idCol].empty() && !cols[fileCol].empty())
				files[cols[idCol]] = cols[fileCol];
		}
		return idCol >= 0;
	}

	// The pack's file for id, "" when the pack has none
	std::string packFile(const Registry& r, const Pack& pack, const std::string& id)
	{
		const std::map<std::string, std::string>& files = pack.hasManifest ? pack.files : r.base;
		auto it = files.find(id);
		return it == files.end() ? std::string() : it->second;
	}

	bool placeholderFile(const Registry& r, const std::string& id)
	{
		auto it = r.placeholder.find(id);
		return it != r.placeholder.end() && it->second;
	}

	bool fileExists(const std::string& path)
	{
		std::error_code ec;
		return std::filesystem::is_regular_file(std::filesystem::path(path), ec);
	}
}

namespace bv2
{
	void setAssetPacks(const std::vector<std::string>& roots)
	{
		Registry& r = registry();
		r.packs.clear();
		r.resolved.clear();
		std::vector<std::string> unique;
		for (const std::string& dir : roots)
		{
			const std::string root = withSlash(dir);
			bool seen = root.empty();
			for (const std::string& u : unique)
				seen = seen || u == root;
			if (!seen)
				unique.push_back(root);
		}
		for (size_t i = 0; i < unique.size(); ++i)
		{
			Pack pack;
			pack.root = unique[i];
			// the bottom pack is the base pack: its ids are the compiled-in manifest, whatever its pack.tsv holds
			if (i + 1 < unique.size())
				pack.hasManifest = readManifest(pack.root, pack.files);
			r.packs.push_back(pack);
		}
	}

	char* assetPath(const char* id)
	{
		Registry& r = registry();
		const std::string key = id ? id : "";
		auto done = r.resolved.find(key);
		if (done != r.resolved.end())
			return const_cast<char*>(done->second);

		auto base = r.base.find(key);
		const char* path = nullptr;
		if (r.packs.empty())
		{
			if (base != r.base.end())
				path = r.keep(DATA_FOLDER + base->second);
		}
		else
		{
			std::string firstFile; // the topmost pack's file name for the id, "" when no pack knows it
			bool legacyAbove = false; // a pack without pack.tsv is above the one being looked at
			for (size_t i = 0; i < r.packs.size() && !path; ++i)
			{
				if (i > 0)
					legacyAbove = legacyAbove || !r.packs[i - 1].hasManifest;
				const std::string file = packFile(r, r.packs[i], key);
				if (firstFile.empty())
					firstFile = file;
				if (file.empty() || !fileExists(r.packs[i].root + DATA_FOLDER + file))
					continue;
				// under the BV2 data (a pack without pack.tsv) a generated placeholder doesn't show through: the BV2
				// data plays as it did where it lacks a file (textures/sky); the base pack's art does show through
				if (i + 1 == r.packs.size() && legacyAbove && placeholderFile(r, key))
					continue;
				// the top pack is the working directory (pathsInit changes into the data root): keep its paths relative
				path = r.keep(i == 0 ? DATA_FOLDER + file : r.packs[i].root + DATA_FOLDER + file);
			}
			if (!path && !firstFile.empty())
			{
				// no pack has the file: the same missing path as before stacking, so loaders fail as they did
				path = r.keep(DATA_FOLDER + (base != r.base.end() ? base->second : firstFile));
			}
		}
		if (path)
		{
			r.resolved[key] = path;
			return const_cast<char*>(path);
		}

		auto u = r.unknown.find(key);
		if (u == r.unknown.end())
		{
			fprintf(stderr, "asset: unknown id \"%s\" (not in content/base-pack.tsv or a pack's pack.tsv)\n", key.c_str());
			u = r.unknown.emplace(key, r.keep(DATA_FOLDER + key)).first;
		}
		return const_cast<char*>(u->second);
	}

	char* assetPath(const CString& id)
	{
		return assetPath(id.s);
	}

	bool assetKnown(const char* id)
	{
		if (!id)
			return false;
		const Registry& r = registry();
		if (r.base.count(id))
			return true;
		for (const Pack& p : r.packs)
			if (p.hasManifest && p.files.count(id))
				return true;
		return false;
	}

	int assetCount()
	{
		return (int)registry().base.size();
	}

	int assetUnknownCount()
	{
		return (int)registry().unknown.size();
	}
}
