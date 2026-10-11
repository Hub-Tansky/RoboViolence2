// Phase B step 8: the server's cheat checks kick as they did before ServerValidator (8.1 pins this behaviour).
#include "server_harness.h"

#include "Console.h"
#include "Game.h"
#include "Scene.h"
#include "Server.h"

#include <cstdio>
#include <cstring>

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
		harness::sent().clear();
		harness::deliver(c, NET_CLSV_SPAWN_REQUEST, spawn);
		harness::tick();
		CHECK(scene->server->game->players[slot] == nullptr);
		bool told = false; // the kick message names the weapon
		for (const harness::SentPacket & p : harness::sent())
			if (p.typeID == NET_CLSV_SVCL_CHAT)
				told |= strstr(((const net_clsv_svcl_chat *)p.data.data())->message, w[0] == WEAPON_SMG ? "his secondary weapon" : "his primary weapon") != nullptr;
		CHECK(told);
		harness::disconnect(c);
		harness::tick();
	}
}

static void coordFrame(UINT4 c, int vel, int frameID)
{
	Player * p = player(c);
	net_clsv_svcl_player_coord_frame cf = {};
	for (int k = 0; k < 3; ++k) cf.position[k] = (short)(p->currentCF.position[k] * 100);
	cf.vel[0] = (char)vel;
	cf.babonetID = (int32_t)c;
	cf.frameID = frameID;
	cf.camPosZ = 7;
	harness::deliver(c, NET_CLSV_SVCL_PLAYER_COORD_FRAME, cf);
	harness::tick();
}

// One 3 s check window: a coord frame at its start and one at its end, `frames` client frames later, horizontal
// speed vel/10. The client answers pings (the server drops it after 3 s without a pong). knockback: hit just before
// the end.
static void window(UINT4 c, int vel, int frames = 90, bool knockback = false)
{
	if (harness::slotOf(c) == -1) return; // kicked
	coordFrame(c, 20, 1000);
	for (int i = 0; i < 3; ++i)
	{
		net_clsv_pong pong = {};
		pong.playerID = (char)harness::slotOf(c);
		harness::deliver(c, NET_CLSV_PONG, pong);
		harness::tick(30);
	}
	if (knockback) player(c)->framesSinceKnockback = 0;
	coordFrame(c, vel, 1000 + frames);
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

// More client frames than server ticks in a window (a sped-up client) at a legal speed: three in a row kick
static void frameHack()
{
	UINT4 c = harness::spawned(WEAPON_SMG);
	int slot = harness::slotOf(c);
	window(c, 20, 200);
	window(c, 20, 200);
	CHECK(scene->server->game->players[slot] != nullptr);
	window(c, 20, 200);
	CHECK(scene->server->game->players[slot] == nullptr);
	harness::disconnect(c);
	harness::tick();
}

// Knockback from a hit exceeds the speed limit: no violation within 2 s of a hit
static void knockback()
{
	UINT4 c = harness::spawned(WEAPON_SMG);
	int slot = harness::slotOf(c);
	for (int i = 0; i < 3; ++i) window(c, 50, 90, true);
	CHECK(scene->server->game->players[slot] != nullptr);
	harness::disconnect(c);
	harness::tick();
}

// The count restarts while the player is dead, and for a new player in the slot
static void resets()
{
	UINT4 c = harness::spawned(WEAPON_SMG);
	int slot = harness::slotOf(c);
	window(c, 50);
	window(c, 50);
	player(c)->status = PLAYER_STATUS_DEAD;
	coordFrame(c, 20, 2000);
	player(c)->status = PLAYER_STATUS_ALIVE;
	window(c, 50);
	window(c, 50);
	CHECK(scene->server->game->players[slot] != nullptr);
	harness::disconnect(c);
	harness::tick();

	c = harness::spawned(WEAPON_SMG);
	slot = harness::slotOf(c);
	window(c, 50);
	window(c, 50);
	harness::disconnect(c);
	harness::tick();
	UINT4 d = harness::spawned(WEAPON_SMG);
	CHECK(harness::slotOf(d) == slot);
	window(d, 50);
	window(d, 50);
	CHECK(scene->server->game->players[slot] != nullptr);
	harness::disconnect(d);
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
	frameHack();
	knockback();
	resets();
	speedHackKickVar();
	harness::stop();
	if (harness::failures() == 0) std::printf("ok: validator\n");
	return harness::failures() ? 1 : 0;
}
