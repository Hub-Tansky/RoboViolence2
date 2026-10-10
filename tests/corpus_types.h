// Client-to-server message types for the corpus, replay and fuzz tests (Phase B step 3).
// tests/corpus/<name>/*.bin holds payloads of one type; gen_corpus writes valid.bin for each from netPacket.h.
#pragma once
#include "netPacket.h"

#include <cstring>
#include <string>
#include <vector>

namespace corpus
{
	struct Type
	{
		const char * name;
		int id;
		int size;      // payload bytes the server memcpy's (0: text, any length)
		int reply;     // a message the server sends after the valid sample, 0 if none is checked
	};

	// In replay order: the join handshake first.
	const Type types[] = {
		{"clsv_gameversion_accepted", NET_CLSV_GAMEVERSION_ACCEPTED, sizeof(net_clsv_gameversion_accepted), NET_SVCL_SERVER_INFO},
		{"clsv_svcl_player_info", NET_CLSV_SVCL_PLAYER_INFO, sizeof(net_clsv_svcl_player_info), NET_CLSV_SVCL_PLAYER_INFO},
		{"clsv_pong", NET_CLSV_PONG, sizeof(net_clsv_pong), 0},
		{"clsv_spawn_request", NET_CLSV_SPAWN_REQUEST, sizeof(net_clsv_spawn_request), 0},
		{"clsv_player_shoot", NET_CLSV_PLAYER_SHOOT, sizeof(net_clsv_player_shoot), 0},
		{"clsv_pickup_request", NET_CLSV_PICKUP_REQUEST, sizeof(net_clsv_pickup_request), 0},
		{"clsv_admin_request", NET_CLSV_ADMIN_REQUEST, sizeof(net_clsv_admin_request), 0},
		{"clsv_vote", NET_CLSV_VOTE, sizeof(net_clsv_vote), 0},
		{"clsv_map_list_request", NET_CLSV_MAP_LIST_REQUEST, sizeof(net_clsv_map_list_request), NET_SVCL_MAP_LIST},
		{"clsv_svcl_chat", NET_CLSV_SVCL_CHAT, sizeof(net_clsv_svcl_chat), NET_CLSV_SVCL_CHAT},
		{"clsv_svcl_team_request", NET_CLSV_SVCL_TEAM_REQUEST, sizeof(net_clsv_svcl_team_request), NET_CLSV_SVCL_TEAM_REQUEST},
		{"clsv_svcl_player_coord_frame", NET_CLSV_SVCL_PLAYER_COORD_FRAME, sizeof(net_clsv_svcl_player_coord_frame), 0},
		{"clsv_svcl_player_change_name", NET_CLSV_SVCL_PLAYER_CHANGE_NAME, sizeof(net_clsv_svcl_player_change_name), 0},
		{"clsv_svcl_player_projectile", NET_CLSV_SVCL_PLAYER_PROJECTILE, sizeof(net_clsv_svcl_player_projectile), 0},
		{"clsv_svcl_player_shoot_melee", NET_CLSV_SVCL_PLAYER_SHOOT_MELEE, sizeof(net_clsv_svcl_player_shoot_melee), 0},
		{"clsv_svcl_vote_request", NET_CLSV_SVCL_VOTE_REQUEST, sizeof(net_clsv_svcl_vote_request), 0},
		{"clsv_map_request", NET_CLSV_MAP_REQUEST, sizeof(net_clsv_map_request), 0},
		{"clsv_svcl_player_update_skin", NET_CLSV_SVCL_PLAYER_UPDATE_SKIN, sizeof(net_clsv_svcl_player_update_skin), 0},
		{"svcl_play_sound", NET_SVCL_PLAY_SOUND, sizeof(net_svcl_play_sound), 0},
		{"svcl_console", NET_SVCL_CONSOLE, 0, 0},
	};

	inline const Type * find(const std::string & name)
	{
		for (const Type & t : types)
			if (name == t.name) return &t;
		return 0;
	}

	template <typename T> std::vector<char> bytes(const T & v)
	{
		const char * p = reinterpret_cast<const char *>(&v);
		return std::vector<char>(p, p + sizeof(T));
	}

