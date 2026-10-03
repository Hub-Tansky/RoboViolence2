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

#ifndef BV2_PATHS_H
#define BV2_PATHS_H

#include <string>
#include <vector>

struct sqlite3;

// Where the game finds its content and keeps user data, and how config layers are applied (ADR 0007).
//
//   content (read-only)  <dataRoot>/main/...   found by bv2PathsInit(); the process then chdir()s to dataRoot,
//                                              so the code's "main/..." paths keep working
//   user data (writable) <prefDir>/...         bv2.cfg, bv2.db, banlist, console.log, screenshots, saved maps
namespace bv2
{
	// Finds dataRoot (BV2_DATA_DIR, the app bundle, <exe>/../share/bv2, next to the exe, the working directory,
	// in that order; the first with a main/ folder wins), creates prefDir and changes into dataRoot.
	// Returns false when no main/ folder exists anywhere (the game can still run servers without content).
	bool pathsInit(const char * argv0);

	const std::string & dataRoot();

	// Writable per-user directory, always ends with '/'. BV2_PREF_DIR overrides it (read-only installs, tests).
	const std::string & prefDir();

	// prefDir() + name, creating missing parent directories
	std::string userFile(const std::string & name);

	// Files with the given extension (".bvm") in main/maps and prefDir()/maps, names without extension, sorted
	std::vector<std::string> listMapNames();

	// Path of a map to read: prefDir()/maps/<name>.bvm when it exists, else main/maps/<name>.bvm
	std::string mapFileForRead(const std::string & name);

	// Path to write a map to: prefDir()/maps/<name>.bvm
	std::string mapFileForWrite(const std::string & name);

	// Loads the config layers, later ones overriding earlier ones:
	//   1. main/bv2.example.cfg        defaults shipped with the game
	//   2. prefDir()/bv2.cfg           the user's saved settings (created from the defaults on first run)
	//   3. config/local/*.cfg and --config <file> (argv)   server and local overrides, never saved back
	//   4. environment: BV2_MASTER_SERVERS, BV2_ACCOUNT_URL, BV2_SV_PASSWORD, BV2_ADMIN_PASS   never saved back
	// Call after dksvarInit(). Returns the path of the saved bv2.cfg.
	std::string loadConfigLayers(int argc, const char * const * argv);

	// The user's saved config file (prefDir()/bv2.cfg)
	std::string configFile();

	// Opens prefDir()/bv2.db, creating the tables on first use (content-seed/bv2.sql). Close with sqlite3_close.
	sqlite3 * openClientDb();

	// "host:port" entries from cl_masterServers (comma or space separated); port defaults to 11112
	struct HostPort { std::string host; int port; };
	std::vector<HostPort> parseHostPorts(const std::string & list);
}

#endif
