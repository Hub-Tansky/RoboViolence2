/*
	Copyright 2012 bitHeads inc.

	This file is part of the BaboViolent 2 source code.

	The BaboViolent 2 source code is free software: you can redistribute it and/or 
	modify it under the terms of the GNU General Public License as published by the 
	Free Software Foundation, either version 3 of the License, or (at your option) 
	any later version.

	The BaboViolent 2 source code is distributed in the hope that it will be useful, 
	but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
	FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License along with the 
	BaboViolent 2 source code. If not, see http://www.gnu.org/licenses/.
*/

#include "Paths.h"

#include "platform.h"
#include <sqlite3.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>

#ifdef BV2_PLATFORM_WINDOWS
	#include <windows.h>
#elif defined(BV2_PLATFORM_MACOS)
	#include <mach-o/dyld.h>
#else
	#include <unistd.h>
#endif

#include "dksvar.h"

namespace fs = std::filesystem;

namespace
{
	std::string g_dataRoot;
	std::string g_packageRoot;
	std::string g_prefDir;

	std::string getEnv(const char * name)
	{
		const char * v = std::getenv(name);
		return v ? std::string(v) : std::string();
	}

	std::string exeDir(const char * argv0)
	{
		std::error_code ec;
#ifdef BV2_PLATFORM_WINDOWS
		wchar_t buf[4096];
		DWORD n = GetModuleFileNameW(NULL, buf, 4096);
		if (n > 0 && n < 4096)
			return fs::path(buf).parent_path().generic_u8string().c_str();
#elif defined(BV2_PLATFORM_MACOS)
		char buf[4096];
		uint32_t size = sizeof(buf);
		if (_NSGetExecutablePath(buf, &size) == 0)
			return fs::weakly_canonical(fs::path(buf), ec).parent_path().string();
#else
		char buf[4096];
		ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
		if (n > 0)
		{
			buf[n] = 0;
			return fs::path(buf).parent_path().string();
		}
#endif
		if (argv0 && argv0[0])
			return fs::weakly_canonical(fs::path(argv0), ec).parent_path().string();
		return std::string();
	}

	bool hasMain(const fs::path & dir)
	{
		std::error_code ec;
		return !dir.empty() && fs::is_directory(dir / "main", ec);
	}

	// The first folder holding sub (e.g. "main/textures") among dir, its parent when dir is main/, dir/Content and
	// dir/BaboViolent2/Content (the upstream repository); "" when none
	fs::path firstRootWith(const std::string & dir, const char * sub)
	{
		if (dir.empty())
			return fs::path();
		fs::path p(dir);
		if (p.filename().empty())
			p = p.parent_path(); // trailing slash
		std::vector<fs::path> tries(1, p);
		if (p.filename() == "main")
			tries.push_back(p.parent_path());
		tries.push_back(p / "Content");
		tries.push_back(p / "BaboViolent2" / "Content");
		for (size_t i = 0; i < tries.size(); ++i)
		{
			std::error_code ec;
			if (fs::is_directory(tries[i] / sub, ec))
				return tries[i];
		}
		return fs::path();
	}

	// Canonical form with a trailing '/'
	std::string rootString(const fs::path & dir)
	{
		std::error_code ec;
		std::string s = fs::weakly_canonical(dir, ec).generic_string();
		if (!s.empty() && s.back() != '/')
			s += '/';
		return s;
	}

	std::string computePrefDir()
	{
		std::string over = getEnv("BV2_PREF_DIR");
		fs::path base;
		if (!over.empty())
		{
			base = over;
		}
		else
		{
#ifdef BV2_PLATFORM_WINDOWS
			std::string appdata = getEnv("APPDATA");
			base = fs::path(appdata.empty() ? "." : appdata) / "BaboViolent2" / "bv2";
#elif defined(BV2_PLATFORM_MACOS)
			base = fs::path(getEnv("HOME")) / "Library" / "Application Support" / "BaboViolent2" / "bv2";
#else
			std::string xdg = getEnv("XDG_DATA_HOME");
			base = xdg.empty() ? fs::path(getEnv("HOME")) / ".local" / "share" : fs::path(xdg);
			base = base / "BaboViolent2" / "bv2";
#endif
		}
		std::error_code ec;
		fs::create_directories(base, ec);
		std::string s = base.generic_string();
		if (s.empty() || s.back() != '/')
			s += '/';
		return s;
	}

	void applyFile(const std::string & file, bool transient)
	{
		std::error_code ec;
		if (!fs::is_regular_file(file, ec))
			return;
		std::string f = file;
		if (transient)
			dksvarLoadConfigTransient((char *)f.c_str());
		else
			dksvarLoadConfig((char *)f.c_str());
	}

