#include "ServerValidator.h"
#include "Console.h"
#include "GameVar.h"
#include "Scene.h"

#include <cmath>

extern Scene * scene;

namespace
{
	struct RuleSettings
	{
		const char * name;
		int tolerance; // breaks per 3 s window that don't score: lag bunches packets
		bool drops;    // at sv_antiCheat 2 the message is dropped (never a COORD_FRAME: nothing corrects positions)
		float burst, refill; // flood rules: the message budget and its refill per second
	};
	// In Rule order
	const RuleSettings rules[] = {
		{"fire rate", 1, true, 0, 0},
		{"shot with another weapon", 2, true, 0, 0},
		{"shot after death", 2, true, 0, 0},
		{"grenade without ammo", 1, true, 0, 0},
		{"rocket without bazooka", 1, true, 0, 0},
		{"early spawn", 1, true, 0, 0},
		{"chat flood", 0, true, 5, 1},
		{"vote flood", 0, true, 3, 0.2f},
		{"name flood", 0, true, 3, 0.1f},
		{"skin flood", 0, true, 3, 0.1f},
		{"pong flood", 0, true, 3, 2}, // the server pings once a second
		{"team flood", 0, true, 3, 0.2f},
		{"teleport", 0, false, 0, 0},
		{"path speed", 0, false, 0, 0},
		{"wall", 1, false, 0, 0},
	};
	static_assert(sizeof(rules) / sizeof(rules[0]) == ServerValidator::RULE_COUNT, "one setting per rule");

	const float maxSpeed = 3.25f + 0.5f; // the client clamp plus a margin, units per second

	// In the inner half of a wall cell, or off the map. The player's centre stays 0.25 from walls, so an honest
	// position, or a short path cutting a corner, never is.
	bool inWall(const Map & map, float x, float y)
	{
		const int cx = (int)x, cy = (int)y;
		if (x < 0 || y < 0 || cx >= map.size[0] || cy >= map.size[1]) return true;
		const float fx = x - cx, fy = y - cy;
		return !map.cells[cy * map.size[0] + cx].passable && fx > 0.25f && fx < 0.75f && fy > 0.25f && fy < 0.75f;
	}

	bool throughWall(const Map & map, float x0, float y0, float x1, float y1)
	{
		const int steps = (int)(sqrtf((x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0)) / 0.1f) + 1;
		for (int i = 0; i <= steps; ++i)
			if (inWall(map, x0 + (x1 - x0) * i / steps, y0 + (y1 - y0) * i / steps)) return true;
		return false;
	}

	// The client sends a coord frame every max(its ping, sv_minSendInterval) frames
	float sendInterval(const Player & player)
	{
		return fmaxf((float)player.avgPing, (float)gameVar.sv_minSendInterval) / 30.0f;
	}
}

const char * ServerValidator::badSpawnWeapon(const net_clsv_spawn_request & spawn) const
{
	if (!gameVar.sv_validateWeapons) return nullptr;
	switch (spawn.weaponID)
	{
	case WEAPON_SMG: case WEAPON_SHOTGUN: case WEAPON_SNIPER: case WEAPON_DUAL_MACHINE_GUN: case WEAPON_CHAIN_GUN:
	case WEAPON_BAZOOKA: case WEAPON_PHOTON_RIFLE: case WEAPON_FLAME_THROWER:
		break;
	default:
		return "primary";
	}
	switch (spawn.meleeID)
	{
	case WEAPON_KNIVES: case WEAPON_NUCLEAR: case WEAPON_SHIELD: case WEAPON_MINIBOT:
		return nullptr;
	default:
		return "secondary";
	}
}

ServerValidator::Verdict ServerValidator::onSpawnRequest(Player & player)
{
	// The client asks once dead with its timer run out (the server's timer starts earlier), or alive when stuck in a
	// wall (Game.cpp)
	const Map * map = player.game->map;
	const bool early = player.status == PLAYER_STATUS_ALIVE
		? !(map && !map->cells[(int)player.currentCF.position[1] * map->size[0] + (int)player.currentCF.position[0]].passable)
		: player.timeToSpawn > 1.0f;
	return early ? violation(player, SPAWN_EARLY) : ACCEPT;
}

