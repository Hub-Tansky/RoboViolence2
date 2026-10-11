// Phase B step 8: the server's cheat checks kick as they did before ServerValidator (8.1 pins this behaviour).
#include "server_harness.h"

#include "Console.h"
#include "Game.h"
#include "Map.h"
#include "Scene.h"
#include "Server.h"

#include <cstdio>
#include <cmath>
#include <cstring>
#include <vector>

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

static float score(UINT4 c) { return scene->server->validator.scoreOf(harness::slotOf(c)); }

static void leave(UINT4 c)
{
	harness::disconnect(c);
	harness::tick();
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

// 9.1 sv_antiCheat: 0 nothing (and no step 8 speed kick), 1 scores and logs only, 2 drops, warns, then kicks. A
// rule scores at most once per 3 s window.
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
	window(c, 50); // bad windows at 0 don't count once it is back on
	CHECK(scene->server->game->players[slot] != nullptr);
	live(c, 3); // a new window
	bool accepted = true;
	for (int i = 0; i < 20; ++i) accepted &= v.violation(*player(c), ServerValidator::FLOOD_CHAT) == ServerValidator::ACCEPT;
	CHECK(accepted && v.scoreOf(slot) == 1 && scene->server->game->players[slot] != nullptr);
	live(c, 5); // decays 1 point per 10 s
	CHECK(v.scoreOf(slot) > 0.4f && v.scoreOf(slot) < 0.6f);
	CHECK(v.violation(*player(c), ServerValidator::WALL) == ServerValidator::ACCEPT && v.scoreOf(slot) < 0.6f); // tolerated
	leave(c);

	console->sendCommand("set sv_antiCheat 2");
	console->sendCommand("set sv_antiCheatWarn 1");
	console->sendCommand("set sv_antiCheatKick 2"); // reached in the third window: 1, 1.7, 2.4 with the decay
	c = harness::spawned(WEAPON_SMG);
	slot = harness::slotOf(c);
	CHECK(v.scoreOf(slot) == 0); // a new player starts clean
	harness::sent().clear();
	ServerValidator::Verdict verdicts[3];
	for (ServerValidator::Verdict & verdict : verdicts)
	{
		verdict = v.violation(*player(c), ServerValidator::FLOOD_CHAT);
		if (verdict != ServerValidator::KICKED) live(c, 3);
	}
	int warnings = 0;
	for (const harness::SentPacket & p : harness::sent())
		warnings += p.typeID == NET_SVCL_CONSOLE && p.destination == (INT4)c && strstr(p.data.data(), "Anti-cheat warning");
	CHECK(verdicts[0] == ServerValidator::DROP && verdicts[1] == ServerValidator::DROP && warnings >= 1);
	CHECK(verdicts[2] == ServerValidator::KICKED && scene->server->game->players[slot] == nullptr);
	leave(c);
	console->sendCommand("set sv_antiCheatWarn 5");
	console->sendCommand("set sv_antiCheatKick 10");
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

// 9.2 weapon rules: each cheat scores, normal play (with jitter and lag) does not
static void weaponRules()
{
	// Fire rate: honest SMG shots 0.067-0.133 s apart (fireDelay 0.1), then a 1 s stall's ten shots in one frame;
	// then three times the rate for 2 s
	UINT4 c = harness::spawned(WEAPON_SMG);
	for (int i = 0; i < 30; ++i)
	{
		shoot(c, WEAPON_SMG);
		harness::tick(i % 2 ? 1 : 3); // with shoot()'s own frame: 0.067 or 0.133 s apart
	}
	live(c, 1);
	shoot(c, WEAPON_SMG, 10);
	CHECK(score(c) == 0);
	for (int i = 0; i < 60; ++i) shoot(c, WEAPON_SMG);
	CHECK(score(c) > 0.5f);
	leave(c);

	// A shot with a weapon the player doesn't hold; shots in flight with the weapon a pickup just replaced are fine
	c = harness::spawned(WEAPON_SMG);
	scene->server->validator.onPickup(*player(c), WEAPON_SNIPER);
	shoot(c, WEAPON_SNIPER, 3);
	CHECK(score(c) == 0);
	harness::tick(30);
	shoot(c, WEAPON_SHOTGUN, 3);
	CHECK(score(c) > 0.5f);
	leave(c);

	// Shots long after death; ones up to 2 s after death are lag
	c = harness::spawned(WEAPON_SMG);
	player(c)->kill(true);
	player(c)->timeDead = 0;
	shoot(c, WEAPON_SMG, 3);
	live(c, 1);
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
	live(c, 4); // the grenades go off here, not on the next test's player
	leave(c);

	// Rockets: from a bazooka, fine; without one, not
	c = harness::spawned(WEAPON_BAZOOKA);
	throwProjectile(c, WEAPON_BAZOOKA, PROJECTILE_ROCKET, 2);
	CHECK(score(c) == 0);
	live(c, 4);
	leave(c);
	c = harness::spawned(WEAPON_SMG);
	throwProjectile(c, WEAPON_BAZOOKA, PROJECTILE_ROCKET, 2);
	CHECK(score(c) > 0.5f);
	leave(c);

	// Spawn requests: once dead with the timer run out, and alive when stuck in a wall, are fine; alive otherwise not
	c = harness::spawned(WEAPON_SMG);
	live(c, 3); // a new tolerance window (spawned() itself spawns early)
	player(c)->kill(true);
	player(c)->timeToSpawn = 0;
	net_clsv_spawn_request spawn = {};
	spawn.weaponID = WEAPON_SMG;
	spawn.meleeID = WEAPON_KNIVES;
	harness::deliver(c, NET_CLSV_SPAWN_REQUEST, spawn);
	CHECK(player(c)->status == PLAYER_STATUS_ALIVE && score(c) == 0);
	Map & map = *scene->server->game->map;
	map_cell & cell = map.cells[(int)player(c)->currentCF.position[1] * map.size[0] + (int)player(c)->currentCF.position[0]];
	cell.passable = false;
	harness::queueMessage(c, NET_CLSV_SPAWN_REQUEST, &spawn, sizeof(spawn));
	harness::queueMessage(c, NET_CLSV_SPAWN_REQUEST, &spawn, sizeof(spawn));
	harness::tick();
	cell.passable = true;
	CHECK(score(c) == 0);
	live(c, 3);
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
	console->sendCommand("set sv_antiCheat 1");
	CHECK(fired[1] > 0 && fired[2] < fired[1]);
}

// A coord frame at (x, y), client frame `frame`, queued for the next server frame
static void queueFrame(UINT4 c, float x, float y, int frame)
{
	net_clsv_svcl_player_coord_frame cf = {};
	cf.position[0] = (short)(x * 100);
	cf.position[1] = (short)(y * 100);
	cf.position[2] = (short)(player(c)->currentCF.position[2] * 100);
	cf.babonetID = (int32_t)c;
	cf.frameID = frame;
	cf.camPosZ = 7;
	harness::queueMessage(c, NET_CLSV_SVCL_PLAYER_COORD_FRAME, &cf, sizeof(cf));
}

static void pong(UINT4 c)
{
	net_clsv_pong pong = {};
	pong.playerID = (char)harness::slotOf(c);
	harness::queueMessage(c, NET_CLSV_PONG, &pong, sizeof(pong));
}

// Back and forth along x around x0 (`reach` each way) at `speed` units/s for `seconds`, one server frame per client
// frame. The client sends a frame every `every` client frames; lag holds them back and delivers `burst` at a time.
// With `lag`, the client answers each ping `lag` frames late (the server's ping), else once a second. `start`: the
// client frame a previous walk ended on, to go on from where it stopped.
static void walk(UINT4 c, float x0, float y0, float speed, float seconds, int every, int burst, float reach = 0.4f,
	int lag = 0, int start = 0)
{
	struct Held { float x; int frame; };
	std::vector<Held> held;
	for (int frame = start + 1; frame <= start + (int)(seconds * 30); ++frame)
	{
		if (frame % every == 0)
		{
			float d = fmodf(speed * frame / 30.0f, 4 * reach); // a round trip
			held.push_back({x0 - reach + (d < 2 * reach ? d : 4 * reach - d), frame});
		}
		if ((int)held.size() == burst)
		{
			for (const Held & h : held) queueFrame(c, h.x, y0, h.frame);
			held.clear();
		}
		Player * p = player(c);
		if (lag ? p->waitForPong && p->currentPingFrame >= lag : frame % 30 == 0) pong(c);
		harness::tick();
	}
}

// The middle of the map's longest open row, and how far a player can walk each way from it
static void longestRun(float & x0, float & y0, float & reach)
{
	const Map & map = *scene->server->game->map;
	int best = 0;
	for (int y = 1; y < map.size[1] - 1; ++y)
		for (int x = 1, run = 0; x < map.size[0] - 1; ++x)
		{
			run = map.cells[y * map.size[0] + x].passable ? run + 1 : 0;
			if (run > best) best = run, x0 = x + 1 - run / 2.0f, y0 = y + 0.5f;
		}
	reach = best / 2.0f - 0.5f;
}

// 9.3 movement rules
static void movementRules()
{
	// Honest walking at the client clamp (3.25 units/s), frames bunched by lag: nothing. Then a 10-unit jump in one frame: teleport.
	UINT4 c = harness::spawned(WEAPON_SMG);
	float x0 = player(c)->currentCF.position[0], y0 = player(c)->currentCF.position[1];
	walk(c, x0, y0, 3.25f, 4, 3, 4);
	CHECK(score(c) == 0);
	queueFrame(c, x0 + 10, y0, 200); // the test's pongs come late: the server sees up to 1 s of ping (send interval)
	harness::tick();
	CHECK(score(c) > 0.5f);
	leave(c);

	// Twice the speed in small steps: no single jump, but the window's path is too long
	c = harness::spawned(WEAPON_SMG);
	x0 = player(c)->currentCF.position[0], y0 = player(c)->currentCF.position[1];
	walk(c, x0, y0, 7, 4, 1, 1);
	CHECK(score(c) > 0.5f);
	leave(c);

	// Along the longest open row: high ping (a frame every 15 client frames, the server's ping 15, two frames at a
	// time): nothing. Frames held back 1 s across a window start: nothing.
	float reach;
	longestRun(x0, y0, reach);
	CHECK(reach >= 1.5f);
	c = harness::spawned(WEAPON_SMG);
	walk(c, x0, y0, 0, 3, 1000, 1, reach, 15); // standing: the server measures the 15-frame ping
	walk(c, x0, y0, 3.25f, 6, 15, 2, reach, 15);
	walk(c, x0, y0, 3.25f, 7, 3, 10, reach, 0, 180);
	CHECK(score(c) == 0);
	leave(c);

	// Walls: the test makes a one-cell wall in an open 3x3 block (the placeholder map has none). Round its corner in
	// high-ping steps: nothing. Through it in short steps, twice: wall.
	c = harness::spawned(WEAPON_SMG);
	Map & map = *scene->server->game->map;
	auto open = [&](int x, int y) { return map.cells[y * map.size[0] + x].passable; };
	int wx = -1, wy = -1;
	for (int y = 2; y < map.size[1] - 2 && wx < 0; ++y)
		for (int x = 2; x < map.size[0] - 2 && wx < 0; ++x)
		{
			bool all = true;
			for (int k = 0; k < 9; ++k) all &= open(x - 1 + k % 3, y - 1 + k / 3);
			if (all) wx = x, wy = y;
		}
	CHECK(wx > 0);
	map_cell & wall = map.cells[wy * map.size[0] + wx];
	wall.passable = false;
	// 0.25 from the wall all round it, each corner cut between samples (an honest player rounding it); then across a
	// corner with a second between samples (a high ping)
	const float corner[][2] = {{-0.25f, 0.6f}, {0.4f, 1.25f}, {0.6f, 1.25f}, {1.25f, 0.6f}, {1.25f, 0.4f}, {0.6f, -0.25f},
		{0.4f, -0.25f}, {-0.25f, 0.4f}};
	for (int i = 0; i < 8; ++i)
	{
		queueFrame(c, wx + corner[i][0], wy + corner[i][1], 100 + i * 10);
		harness::tick(10);
	}
	for (int i = 0; i < 3; ++i)
	{
		queueFrame(c, wx + (i % 2 ? 1.25f : -0.25f), wy + (i % 2 ? 1.25f : -0.25f), 200 + i * 30);
		harness::tick(30);
	}
	CHECK(score(c) == 0);
	const float through[] = {-0.4f, 0.5f, 1.4f, 0.5f, -0.4f};
	for (int i = 0; i < 5; ++i)
	{
		queueFrame(c, wx + through[i], wy + 0.5f, 300 + i * 15);
		harness::tick(15);
	}
	CHECK(score(c) > 0.5f);
	wall.passable = true;
	leave(c);

	// sv_antiCheat 2: a client that keeps teleporting is kicked through the coord frame handler, and the server goes on
	console->sendCommand("set sv_antiCheat 2");
	c = harness::spawned(WEAPON_SMG);
	int slot = harness::slotOf(c);
	x0 = player(c)->currentCF.position[0], y0 = player(c)->currentCF.position[1];
	for (int frame = 1; frame <= 90 * 30 && harness::slotOf(c) == slot; ++frame)
	{
		queueFrame(c, x0 + (frame % 2) * 5, y0, frame);
		if (frame % 30 == 0) pong(c);
		harness::tick();
	}
	CHECK(scene->server->game->players[slot] == nullptr);
	harness::tick(5);
	leave(c);
	console->sendCommand("set sv_antiCheat 1");

	// A respawn starts over: the first frame after it is far from the last one before death
	c = harness::spawned(WEAPON_SMG);
	x0 = player(c)->currentCF.position[0], y0 = player(c)->currentCF.position[1];
	queueFrame(c, x0, y0, 10);
	harness::tick(31);
	player(c)->kill(true);
	player(c)->timeToSpawn = 0;
	net_clsv_spawn_request spawn = {};
	spawn.weaponID = WEAPON_SMG;
	spawn.meleeID = WEAPON_KNIVES;
	harness::deliver(c, NET_CLSV_SPAWN_REQUEST, spawn);
	queueFrame(c, x0 + 10, y0, 20);
	harness::tick();
	CHECK(score(c) == 0);
	leave(c);
}

// `count` zeroed messages of a type in one server frame
static void burstOf(UINT4 c, int typeID, int size, int count)
{
	std::vector<char> zero(size, 0);
	for (int i = 0; i < count; ++i) harness::queueMessage(c, typeID, zero.data(), size);
	harness::tick();
}

static int sentOfType(int typeID)
{
	int n = 0;
	for (const harness::SentPacket & p : harness::sent()) n += p.typeID == typeID;
	return n;
}

// 9.4 rate limits: a burst past each type's budget scores; chat at a sane rate does not; level 2 drops the excess
static void rateLimits()
{
	struct { int typeID, size, burst; } types[] = {
		{NET_CLSV_SVCL_CHAT, (int)sizeof(net_clsv_svcl_chat), 5},
		{NET_CLSV_VOTE, (int)sizeof(net_clsv_vote), 3},
		{NET_CLSV_SVCL_VOTE_REQUEST, (int)sizeof(net_clsv_svcl_vote_request), 3},
		{NET_CLSV_SVCL_PLAYER_CHANGE_NAME, (int)sizeof(net_clsv_svcl_player_change_name), 3},
		{NET_CLSV_SVCL_PLAYER_UPDATE_SKIN, (int)sizeof(net_clsv_svcl_player_update_skin), 3},
		{NET_CLSV_PONG, (int)sizeof(net_clsv_pong), 3},
		{NET_CLSV_SVCL_TEAM_REQUEST, (int)sizeof(net_clsv_svcl_team_request), 3},
	};
	for (const auto & t : types)
	{
		UINT4 c = harness::spawned(WEAPON_SMG);
		live(c, 3); // budgets refilled
		burstOf(c, t.typeID, t.size, t.burst - 1); // within the budget (live() used a pong)
		CHECK(score(c) == 0);
		burstOf(c, t.typeID, t.size, 3);
		if (score(c) <= 0.5f) std::printf("FAIL rate limit for message %i\n", t.typeID), ++harness::failures();
		leave(c);
	}

	UINT4 c = harness::spawned(WEAPON_SMG);
	for (int i = 0; i < 10; ++i)
	{
		burstOf(c, NET_CLSV_SVCL_CHAT, sizeof(net_clsv_svcl_chat), 1);
		live(c, 1); // a line a second
	}
	CHECK(score(c) == 0);
	leave(c);

	int relayed[3] = {};
	for (int level = 1; level <= 2; ++level)
	{
		console->sendCommand(CString("set sv_antiCheat %i", level));
		c = harness::spawned(WEAPON_SMG);
		harness::sent().clear();
		net_clsv_svcl_chat chat = {};
		chat.teamID = -2; // everyone
		for (int i = 0; i < 8; ++i) harness::queueMessage(c, NET_CLSV_SVCL_CHAT, &chat, sizeof(chat));
		harness::tick();
		relayed[level] = sentOfType(NET_CLSV_SVCL_CHAT);
		leave(c);
	}
	console->sendCommand("set sv_antiCheat 1");
	CHECK(relayed[1] == 8 && relayed[2] == 5);
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
	movementRules();
	rateLimits();
	harness::stop();
	if (harness::failures() == 0) std::printf("ok: validator\n");
	return harness::failures() ? 1 : 0;
}