	void applyEnv(const char * envName, const char * varName)
	{
		std::string v = getEnv(envName);
		if (!v.empty())
			dksvarSetTransient(varName, v.c_str());
	}
}

namespace bv2
{
	bool pathsInit(const char * argv0)
	{
		g_prefDir = computePrefDir();

		std::vector<fs::path> candidates;
		std::string exe = exeDir(argv0);
		if (!exe.empty())
		{
			fs::path e(exe);
#ifdef BV2_PLATFORM_MACOS
			candidates.push_back(e.parent_path() / "Resources"); // bv2.app/Contents/MacOS -> Contents/Resources
#endif
			candidates.push_back(e.parent_path() / "share" / "bv2"); // <prefix>/bin -> <prefix>/share/bv2
			candidates.push_back(e);
		}
		candidates.push_back(fs::current_path());
		g_packageRoot.clear();
		for (size_t i = 0; i < candidates.size() && g_packageRoot.empty(); ++i)
			if (hasMain(candidates[i]))
				g_packageRoot = rootString(candidates[i]);

		fs::path envRoot = firstRootWith(getEnv("BV2_DATA_DIR"), "main");
		if (!envRoot.empty())
			return setDataRoot(envRoot.generic_string());
		if (!g_packageRoot.empty())
			return setDataRoot(g_packageRoot);
		g_dataRoot.clear();
		return false;
	}

	bool setDataRoot(const std::string & dir)
	{
		std::error_code ec;
		g_dataRoot = rootString(dir);
		fs::current_path(dir, ec);
		return !ec;
	}

	std::string contentFile(const std::string & rel)
	{
		std::error_code ec;
		if (!g_packageRoot.empty() && fs::exists(g_packageRoot + rel, ec))
			return g_packageRoot + rel;
		return rel;
	}

	std::string findDataIn(const std::string & dir)
	{
		fs::path root = firstRootWith(dir, "main/textures");
		return root.empty() ? std::string() : rootString(root);
	}

	std::string savedDataDir()
	{
		std::ifstream in(fs::path(g_prefDir) / "data-dir.txt");
		std::string line;
		std::getline(in, line);
		return findDataIn(line);
	}

	void saveDataDir(const std::string & dir)
	{
		std::ofstream out(fs::path(userFile("data-dir.txt")), std::ios::trunc);
		out << dir << "\n";
	}

	std::string detectDataDir()
	{
		std::vector<fs::path> places;
		std::string home = getEnv("HOME");
#ifdef BV2_PLATFORM_WINDOWS
		if (home.empty())
			home = getEnv("USERPROFILE");
		const char * programDirs[2] = { "ProgramFiles", "ProgramFiles(x86)" };
		for (int i = 0; i < 2; ++i)
		{
			std::string pf = getEnv(programDirs[i]);
			if (!pf.empty())
			{
				places.push_back(fs::path(pf) / "BaboViolent 2");
				places.push_back(fs::path(pf) / "BaboViolent2");
			}
		}
#endif
		if (!home.empty())
		{
			const char * folders[3] = { "Downloads", "Desktop", "Documents" };
			const char * names[2] = { "BaboViolent2-master", "BaboViolent2" };
			for (int f = 0; f < 3; ++f)
				for (int n = 0; n < 2; ++n)
					places.push_back(fs::path(home) / folders[f] / names[n]);
		}
		for (size_t i = 0; i < places.size(); ++i)
		{
			std::string found = findDataIn(places[i].generic_string());
			if (!found.empty())
				return found;
		}
		return "";
	}

	const std::string & dataRoot() { return g_dataRoot; }
	const std::string & packageRoot() { return g_packageRoot; }
	const std::string & prefDir() { return g_prefDir; }

	std::string userFile(const std::string & name)
	{
		fs::path p = fs::path(g_prefDir) / name;
		std::error_code ec;
		fs::create_directories(p.parent_path(), ec);
		return p.generic_string();
	}

