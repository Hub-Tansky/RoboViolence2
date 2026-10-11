// Phase B step 5: one check per fixed defect (KEY_QUESTIONS.md Part D) on the in-process server.
#include "server_harness.h"

#include "Console.h"
#include "GameVar.h"
#include "Scene.h"
#include "Server.h"

#include <cstdio>
#include <cstring>

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

template <typename T> static void deliver(UINT4 from, int typeID, const T & m)
{
	harness::deliver(from, typeID, &m, sizeof(m));
}

// A joined client that has spawned with `weaponID`; returns its babonetID
static UINT4 spawned(int weaponID)
{
	UINT4 c = harness::join();
	net_clsv_svcl_team_request team = {};
	team.teamRequested = PLAYER_TEAM_BLUE;
	deliver(c, NET_CLSV_SVCL_TEAM_REQUEST, team);
	net_clsv_spawn_request spawn = {};
	spawn.weaponID = (int8_t)weaponID;
	spawn.meleeID = WEAPON_KNIVES;
	deliver(c, NET_CLSV_SPAWN_REQUEST, spawn);
	harness::tick(60); // past the fire delay
	return c;
}

// R3 (ADR 0013): a rocket in flight no longer switches a Normal server to the Pro rules
static void r3()
{
	console->sendCommand("set sv_serverType 0");
	UINT4 c = spawned(WEAPON_BAZOOKA);
	Player * p = scene->server->game->players[harness::slotOf(c)];
	CHECK(p && p->status == PLAYER_STATUS_ALIVE);
	net_clsv_svcl_player_projectile rocket = {};
	rocket.weaponID = WEAPON_BAZOOKA;
	rocket.projectileType = PROJECTILE_ROCKET;
	rocket.position[0] = (short)(p->currentCF.position[0] * 100);
	rocket.position[1] = (short)(p->currentCF.position[1] * 100);
	rocket.position[2] = (short)(p->currentCF.position[2] * 100);
	rocket.vel[0] = 10;
	deliver(c, NET_CLSV_SVCL_PLAYER_PROJECTILE, rocket);
	CHECK(!scene->server->game->projectiles.empty());
	harness::tick(10);
	CHECK(gameVar.sv_serverType == 0);
}

int main(int, char ** argv)
{
	if (!harness::start(argv[0])) return 1;
	r3();
	harness::stop();
	if (failures == 0) std::printf("ok: crash fixes\n");
	return failures ? 1 : 0;
}
