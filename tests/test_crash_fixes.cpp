// Phase B step 5: one check per fixed defect (KEY_QUESTIONS.md Part D) on the in-process server.
#include "server_harness.h"

#include "CMaster.h"
#include "Console.h"
#include "Game.h"
#include "GameVar.h"
#include "Map.h"
#include "Paths.h"
#include "ePTexture.h"
#include "Scene.h"
#include "Server.h"

#include <cstdio>
#include <cstring>

extern Scene * scene;



// Fires a rocket from the client's position with x velocity vel (in tenths)
static void fireRocket(UINT4 c, int vel)
{
	Player * p = scene->server->game->players[harness::slotOf(c)];
	net_clsv_svcl_player_projectile rocket = {};
	rocket.weaponID = WEAPON_BAZOOKA;
	rocket.projectileType = PROJECTILE_ROCKET;
	for (int k = 0; k < 3; ++k) rocket.position[k] = (short)(p->currentCF.position[k] * 100);
	rocket.vel[0] = (char)vel;
	harness::deliver(c, NET_CLSV_SVCL_PLAYER_PROJECTILE, rocket);
}

// R3 (ADR 0013): a rocket in flight no longer switches a Normal server to the Pro rules, and switching the ruleset
// reapplies the weapon table on the server too
static void r3()
{
	console->sendCommand("set sv_serverType 0");
	UINT4 c = harness::spawned(WEAPON_BAZOOKA);
	CHECK(scene->server->game->players[harness::slotOf(c)]->status == PLAYER_STATUS_ALIVE);
	fireRocket(c, 10);
	CHECK(!scene->server->game->projectiles.empty());
	harness::tick(10);
	CHECK(gameVar.sv_serverType == 0);
	console->sendCommand("set sv_serverType 1");
	CHECK(gameVar.weapons[WEAPON_SHIELD]->fireDelay == 2.5f);
	console->sendCommand("set sv_serverType 0");
	CHECK(gameVar.weapons[WEAPON_SHIELD]->fireDelay == 3.0f);
}

// R2: a rocket whose shooter disconnects while it flies doesn't dereference the missing shooter
static void r2()
{
	harness::tick(400); // r3's rocket is gone
	size_t before = scene->server->game->projectiles.size();
	UINT4 c = harness::spawned(WEAPON_BAZOOKA);
	fireRocket(c, 1); // slow, so it is still flying after the disconnect
	CHECK(scene->server->game->projectiles.size() == before + 1);
	harness::disconnect(c);
	harness::tick(400); // the rocket ends (wall or lifetime) without its shooter: ASan or a crash without the R2 checks
	CHECK(scene->server->game->projectiles.size() == before);
}

// R4: a ban answer from the master for a slot that is empty (or out of range) is ignored, not dereferenced
static void r4()
{
	console->sendCommand("set sv_gamePublic true");
	master->BannedAnswers[0].ID = 20; // no player in slot 20
	master->BannedAnswers[0].Answer = 1;
	master->BannedAnswers[1].ID = 100;
	master->BannedAnswers[1].Answer = 1;
	harness::tick(2);
	CHECK(master->BannedAnswers[0].ID == -1 && master->BannedAnswers[1].ID == -1); // handled
	console->sendCommand("set sv_gamePublic false");
}

// Writes a map file into the pref dir's maps/: a version, then two 16-bit sizes and nothing else
static void writeMap(const char * name, uint32_t version, int16_t x, int16_t y)
{
	std::string path = bv2::mapFileForWrite(name);
	FILE * f = std::fopen(path.c_str(), "wb");
	std::fwrite(&version, 4, 1, f);
	std::fwrite(&x, 2, 1, f);
	std::fwrite(&y, 2, 1, f);
	std::fclose(f);
}

// R5: a map file with an unknown version or an absurd size is invalid, not half-read
static void r5()
{
	writeMap("R5-Version", 99999, 32, 32);
	writeMap("R5-Size", 10010, 30000, -5);
	Map version(CString("R5-Version"), scene->server->game, 0);
	CHECK(!version.isValid);
	Map size(CString("R5-Size"), scene->server->game, 0);
	CHECK(!size.isValid);
}

// R18: a DKT texture whose dimensions ask for gigabytes is skipped, not allocated
static void r18(const char * scratch)
{
	std::string path = std::string(scratch) + "/r18.dkt";
	FILE * f = std::fopen(path.c_str(), "wb");
	auto chunk = [f](uint16_t id) { uint16_t pad = 0; int32_t len = 0; std::fwrite(&id, 2, 1, f); std::fwrite(&pad, 2, 1, f); std::fwrite(&len, 4, 1, f); };
	chunk(0x0000); // version
	int16_t version = 1;
	std::fwrite(&version, 2, 1, f);
	chunk(0x1000); // map
	chunk(0x1100); // map name
	std::fwrite("basemap", 8, 1, f);
	chunk(0x1200); // texture
	chunk(0x1210); // dimension
	int16_t dim[3] = {32767, 32767, 4};
	std::fwrite(dim, 2, 3, f);
	chunk(0x1230); // data
	std::fclose(f);
	ePTexture * texture = new ePTexture(); // not deleted: ~_typLayer calls GL, which the test has no context for
	texture->loadIt((char *)path.c_str());
	CHECK(texture->baseMap.textureID == 0);
}

