// Phase B step 4: the server's pre-dispatch check and the fixes for Q-S1, Q-S2, Q-S4, Q-S6 and Q-S7.
#include "corpus_types.h"
#include "server_harness.h"

#include "Console.h"
#include "Game.h"
#include "PacketCheck.h"
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
	UINT4 a = harness::join();
	UINT4 b = harness::join();
	int slotA = harness::slotOf(a), slotB = harness::slotOf(b);
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
	net_clsv_spawn_request spawn = {};
	spawn.weaponID = 60;
	spawn.meleeID = 10;
	deliver(b, NET_CLSV_SPAWN_REQUEST, spawn);
	CHECK(sentOfType(NET_SVCL_PLAYER_SPAWN) == 0);
	spawn.weaponID = 0; // control: a known weapon spawns
	deliver(b, NET_CLSV_SPAWN_REQUEST, spawn);
	CHECK(sentOfType(NET_SVCL_PLAYER_SPAWN) == 1);
	net_clsv_player_shoot shoot = {};
	shoot.weaponID = 100;
	shoot.p2[0] = 500;
	harness::tick(30); // past the fire delay
	deliver(b, NET_CLSV_PLAYER_SHOOT, shoot);
	CHECK(sentOfType(NET_SVCL_PLAYER_SHOOT) == 0);
	shoot.weaponID = 0; // control: the held SMG shoots
	deliver(b, NET_CLSV_PLAYER_SHOOT, shoot);
	CHECK(sentOfType(NET_SVCL_PLAYER_SHOOT) > 0);

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

	// Q-S7: an unknown sound is not relayed; a known one plays at the (living) sender's position, not the packet's
	net_svcl_play_sound sound = {};
	sound.soundID = 99;
	deliver(b, NET_SVCL_PLAY_SOUND, sound);
	CHECK(sentOfType(NET_SVCL_PLAY_SOUND) == 0);
	sound.soundID = SOUND_MOLOTOV;
	sound.position[0] = 200;
	deliver(b, NET_SVCL_PLAY_SOUND, sound);
	CHECK(sentOfType(NET_SVCL_PLAY_SOUND) == 1);
	for (const harness::SentPacket & p : harness::sent())
		if (p.typeID == NET_SVCL_PLAY_SOUND)
			CHECK(((const net_svcl_play_sound *)p.data.data())->position[0] == (unsigned char)game()->players[slotB]->currentCF.position[0]);

	// 4.1: before its join handshake a client may send only the handshake and pings
	UINT4 c = harness::connect();
	net_clsv_svcl_chat chat = {};
	chat.teamID = -2;
	strcpy(chat.message, "early");
	deliver(c, NET_CLSV_SVCL_CHAT, chat);
	CHECK(sentOfType(NET_CLSV_SVCL_CHAT) == 0);
	deliver(a, NET_CLSV_SVCL_CHAT, chat); // control: a joined client's chat is relayed
	CHECK(sentOfType(NET_CLSV_SVCL_CHAT) == 1);

	// Q-S4: SV_CHANGE may only set sv_* variables
	CString adminPass("%s", gameVar.zsv_adminPass.s);
	console->svChange("set zsv_adminPass changed");
	CHECK(gameVar.zsv_adminPass == adminPass);
	console->svChange("set sv_scoreLimit 42");
	CHECK(gameVar.sv_scoreLimit == 42);

	// 4.1, every client message type: the right size passes the pre-dispatch check, one byte short or long doesn't
	std::vector<char> out;
	for (const corpus::Type & t : corpus::types)
	{
		std::vector<char> m = corpus::valid(t.id);
		CHECK(scene->server->checkPacket(m.data(), (int)m.size(), t.id, a, out) == slotA || std::printf("  %s\n", t.name) < 0);
		if (t.size == 0) continue; // text: any length
		for (int size : {t.size - 1, t.size + 1})
		{
			std::vector<char> wrong(m.begin(), m.end());
			wrong.resize(size);
			CHECK(scene->server->checkPacket(wrong.data(), size, t.id, a, out) < 0 || std::printf("  %s, %d bytes\n", t.name, size) < 0);
		}
	}

	// The client's check of server messages, with values the server really sends (and some it never does)
	net_clsv_svcl_player_projectile projectile = {};
	projectile.weaponID = -2; // a flame, listed to a client that joins mid-game
	CHECK(packet::checkServerMessage((char *)&projectile, sizeof(projectile), NET_CLSV_SVCL_PLAYER_PROJECTILE, out));
	projectile.weaponID = -1; // a life pack or dropped weapon
	CHECK(packet::checkServerMessage((char *)&projectile, sizeof(projectile), NET_CLSV_SVCL_PLAYER_PROJECTILE, out));
	projectile.weaponID = 15; // an empty weapon slot
	CHECK(!packet::checkServerMessage((char *)&projectile, sizeof(projectile), NET_CLSV_SVCL_PLAYER_PROJECTILE, out));
	net_svcl_player_shoot svShoot = {};
	svShoot.hitPlayerID = -1;
	svShoot.weaponID = WEAPON_MINIBOT_WEAPON;
	CHECK(packet::checkServerMessage((char *)&svShoot, sizeof(svShoot), NET_SVCL_PLAYER_SHOOT, out));
	net_svcl_player_hit hit = {};
	hit.weaponID = WEAPON_MINIBOT_WEAPON; // only shots carry the minibot's gun
	CHECK(!packet::checkServerMessage((char *)&hit, sizeof(hit), NET_SVCL_PLAYER_HIT, out));
	hit.weaponID = 0;
	hit.fromID = -1;
	CHECK(!packet::checkServerMessage((char *)&hit, sizeof(hit), NET_SVCL_PLAYER_HIT, out));
	net_svcl_pickup_item pickup = {};
	pickup.itemFlag = 15;
	CHECK(!packet::checkServerMessage((char *)&pickup, sizeof(pickup), NET_SVCL_PICKUP_ITEM, out));
	net_svcl_flag_enum flags = {};
	flags.flagState[0] = -2;
	flags.flagState[1] = 40;
	CHECK(!packet::checkServerMessage((char *)&flags, sizeof(flags), NET_SVCL_FLAG_ENUM, out));
	flags.flagState[1] = -1;
	CHECK(packet::checkServerMessage((char *)&flags, sizeof(flags), NET_SVCL_FLAG_ENUM, out));
	net_svcl_map_list list;
	memset(list.mapName, 'A', sizeof(list.mapName));
	CHECK(packet::checkServerMessage((char *)&list, sizeof(list), NET_SVCL_MAP_LIST, out) && out.back() == '\0');
	CHECK(!packet::checkServerMessage((char *)&list, sizeof(list) - 1, NET_SVCL_MAP_LIST, out));
	CHECK(!packet::checkServerMessage((char *)&list, sizeof(list), 9999, out));

	harness::stop();
	if (failures == 0) std::printf("ok: packet hygiene\n");
	return failures ? 1 : 0;
}
