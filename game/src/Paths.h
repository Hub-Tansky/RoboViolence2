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
//   package content      <packageRoot>/main/...  launch scripts, languages, bv2.example.cfg; see contentFile() (ADR 0016)
//   user data (writable) <prefDir>/...         bv2.cfg, bv2.db, banlist, console.log, screenshots, saved maps
namespace bv2
{
	// Finds packageRoot (the app bundle, <exe>/../share/bv2, next to the exe, the working directory, in that order;
	// the first with a main/ folder wins) and dataRoot (the folder findDataIn finds in BV2_DATA_DIR, else packageRoot),
	// creates prefDir and changes into dataRoot.
	// Returns false when no main/ folder exists anywhere (the game can still run servers without content).
	bool pathsInit(const char * argv0);

	const std::string & dataRoot();
	const std::string & packageRoot();
	// True when BV2_DATA_DIR supplied the data root
	bool dataFromEnv();

	// Makes dir (a folder holding main/) the data root and changes into it
	bool setDataRoot(const std::string & dir);

	// Path of content the package owns ("main/LaunchScript/CTF.cfg"): packageRoot() + rel when it exists there,
	// else rel (relative to the data root)
	std::string contentFile(const std::string & rel);

	// The folder holding BV2 data (main/textures/) at dir, at its parent when dir is main/, at dir/Content, at
	// dir/BaboViolent2/Content (the upstream repository) or one level deeper in a folder of dir's own name
	// (Windows Extract All); "" when none
	std::string findDataIn(const std::string & dir);

	// The player's data folder saved in prefDir()/data-dir.txt; "" when unset or it no longer holds data
	std::string savedDataDir();
	// Saves dir, or PLACEHOLDERS for the player's choice to play without BV2 data (ADR 0041)
	void saveDataDir(const std::string & dir);
	const char * const PLACEHOLDERS = "placeholders";
	// True when data-dir.txt holds PLACEHOLDERS
	bool placeholdersSaved();

	// Where the first-run download installs the original data, without a trailing '/': "original" beside prefDir()
	// (%APPDATA%/BaboViolent2/original on Windows), or inside it when BV2_PREF_DIR overrides it
	const std::string & originalDataDir();

	// BV2 data in usual places: originalDataDir(), BaboViolent2-master or BaboViolent2 in ~/Downloads, ~/Desktop, ~/Documents;
	// on Windows also "BaboViolent 2" or BaboViolent2 in Program Files; "" when none
	std::string detectDataDir();

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
	// cl_masterServers is never saved and layer 2 cannot set it (ADR 0028).
	// Call after dksvarInit(). Returns the path of the saved bv2.cfg.
	std::string loadConfigLayers(int argc, const char * const * argv);

	// The user's saved config file (prefDir()/bv2.cfg)
	std::string configFile();

	// Opens prefDir()/bv2.db, creating the tables on first use (content-seed/bv2.sql). Close with sqlite3_close.
	sqlite3 * openClientDb();

	// "host:port" entries from cl_masterServers (comma or space separated); port defaults to 11112; a "none" entry is skipped, so "none" alone means no master
	struct HostPort { std::string host; int port; };
	std::vector<HostPort> parseHostPorts(const std::string & list);
}

#endif