// R10: clipping a position at the map's edge doesn't read cells outside the map (ASan builds report it)
static void r10()
{
	Map * map = scene->server->game->map;
	for (float x : {0.1f, (float)map->size[0] - 0.1f})
		for (float y : {0.1f, (float)map->size[1] - 0.1f})
		{
			CoordFrame cf;
			cf.position.set(x, y, 0);
			map->collisionClip(cf, .25f);
			CHECK(cf.position[0] > 1 && cf.position[0] < map->size[0] - 1 && cf.position[1] > 1 && cf.position[1] < map->size[1] - 1);
		}
}

// R9: choosing the next map doesn't reload the running map, and an empty rotation keeps the current map
static void r9()
{
	Server * server = scene->server;
	server->mapInfoList.clear(); // every map in mapList is new to the rotation
	server->game->map->flagState[0] = 5; // state that a reload would reset to -2
	CString next = server->queryNextMap();
	CHECK(server->game->map->flagState[0] == 5);
	CHECK(!next.isNull());
	std::vector<CString> maps = server->mapList;
	server->mapList.clear();
	server->mapInfoList.clear();
	CHECK(server->queryNextMap() == server->game->mapName);
	server->mapList = maps;
	server->game->map->flagState[0] = -2;
}

// R13: an SND spawn index equal to the number of spawn points is clamped, not used (ASan builds report it)
static void r13()
{
	Game * game = scene->server->game;
	UINT4 c = harness::spawned(WEAPON_SMG);
	int slot = harness::slotOf(c);
	int gameType = game->gameType;
	std::vector<CVector3f> spawns = game->map->dm_spawns;
	game->gameType = GAME_TYPE_SND;
	game->map->dm_spawns.assign(10, CVector3f(5, 5, 0));
	for (int i = 0; i < 60; ++i)
	{
		game->players[slot]->spawnSlot = 2; // 2*5 + rand(0, 5): index 10 is one past the end
		game->spawnPlayer(slot);
	}
	CHECK(game->players[slot]->status == PLAYER_STATUS_ALIVE);
	game->map->dm_spawns = spawns;
	game->gameType = gameType;
}

// R8: auto-balance moves half the difference, a different player each time, and broadcasts the real team
static void r8()
{
	Game * game = scene->server->game;
	std::vector<int> slots;
	for (int i = 0; i < 6; ++i) slots.push_back(harness::slotOf(harness::join()));
	for (int i = 0; i < MAX_PLAYER; ++i)
		if (game->players[i]) game->players[i]->teamID = PLAYER_TEAM_SPECTATOR;
	for (size_t i = 0; i < slots.size(); ++i) game->players[slots[i]]->teamID = i == 0 ? PLAYER_TEAM_RED : PLAYER_TEAM_BLUE; // 1 red, 5 blue
	harness::sent().clear();
	scene->server->autoBalance();
	int reds = 0, blues = 0;
	for (int s : slots) (game->players[s]->teamID == PLAYER_TEAM_RED ? reds : blues)++;
	CHECK(reds == 3 && blues == 3);
	int moves = 0;
	for (const harness::SentPacket & p : harness::sent())
		if (p.typeID == NET_CLSV_SVCL_TEAM_REQUEST)
		{
			const net_clsv_svcl_team_request * t = (const net_clsv_svcl_team_request *)p.data.data();
			CHECK(game->players[(int)t->playerID]->teamID == t->teamRequested);
			++moves;
		}
	CHECK(moves == 2);
}

// R12: a dropped flag is returned (and taken) within 0.25 for both teams; the blue one used 0.5
static void r12()
{
	Game * game = scene->server->game;
	UINT4 c = harness::spawned(WEAPON_SMG); // blue
	Player * p = game->players[harness::slotOf(c)];
	CHECK(game->gameType == GAME_TYPE_CTF && p->status == PLAYER_STATUS_ALIVE);
	game->map->flagState[0] = -1; // the blue flag lies dropped 0.4 from a blue player
	game->map->flagPos[0] = CVector3f(p->currentCF.position[0] + 0.4f, p->currentCF.position[1], 0);
	scene->server->updateCTF(1 / 30.0f);
	CHECK(game->map->flagState[0] == -1);
	game->map->flagPos[0] = CVector3f(p->currentCF.position[0] + 0.2f, p->currentCF.position[1], 0);
	scene->server->updateCTF(1 / 30.0f);
	CHECK(game->map->flagState[0] == -2); // returned
}

int main(int, char ** argv)
{
	if (!harness::start(argv[0])) return 1;
	r3();
	r2();
	r4();
	r5();
	r10();
	r9();
	r13();
	r8();
	r12();
	r18(bv2::userFile("").c_str());
	harness::stop();
	if (harness::failures() == 0) std::printf("ok: crash fixes\n");
	return harness::failures() ? 1 : 0;
}
