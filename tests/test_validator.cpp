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

// `seconds` of server frames; the client answers pings (the server drops it after 3 s without a pong)
static void live(UINT4 c, int seconds)
{
	for (int i = 0; i < seconds; ++i)
	{
		net_clsv_pong pong = {};
		pong.playerID = (char)harness::slotOf(c);
		harness::deliver(c, NET_CLSV_PONG, pong);
		harness::tick(30);
	}
}

// One 3 s check window: a coord frame at its start and one at its end, `frames` client frames later, horizontal
// speed vel/10. knockback: hit just before the end.
static void window(UINT4 c, int vel, int frames = 90, bool knockback = false)
{
	if (harness::slotOf(c) == -1) return; // kicked
	coordFrame(c, 20, 1000);
	live(c, 3);
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

// 9.1 sv_antiCheat: 0 nothing (and no step 8 speed kick), 1 scores and logs only, 2 drops, warns, then kicks
static void antiCheatLevels()
{
	ServerValidator & v = scene->server->validator;
	UINT4 c = harness::spawned(WEAPON_SMG);
	int slot = harness::slotOf(c);
	console->sendCommand("set sv_antiCheat 0");
	CHECK(v.violation(*player(c), ServerValidator::FLOOD_CHAT) == ServerValidator::ACCEPT && v.scoreOf(slot) == 0);
	for (int i = 0; i < 4; ++i) window(c, 50);
	CHECK(scene->server->game->players[slot] != nullptr); // speed kick off

	console->sendCommand("set sv_antiCheat 1");
	bool accepted = true;
	for (int i = 0; i < 20; ++i) accepted &= v.violation(*player(c), ServerValidator::FLOOD_CHAT) == ServerValidator::ACCEPT;
	CHECK(accepted && v.scoreOf(slot) >= 19 && scene->server->game->players[slot] != nullptr);
	live(c, 10); // decays 1 point per 10 s
	CHECK(v.scoreOf(slot) < 19.5f && v.scoreOf(slot) > 17);
	CHECK(v.violation(*player(c), ServerValidator::FIRE_RATE) == ServerValidator::ACCEPT); // within tolerance: no score
	harness::disconnect(c);
	harness::tick();

	console->sendCommand("set sv_antiCheat 2");
	c = harness::spawned(WEAPON_SMG);
	slot = harness::slotOf(c);
	CHECK(v.scoreOf(slot) == 0); // a new player starts clean
	harness::sent().clear();
	bool dropped = true;
	for (int i = 0; i < 5; ++i) dropped &= v.violation(*player(c), ServerValidator::FLOOD_CHAT) == ServerValidator::DROP;
	int warnings = 0;
	for (const harness::SentPacket & p : harness::sent())
		warnings += p.typeID == NET_SVCL_CONSOLE && p.destination == (INT4)c && strstr(p.data.data(), "Anti-cheat warning");
	CHECK(dropped && warnings == 1);
	for (int i = 0; i < 4; ++i) v.violation(*player(c), ServerValidator::FLOOD_CHAT);
	CHECK(v.violation(*player(c), ServerValidator::FLOOD_CHAT) == ServerValidator::KICK);
	CHECK(scene->server->game->players[slot] == nullptr);
	harness::disconnect(c);
	harness::tick();
	console->sendCommand("set sv_antiCheat 1");
}

static void shoot(UINT4 c, int weaponID, int count = 1)
{
	Player * p = player(c);
	net_clsv_player_shoot shot = {};
	shot.weaponID = (char)weaponID;
	for (int k = 0; k < 2; ++k) shot.p1[k] = (short)(p->currentCF.position[k] * 100), shot.p2[k] = shot.p1[k] + 300;
	for (int i = 0; i < count; ++i) harness::queueMessage(c, NET_CLSV_PLAYER_SHOOT, &shot, sizeof(shot)); // one frame
	harness::tick();
}

static void throwProjectile(UINT4 c, int weaponID, int type, int count = 1)
{
	Player * p = player(c);
	net_clsv_svcl_player_projectile projectile = {};
	projectile.weaponID = (char)weaponID;
	projectile.projectileType = (char)type;
	for (int k = 0; k < 3; ++k) projectile.position[k] = (short)(p->currentCF.position[k] * 100);
	projectile.vel[0] = 10;
	for (int i = 0; i < count; ++i) harness::queueMessage(c, NET_CLSV_SVCL_PLAYER_PROJECTILE, &projectile, sizeof(projectile));
	harness::tick();
}

static float score(UINT4 c) { return scene->server->validator.scoreOf(harness::slotOf(c)); }

static void leave(UINT4 c)
{
	harness::disconnect(c);
	harness::tick();
}

// 9.2 weapon rules: each cheat scores, normal play (with jitter) does not
static void weaponRules()
{
	// Fire rate: ten SMG shots in one frame; honest shots 0.067-0.133 s apart (fireDelay 0.1)
	UINT4 c = harness::spawned(WEAPON_SMG);
	for (int i = 0; i < 30; ++i)
	{
		shoot(c, WEAPON_SMG);
		harness::tick(i % 2 ? 1 : 3); // with shoot()'s own frame: 0.067 or 0.133 s apart
	}
	CHECK(score(c) == 0);
	shoot(c, WEAPON_SMG, 10);
	CHECK(score(c) > 3.5f); // 9 drops, 5 tolerated
	leave(c);

	// A shot with a weapon the player doesn't hold
	c = harness::spawned(WEAPON_SMG);
	harness::tick(30);
	shoot(c, WEAPON_SNIPER, 3);
	CHECK(score(c) > 0.5f);
	leave(c);

	// Shots long after death; one just after death is lag
	c = harness::spawned(WEAPON_SMG);
	player(c)->kill(true);
	player(c)->timeDead = 0;
	shoot(c, WEAPON_SMG, 3);
	CHECK(score(c) == 0);
	live(c, 2);
	shoot(c, WEAPON_SMG, 3);
	CHECK(score(c) > 0.5f);
	leave(c);

	// Grenades: the two a player starts with, then more
	c = harness::spawned(WEAPON_SMG);
	throwProjectile(c, WEAPON_GRENADE, PROJECTILE_GRENADE, 2);
	CHECK(score(c) == 0);
	throwProjectile(c, WEAPON_GRENADE, PROJECTILE_GRENADE, 2);
	CHECK(score(c) > 0.5f);
	leave(c);

	// A rocket without a bazooka
	c = harness::spawned(WEAPON_SMG);
	throwProjectile(c, WEAPON_BAZOOKA, PROJECTILE_ROCKET, 2);
	CHECK(score(c) > 0.5f);
	leave(c);

	// Spawn requests while alive; one once dead with the timer run out is fine
	c = harness::spawned(WEAPON_SMG);
	live(c, 3); // a new tolerance window (spawned() itself spawns early)
	player(c)->kill(true);
	player(c)->timeToSpawn = 0;
	net_clsv_spawn_request spawn = {};
	spawn.weaponID = WEAPON_SMG;
	spawn.meleeID = WEAPON_KNIVES;
	harness::deliver(c, NET_CLSV_SPAWN_REQUEST, spawn);
	CHECK(player(c)->status == PLAYER_STATUS_ALIVE && score(c) == 0);
	harness::queueMessage(c, NET_CLSV_SPAWN_REQUEST, &spawn, sizeof(spawn));
	harness::queueMessage(c, NET_CLSV_SPAWN_REQUEST, &spawn, sizeof(spawn));
	harness::tick();
	CHECK(score(c) > 0.5f);
	leave(c);

	// sv_antiCheat 2 drops the message: of three shots with a weapon the player doesn't hold (in one frame), the one
	// past the tolerance is not fired; at 1 all are
	int fired[3] = {};
	for (int level = 1; level <= 2; ++level)
	{
		console->sendCommand(CString("set sv_antiCheat %i", level));
		c = harness::spawned(WEAPON_SMG);
		harness::tick(90);
		harness::sent().clear();
		shoot(c, WEAPON_SNIPER, 3);
		for (const harness::SentPacket & p : harness::sent()) fired[level] += p.typeID == NET_SVCL_PLAYER_SHOOT;
		leave(c);
	}
	CHECK(fired[1] > 0 && fired[2] < fired[1]);
	console->sendCommand("set sv_antiCheat 1");
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
	antiCheatLevels();
	weaponRules();
	harness::stop();
	if (harness::failures() == 0) std::printf("ok: validator\n");
	return harness::failures() ? 1 : 0;
}