void ServerValidator::onSpawned(Player & player)
{
	slots[(int)player.playerID].track = Track();
}

ServerValidator::Verdict ServerValidator::onShot(Player & player, int weaponID)
{
	Slot & s = slots[(int)player.playerID];
	const int held = player.weapon->weaponID;
	if (weaponID != held && !(weaponID == s.oldWeapon && time - s.pickupTime < 1.0f)) // shots in flight at a pickup
	{
		const Verdict v = violation(player, SHOT_WEAPON);
		if (v != ACCEPT) return v;
	}
	// A budget of 2 s of fire (at least 1.5 s), refilled at the held weapon's rate: a lag burst spends what the stall
	// saved. Shotgun and sniper send one message per pellet.
	const float perVolley = held == WEAPON_SHOTGUN ? 5.0f : held == WEAPON_SNIPER ? 3.0f : 1.0f;
	const float fireDelay = player.weapon->fireDelay;
	s.fireTokens = fminf(perVolley * fmaxf(2.0f, 1.5f / fireDelay), s.fireTokens + (time - s.fireTime) * perVolley / fireDelay);
	s.fireTime = time;
	if (s.fireTokens >= 1)
	{
		s.fireTokens -= 1;
		return ACCEPT;
	}
	return violation(player, FIRE_RATE);
}

void ServerValidator::onPickup(Player & player, int oldWeaponID)
{
	Slot & s = slots[(int)player.playerID];
	s.oldWeapon = oldWeaponID;
	s.pickupTime = time;
}

void ServerValidator::onDeadShot(Player & player)
{
	if (player.timeDead > 2.0f) violation(player, SHOT_DEAD); // a lagged shot arrives within a stall
}

ServerValidator::Verdict ServerValidator::onMove(Player & player, const net_clsv_svcl_player_coord_frame & frame)
{
	const float x = frame.position[0] / 100.0f, y = frame.position[1] / 100.0f;
	Track & t = slots[(int)player.playerID].track;
	// A server-side spawn (search and destroy rounds) shows as timeAlive starting over
	if (t.known && player.timeAlive >= t.timeAlive && player.game->map)
	{
		const float d = sqrtf((x - t.x) * (x - t.x) + (y - t.y) * (y - t.y));
		const bool knockback = player.framesSinceKnockback <= 60; // a hit pushes the player past the speed limit
		Verdict v = ACCEPT;
		// Frames held back by lag arrive together: allow the client's send interval at least
		if (!knockback && d > maxSpeed * fmaxf(time - t.time, sendInterval(player)) + 1.0f)
			v = violation(player, TELEPORT);
		else if (inWall(*player.game->map, x, y) || (d <= 1.0f && throughWall(*player.game->map, t.x, t.y, x, y)))
			v = violation(player, WALL); // a longer path between samples may cut a corner
		if (v == KICKED) return KICKED;
		t.path += d;
	}
	t.known = true;
	t.x = x;
	t.y = y;
	t.time = time;
	t.timeAlive = player.timeAlive;
	return ACCEPT;
}

