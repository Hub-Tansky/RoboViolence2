// Phase B step 5: one check per fixed defect (KEY_QUESTIONS.md Part D) on the in-process server.
#include "server_harness.h"

#include "CMaster.h"
#include "Console.h"
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

int main(int, char ** argv)
{
	if (!harness::start(argv[0])) return 1;
	r3();
	r2();
	r4();
	r5();
	r10();
	r18(bv2::userFile("").c_str());
	harness::stop();
	if (harness::failures() == 0) std::printf("ok: crash fixes\n");
	return harness::failures() ? 1 : 0;
}