	// A valid payload from the first connected player (playerID 0).
	inline std::vector<char> valid(int id)
	{
		switch (id)
		{
		case NET_CLSV_PONG: { net_clsv_pong m = {}; return bytes(m); }
		case NET_CLSV_SPAWN_REQUEST: { net_clsv_spawn_request m = {}; m.weaponID = 0; /* SMG */ m.meleeID = 10; /* knives; others are kicked */ strcpy(m.skin, "skin10"); return bytes(m); }
		case NET_CLSV_PLAYER_SHOOT: { net_clsv_player_shoot m = {}; m.p2[0] = 500; return bytes(m); }
		case NET_CLSV_GAMEVERSION_ACCEPTED: { net_clsv_gameversion_accepted m = {}; return bytes(m); }
		case NET_CLSV_PICKUP_REQUEST: { net_clsv_pickup_request m = {}; return bytes(m); }
		case NET_CLSV_ADMIN_REQUEST: { net_clsv_admin_request m = {}; strcpy(m.login, "user"); strcpy(m.password, "pass"); return bytes(m); }
		case NET_CLSV_VOTE: { net_clsv_vote m = {}; m.value = true; return bytes(m); }
		case NET_CLSV_MAP_LIST_REQUEST: { net_clsv_map_list_request m = {}; m.all = false; /* the server's map list */ return bytes(m); }
		case NET_CLSV_SVCL_PLAYER_INFO: { net_clsv_svcl_player_info m = {}; strcpy(m.playerName, "Tester"); return bytes(m); }
		case NET_CLSV_SVCL_CHAT: { net_clsv_svcl_chat m = {}; m.teamID = -2; /* everyone */ strcpy(m.message, "Tester: hello"); return bytes(m); }
		case NET_CLSV_SVCL_TEAM_REQUEST: { net_clsv_svcl_team_request m = {}; m.teamRequested = 1; /* red */ return bytes(m); }
		case NET_CLSV_SVCL_PLAYER_COORD_FRAME: { net_clsv_svcl_player_coord_frame m = {}; m.frameID = 1; m.position[0] = 500; m.position[1] = 500; return bytes(m); }
		case NET_CLSV_SVCL_PLAYER_CHANGE_NAME: { net_clsv_svcl_player_change_name m = {}; strcpy(m.playerName, "Tester2"); return bytes(m); }
		case NET_CLSV_SVCL_PLAYER_PROJECTILE: { net_clsv_svcl_player_projectile m = {}; m.weaponID = 5; /* bazooka */ m.position[0] = 500; m.position[1] = 500; m.vel[0] = 10; return bytes(m); }
		case NET_CLSV_SVCL_PLAYER_SHOOT_MELEE: { net_clsv_svcl_player_shoot_melee m = {}; return bytes(m); }
		case NET_CLSV_SVCL_VOTE_REQUEST: { net_clsv_svcl_vote_request m = {}; strcpy(m.vote, "changemap CTF-Placeholder"); return bytes(m); }
		case NET_CLSV_MAP_REQUEST: { net_clsv_map_request m = {}; strcpy(m.mapName, "CTF-Placeholder"); return bytes(m); }
		case NET_CLSV_SVCL_PLAYER_UPDATE_SKIN: { net_clsv_svcl_player_update_skin m = {}; strcpy(m.skin, "skin10"); return bytes(m); }
		case NET_SVCL_PLAY_SOUND: { net_svcl_play_sound m = {}; m.volume = 100; return bytes(m); }
		case NET_SVCL_CONSOLE: { const char text[] = "status"; return std::vector<char>(text, text + sizeof(text)); }
		}
		return std::vector<char>();
	}

	// Inputs for known defects (KEY_QUESTIONS.md). crash-*.bin: ctest expects ASan to catch it until its fix step;
	// fixed-*.bin: replayed like valid.bin, so the fix stays in place. Step 4 fixed all of these.
	struct Input
	{
		const char * type;
		const char * name;
		std::vector<char> data;
	};
	inline std::vector<Input> knownDefects()
	{
		net_clsv_svcl_team_request team = {};
		team.playerID = 100; // Q-S2: indexes players[32] unchecked
		net_clsv_svcl_chat chat = {};
		chat.teamID = -2;
		strcpy(chat.message, "%s%s%s%s%s%s%s%s%s%s"); // R21: chat text is a printf format
		const char console[] = {'s', 't', 'a', 't'}; // R22: no '\0'; read as a format string
		net_clsv_admin_request admin = {};
		strcpy(admin.login, "%s%s%s%s%s%s%s%s%s%s"); // R23: login is a printf format
		net_clsv_map_request map = {};
		memset(map.mapName, 'A', sizeof(map.mapName)); // R24: mapName without '\0'
		map.uniqueClientID = 0x41414141; // nor in the field after it
		return {{"clsv_svcl_team_request", "fixed-playerid-out-of-range", bytes(team)},
			{"clsv_svcl_chat", "fixed-format-string", bytes(chat)},
			{"svcl_console", "fixed-unterminated", std::vector<char>(console, console + sizeof(console))},
			{"clsv_admin_request", "fixed-format-string", bytes(admin)},
			{"clsv_map_request", "fixed-unterminated", bytes(map)}};
	}
}