ServerValidator::Verdict ServerValidator::onCoordFrame(Player & player, const net_clsv_svcl_player_coord_frame & frame)
{
	if (onMove(player, frame) == KICKED) return KICKED;
	if (player.lastFrame == 0) player.lastFrame = frame.frameID;
	player.currentFrame = frame.frameID;
	if (player.frameSinceLast < 90) return ACCEPT;

	// Frame check: client frames elapsed vs server ticks in the same window. Allow 5 frames + 10% for packet jitter
	// and the client's catch-up ticks after a stall; a real speed hack still drifts far beyond that.
	long clientFrames = player.currentFrame - player.lastFrame;
	bool frameHack = clientFrames > player.frameSinceLast + 5 + player.frameSinceLast / 10;

	// Speed check: horizontal speed vs the client clamp (3.25) plus the char*10 quantisation margin. Knockback from a
	// hit legitimately exceeds the clamp, so skip it for 2 s after the server registers one.
	float vx = frame.vel[0] / 10.0f, vy = frame.vel[1] / 10.0f;
	bool speedHack = player.framesSinceKnockback > 60 && sqrtf(vx * vx + vy * vy) > 3.25f + 0.15f;

	// Path speed: the path walked this window against the time it covers (no hit in the window). Frames held back
	// across the window's start cover more client time than server time.
	Slot & s = slots[(int)player.playerID];
	const float seconds = fmaxf((float)player.frameSinceLast, (float)clientFrames) / 30.0f + sendInterval(player);
	const bool pathSpeed = player.framesSinceKnockback > player.frameSinceLast && s.track.path > maxSpeed * seconds + 1.5f;
	s.track.path = 0;
	if (pathSpeed && violation(player, PATH_SPEED) == KICKED) return KICKED;

	s.badWindows = (frameHack || speedHack) && gameVar.sv_antiCheat > 0 ? s.badWindows + 1 : 0;
	if (s.badWindows >= gameVar.sv_speedHackKick) return kick(player, nullptr);
	player.frameSinceLast = player.lastFrame = player.currentFrame = 0; // the next window
	return ACCEPT;
}

void ServerValidator::reset(Player & player)
{
	Slot & s = slots[(int)player.playerID];
	s.badWindows = 0;
	s.track = Track();
	player.frameSinceLast = player.lastFrame = player.currentFrame = 0;
}

void ServerValidator::join(Player & player)
{
	Slot & s = slots[(int)player.playerID];
	s = Slot();
	for (int r = 0; r < RULE_COUNT; ++r) s.tokens[r] = rules[r].burst;
	s.fireTokens = 1000; // full: the first shot caps it to the weapon's budget
	s.fireTime = time;
	reset(player);
}

ServerValidator::Verdict ServerValidator::budget(Player & player, Rule flood)
{
	float & t = slots[(int)player.playerID].tokens[flood];
	if (t >= 1)
	{
		t -= 1;
		return ACCEPT;
	}
	return violation(player, flood);
}

ServerValidator::Verdict ServerValidator::kick(Player & player, const char * why)
{
	if (why) console->sendCommand(CString("sayall %s kicked by the anti-cheat (%s)", player.name.s, why));
	scene->kick(player.playerID);
	return KICKED;
}

ServerValidator::Verdict ServerValidator::violation(Player & player, Rule rule)
{
	if (gameVar.sv_antiCheat == 0) return ACCEPT;
	Slot & s = slots[(int)player.playerID];
	const int breaks = ++s.breaks[rule];
	if (breaks <= rules[rule].tolerance) return ACCEPT;
	if (breaks == rules[rule].tolerance + 1) // once per rule and window: one lag spike is one point
	{
		s.score += 1;
		console->add(CString("\x9> anti-cheat: %s (%s) %s, score %.0f", player.name.s, player.playerIP, rules[rule].name, s.score), true);
	}
	if (gameVar.sv_antiCheat < 2) return ACCEPT;
	if (s.score >= gameVar.sv_antiCheatKick) return kick(player, rules[rule].name);
	if (s.score >= gameVar.sv_antiCheatWarn && !s.warned)
	{
		s.warned = true;
		CString message("Anti-cheat warning: %s", rules[rule].name);
		bb_serverSend(message.s, message.len() + 1, NET_SVCL_CONSOLE, player.babonetID);
	}
	return rules[rule].drops ? DROP : ACCEPT;
}

void ServerValidator::update(float delay)
{
	time += delay;
	window += delay;
	const bool newWindow = window >= 3;
	if (newWindow) window = 0;
	for (Slot & s : slots)
	{
		if (newWindow)
			for (int & b : s.breaks) b = 0;
		for (int r = 0; r < RULE_COUNT; ++r) s.tokens[r] = fminf(rules[r].burst, s.tokens[r] + rules[r].refill * delay);
		s.score = fmaxf(0, s.score - delay / 10);
		if (s.score < gameVar.sv_antiCheatWarn) s.warned = false;
	}
}