	std::vector<std::string> listMapNames()
	{
		std::vector<std::string> names;
		const std::string dirs[2] = { "main/maps", g_prefDir + "maps" };
		for (int d = 0; d < 2; ++d)
		{
			std::error_code ec;
			for (fs::directory_iterator it(dirs[d], ec), end; !ec && it != end; it.increment(ec))
			{
				const fs::path & p = it->path();
				std::string ext = p.extension().string();
				std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return (char)std::tolower(c); });
				if (ext == ".bvm")
					names.push_back(p.stem().string());
			}
		}
		std::sort(names.begin(), names.end());
		names.erase(std::unique(names.begin(), names.end()), names.end());
		return names;
	}

	std::string mapFileForRead(const std::string & name)
	{
		std::string user = g_prefDir + "maps/" + name + ".bvm";
		std::error_code ec;
		if (fs::is_regular_file(user, ec))
			return user;
		return "main/maps/" + name + ".bvm";
	}

	std::string mapFileForWrite(const std::string & name)
	{
		return userFile("maps/" + name + ".bvm");
	}

	std::string configFile()
	{
		return g_prefDir + "bv2.cfg";
	}

	std::string loadConfigLayers(int argc, const char * const * argv)
	{
		// 1. defaults shipped with the game
		applyFile(contentFile("main/bv2.example.cfg"), false);

		// 2. the user's saved settings; the first run writes them out from the defaults
		std::string cfg = configFile();
		std::error_code ec;
		if (!fs::exists(cfg, ec))
			dksvarSaveConfig((char *)cfg.c_str());
		applyFile(cfg, false);

		// 3. local overrides: config/local/*.cfg beside the package's main/ and, when it differs, the data root's
		// (ADR 0016), then --config files
		std::vector<std::string> dirs;
		dirs.push_back(g_packageRoot + "config/local");
		if (g_dataRoot != g_packageRoot)
			dirs.push_back(g_dataRoot + "config/local");
		for (size_t d = 0; d < dirs.size(); ++d)
		{
			std::vector<std::string> files;
			for (fs::directory_iterator it(dirs[d], ec), end; !ec && it != end; it.increment(ec))
				if (it->path().extension() == ".cfg")
					files.push_back(it->path().generic_string());
			std::sort(files.begin(), files.end());
			for (size_t i = 0; i < files.size(); ++i)
				applyFile(files[i], true);
		}
		for (int i = 1; i + 1 < argc; ++i)
			if (std::strcmp(argv[i], "--config") == 0)
				applyFile(argv[i + 1], true);

		// 4. environment
		applyEnv("BV2_MASTER_SERVERS", "cl_masterServers");
		applyEnv("BV2_ACCOUNT_URL", "cl_accountURL");
		applyEnv("BV2_SV_PASSWORD", "sv_password");
		applyEnv("BV2_ADMIN_PASS", "zsv_adminPass");
		applyEnv("BV2_ADMIN_API_PORT", "zsv_adminApiPort");
		applyEnv("BV2_ADMIN_API_BIND", "zsv_adminApiBind");
		applyEnv("BV2_ADMIN_API_PROXY", "zsv_adminApiProxy");
		return cfg;
	}

	sqlite3 * openClientDb()
	{
		sqlite3 * db = 0;
		std::string path = userFile("bv2.db");
		if (sqlite3_open(path.c_str(), &db) != SQLITE_OK)
		{
			if (db)
				sqlite3_close(db);
			return 0;
		}
		// Same schema and defaults as content-seed/bv2.sql; the master list and account URL live in the config now.
		// Constant DDL and defaults, no data: sqlite3_exec is fine here (Phase B step 6).
		sqlite3_exec(db,
			"CREATE TABLE IF NOT EXISTS LauncherSettings (Name TEXT, Value TEXT);"
			"CREATE TABLE IF NOT EXISTS MasterServers (Score NUMERIC, ID INTEGER PRIMARY KEY, IP TEXT, Location TEXT, Port NUMERIC);"
			"INSERT INTO LauncherSettings (Name, Value) SELECT 'Version', '2.11' WHERE NOT EXISTS (SELECT 1 FROM LauncherSettings WHERE Name = 'Version');"
			"INSERT INTO LauncherSettings (Name, Value) SELECT 'DBVersion', '8' WHERE NOT EXISTS (SELECT 1 FROM LauncherSettings WHERE Name = 'DBVersion');"
			"INSERT INTO LauncherSettings (Name, Value) SELECT 'AccountURL', '' WHERE NOT EXISTS (SELECT 1 FROM LauncherSettings WHERE Name = 'AccountURL');"
			"INSERT INTO LauncherSettings (Name, Value) SELECT 'DidSurvey', '1' WHERE NOT EXISTS (SELECT 1 FROM LauncherSettings WHERE Name = 'DidSurvey');",
			0, 0, 0);
		return db;
	}

	std::vector<HostPort> parseHostPorts(const std::string & list)
	{
		std::vector<HostPort> out;
		std::string token;
		std::istringstream in(list);
		while (std::getline(in, token, ','))
		{
			std::istringstream words(token);
			std::string w;
			while (words >> w)
			{
				HostPort hp;
				hp.port = 11112;
				size_t colon = w.rfind(':');
				if (colon != std::string::npos && colon + 1 < w.size())
				{
					hp.host = w.substr(0, colon);
					hp.port = std::atoi(w.c_str() + colon + 1);
				}
				else
					hp.host = w;
				if (!hp.host.empty() && hp.port > 0 && hp.port < 65536)
					out.push_back(hp);
			}
		}
		return out;
	}
}
