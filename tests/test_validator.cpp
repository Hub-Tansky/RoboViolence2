// Phase B step 8: the server's cheat checks kick as they did before ServerValidator (8.1 pins this behaviour).
#include "server_harness.h"

#include "Console.h"
#include "Game.h"
#include "Scene.h"
#include "Server.h"

#include <cstdio>

extern Scene * scene;

static Player * player(UINT4 c) { return scene->server->game->players[harness::slotOf(c)]; }

// A spawn request with a known weapon that is not a primary (or melee) weapon kicks
static void weaponWhitelist()
{
	const int bad[][2] = {{WEAPON_GRENADE, WEAPON_KNIVES}, {WEAPON_SMG, WEAPON_SMG}};
	for (const auto & w : bad)
	{
		UINT4 c = harness::spawned(WEAPON_SMG);
		int slot = harness::slotOf(c);
		net_clsv_spawn_request spawn = {};
		spawn.weaponID = (char)w[0];
		spawn.meleeID = (char)w[1];
		harness::deliver(c, NET_CLSV_SPAWN_REQUEST, spawn);
		harness::tick();
		CHECK(scene->server->game->players[slot] == nullptr);
		harness::disconnect(c);
		harness::tick();
	}
}

// One coord frame per 3 s check window, horizontal speed vel/10. The client answers pings (the server drops it after
// 3 s without a pong).
static void window(UINT4 c, int vel)
{
	for (int i = 0; i < 3; ++i)
	{
		net_clsv_pong pong = {};
		pong.playerID = (char)harness::slotOf(c);
		harness::deliver(c, NET_CLSV_PONG, pong);
		harness::tick(30);
	}
	Player * p = player(c);
	net_clsv_svcl_player_coord_frame cf = {};
	for (int k = 0; k < 3; ++k) cf.position[k] = (short)(p->currentCF.position[k] * 100);
	cf.vel[0] = (char)vel;
	cf.babonetID = (int32_t)c;
	cf.frameID = 1000;
	cf.camPosZ = 7;
	harness::deliver(c, NET_CLSV_SVCL_PLAYER_COORD_FRAME, cf);
	harness::tick();
}

// Three windows in a row over the speed limit kick; a clean window in between resets the count
static void speedHack()
{
	UINT4 c = harness::spawned(WEAPON_SMG);
	int slot = harness::slotOf(c);
	window(c, 50);
	window(c, 50);
	window(c, 20); // clean: the count restarts
	window(c, 50);
	window(c, 50);
	CHECK(scene->server->game->players[slot] != nullptr);
	window(c, 50);
	CHECK(scene->server->game->players[slot] == nullptr);
	harness::disconnect(c);
	harness::tick();
}

// A player at normal speed with valid weapons is never kicked
static void validPlayer()
{
	UINT4 c = harness::spawned(WEAPON_SHOTGUN);
	int slot = harness::slotOf(c);
	for (int i = 0; i < 6; ++i) window(c, 30);
	net_clsv_spawn_request spawn = {};
	spawn.weaponID = WEAPON_SNIPER;
	spawn.meleeID = WEAPON_SHIELD;
	harness::deliver(c, NET_CLSV_SPAWN_REQUEST, spawn);
	harness::tick();
	CHECK(scene->server->game->players[slot] != nullptr);
	harness::disconnect(c);
	harness::tick();
}

// sv_speedHackKick sets how many violating windows in a row kick
static void speedHackKickVar()
{
	console->sendCommand("set sv_speedHackKick 1");
	UINT4 c = harness::spawned(WEAPON_SMG);
	int slot = harness::slotOf(c);
	window(c, 50);
	CHECK(scene->server->game->players[slot] == nullptr);
	harness::disconnect(c);
	harness::tick();
	console->sendCommand("set sv_speedHackKick 3");
}

int main(int, char ** argv)
{
	if (!harness::start(argv[0])) return 1;
	weaponWhitelist();
	speedHack();
	validPlayer();
	speedHackKickVar();
	harness::stop();
	if (harness::failures() == 0) std::printf("ok: validator\n");
	return harness::failures() ? 1 : 0;
}
