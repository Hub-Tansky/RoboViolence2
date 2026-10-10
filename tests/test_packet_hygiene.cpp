// Phase B step 4: the server's pre-dispatch check and the fixes for Q-S1, Q-S2, Q-S4, Q-S6 and Q-S7.
#include "corpus_types.h"
#include "server_harness.h"

#include "Console.h"
#include "GameVar.h"
#include "Scene.h"
#include "Server.h"

#include <cstdio>

extern Scene * scene;

static int failures = 0;

#define CHECK(cond)                                                    \
	do                                                                 \
	{                                                                  \
		if (!(cond))                                                   \
		{                                                              \
			std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
			++failures;                                                \
		}                                                              \
	} while (0)

static Game * game() { return scene->server->game; }

// A client that has joined (handshake and player info); returns its babonetID. Its slot is the next free one.
static UINT4 join()
{
	UINT4 c = harness::connect();
	for (int id : {NET_CLSV_GAMEVERSION_ACCEPTED, NET_CLSV_SVCL_PLAYER_INFO})
	{
		std::vector<char> m = corpus::valid(id);
		harness::deliver(c, id, m.data(), (int)m.size());
	}
	return c;
}

static int slotOf(UINT4 c)
{
	for (int i = 0; i < MAX_PLAYER; ++i)
		if (game()->players[i] && game()->players[i]->babonetID == (INT4)c) return i;
	return -1;
}

static int sentOfType(int typeID)
{
	int n = 0;
	for (const harness::SentPacket & p : harness::sent())
		if (p.typeID == typeID) ++n;
	return n;
}

template <typename T> static void deliver(UINT4 from, int typeID, const T & m)
{
	harness::sent().clear();
	harness::deliver(from, typeID, &m, sizeof(m));
}

int main(int, char ** argv)
{
	if (!harness::start(argv[0])) return 1;
	UINT4 a = join();
	UINT4 b = join();
	int slotA = slotOf(a), slotB = slotOf(b);
	CHECK(slotA >= 0 && slotB >= 0 && slotA != slotB);

	// Q-S1: B asks to move A to red; the server applies it to B's own slot
	char teamA = game()->players[slotA]->teamID;
	net_clsv_svcl_team_request team = {};
	team.playerID = (int8_t)slotA;
	team.teamRequested = PLAYER_TEAM_RED;
	deliver(b, NET_CLSV_SVCL_TEAM_REQUEST, team);
	CHECK(game()->players[slotA]->teamID == teamA);
	CHECK(game()->players[slotB]->teamID == PLAYER_TEAM_RED);

	// Q-S1: B renames A; the name change lands on B
	net_clsv_svcl_player_change_name rename = {};
	rename.playerID = (int8_t)slotA;
	strcpy(rename.playerName, "Renamed");
	deliver(b, NET_CLSV_SVCL_PLAYER_CHANGE_NAME, rename);
	CHECK(game()->players[slotA]->name != "Renamed");
	CHECK(game()->players[slotB]->name == "Renamed");

	// Q-S2: out-of-range playerID, weaponID and team are ignored, not used as indices (ASan would report them)
	team.playerID = 100;
	team.teamRequested = 50;
	deliver(b, NET_CLSV_SVCL_TEAM_REQUEST, team);
	CHECK(game()->players[slotB]->teamID == PLAYER_TEAM_RED);
	net_clsv_player_shoot shoot = {};
	shoot.weaponID = 100;
	deliver(b, NET_CLSV_PLAYER_SHOOT, shoot);
	net_clsv_spawn_request spawn = {};
	spawn.weaponID = 60;
	spawn.meleeID = 10;
	deliver(b, NET_CLSV_SPAWN_REQUEST, spawn);
	CHECK(sentOfType(NET_SVCL_PLAYER_SPAWN) == 0);

	// Q-S6: a map name with a path is refused; a plain one starts a transfer
	net_clsv_map_request map = {};
	strcpy(map.mapName, "../../bv2.cfg");
	deliver(a, NET_CLSV_MAP_REQUEST, map);
	harness::tick(5);
	CHECK(sentOfType(NET_SVCL_MAP_CHUNK) == 0);
	strcpy(map.mapName, "CTF-Placeholder");
	deliver(a, NET_CLSV_MAP_REQUEST, map);
	harness::tick(5);
	CHECK(sentOfType(NET_SVCL_MAP_CHUNK) > 0);

	// Q-S7: a skin update is broadcast once, not once per player
	net_clsv_svcl_player_update_skin skin = {};
	strcpy(skin.skin, "skin10");
	deliver(a, NET_CLSV_SVCL_PLAYER_UPDATE_SKIN, skin);
	CHECK(sentOfType(NET_CLSV_SVCL_PLAYER_UPDATE_SKIN) == 1);

	// Q-S7: an unknown sound is not relayed; a known one is
	net_svcl_play_sound sound = {};
	sound.soundID = 99;
	deliver(a, NET_SVCL_PLAY_SOUND, sound);
	CHECK(sentOfType(NET_SVCL_PLAY_SOUND) == 0);
	sound.soundID = SOUND_MOLOTOV;
	deliver(a, NET_SVCL_PLAY_SOUND, sound);
	CHECK(sentOfType(NET_SVCL_PLAY_SOUND) == 1);

	// Q-S4: SV_CHANGE may only set sv_* variables
	CString adminPass("%s", gameVar.zsv_adminPass.s);
	console->svChange("set zsv_adminPass changed");
	CHECK(gameVar.zsv_adminPass == adminPass);
	console->svChange("set sv_scoreLimit 42");
	CHECK(gameVar.sv_scoreLimit == 42);

	// Every message type, one byte short or long: dropped before a handler reads it (ASan builds catch any read past
	// the delivered bytes), so a type with a reply gets none
	for (const corpus::Type & t : corpus::types)
	{
		if (t.size == 0) continue; // text: any length
		std::vector<char> m = corpus::valid(t.id);
		for (int size : {t.size - 1, t.size + 1})
		{
			std::vector<char> wrong(m.begin(), m.end());
			wrong.resize(size);
			harness::sent().clear();
			harness::deliver(a, t.id, wrong.data(), size);
			if (t.reply && sentOfType(t.reply)) std::printf("FAIL %s with %d bytes was handled\n", t.name, size), ++failures;
		}
	}

	harness::stop();
	if (failures == 0) std::printf("ok: packet hygiene\n");
	return failures ? 1 : 0;
}
