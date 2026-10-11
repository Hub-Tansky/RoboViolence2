#include "ServerValidator.h"
#include "Console.h"
#include "GameVar.h"
#include "Scene.h"

extern Scene * scene;

namespace
{
	const char * const ruleName[ServerValidator::RULE_COUNT] = {"fire rate", "shot with another weapon",
		"shot after death", "grenade without ammo", "rocket without bazooka", "early spawn", "chat flood", "vote flood",
		"name flood", "skin flood", "pong flood", "team flood", "teleport", "path speed", "wall"};
	// Breaks per 3 s window that don't score: lag bunches packets, and a pickup races shots in flight
	const int tolerance[ServerValidator::RULE_COUNT] = {5, 2, 2, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1};
}

#include <cmath>

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

namespace
{
	const float maxSpeed = 3.25f + 0.5f; // the client clamp plus a margin, units per second

	// A path through the inside of a wall cell (not a corner cut: the player's centre stays 0.25 from walls)
	bool throughWall(const Map & map, float x0, float y0, float x1, float y1)
	{
		const float length = sqrtf((x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0));
		const int steps = (int)(length / 0.1f) + 1;
		for (int i = 0; i <= steps; ++i)
		{
			const float x = x0 + (x1 - x0) * i / steps, y = y0 + (y1 - y0) * i / steps;
			const int cx = (int)x, cy = (int)y;
			if (cx < 0 || cy < 0 || cx >= map.size[0] || cy >= map.size[1]) return true;
			const float fx = x - cx, fy = y - cy;
			if (!map.cells[cy * map.size[0] + cx].passable && fx > 0.25f && fx < 0.75f && fy > 0.25f && fy < 0.75f)
				return true;
		}
		return false;
	}
}

ServerValidator::Verdict ServerValidator::onMove(Player & player, float x, float y)
{
	Track & t = track[(int)player.playerID];
	const bool knockback = player.framesSinceKnockback <= 60; // a hit pushes the player past the speed limit
	Verdict v = ACCEPT;
	if (t.known && player.timeAlive >= t.timeAlive && t.map == player.game->map)
	{
		const float d = sqrtf((x - t.x) * (x - t.x) + (y - t.y) * (y - t.y));
		if (!knockback && d > maxSpeed * (time - t.time) + 1.0f) v = violation(player, TELEPORT);
		else if (player.game->map && throughWall(*player.game->map, t.x, t.y, x, y)) v = violation(player, WALL);
		if (v == KICK) return KICK;
		t.path += d;
	}
	else
		t.path = 0;
	t.known = true;
	t.x = x;
	t.y = y;
	t.time = time;
	t.timeAlive = player.timeAlive;
	t.map = player.game->map;
	return ACCEPT;
}

ServerValidator::Verdict ServerValidator::onCoordFrame(Player & player, const net_clsv_svcl_player_coord_frame & frame)
{
	if (onMove(player, frame.position[0] / 100.0f, frame.position[1] / 100.0f) == KICK) return KICK;
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

	// Path speed: the positions walked this window against the time it took (no hit in the window)
	Track & t = track[(int)player.playerID];
	const bool pathSpeed = player.framesSinceKnockback > player.frameSinceLast &&
		t.path > maxSpeed * player.frameSinceLast / 30.0f + 1.5f;
	t.path = 0;
	if (pathSpeed && violation(player, PATH_SPEED) == KICK) return KICK;

	int & s = score[player.playerID];
	s = frameHack || speedHack ? s + 1 : 0;
	if (s >= gameVar.sv_speedHackKick && gameVar.sv_antiCheat > 0) return KICK;
	player.frameSinceLast = player.lastFrame = player.currentFrame = 0; // the next window
	return ACCEPT;
}

void ServerValidator::reset(Player & player)
{
	score[player.playerID] = 0;
	track[(int)player.playerID] = Track();
	player.frameSinceLast = player.lastFrame = player.currentFrame = 0;
}

void ServerValidator::join(Player & player)
{
	reset(player);
	const int slot = player.playerID;
	cheatScore[slot] = 0;
	warned[slot] = false;
	for (int & b : breaks[slot]) b = 0;
}

ServerValidator::Verdict ServerValidator::violation(Player & player, Rule rule)
{
	if (gameVar.sv_antiCheat == 0) return ACCEPT;
	const int slot = player.playerID;
	if (++breaks[slot][rule] <= tolerance[rule]) return ACCEPT;
	float & s = cheatScore[slot];
	s += 1;
	if (breaks[slot][rule] == tolerance[rule] + 1)
		console->add(CString("\x9> anti-cheat: %s (%s) %s, score %.0f", player.name.s, player.playerIP, ruleName[rule], s), true);
	if (gameVar.sv_antiCheat < 2) return ACCEPT;
	if (s >= gameVar.sv_antiCheatKick)
	{
		console->sendCommand(CString("sayall %s kicked by the anti-cheat (%s)", player.name.s, ruleName[rule]));
		scene->kick(slot);
		return KICK;
	}
	if (s >= gameVar.sv_antiCheatWarn && !warned[slot])
	{
		warned[slot] = true;
		CString message("Anti-cheat warning: %s", ruleName[rule]);
		bb_serverSend(message.s, message.len() + 1, NET_SVCL_CONSOLE, player.babonetID);
	}
	return rule < TELEPORT ? DROP : ACCEPT;
}

void ServerValidator::update(float delay)
{
	time += delay;
	window += delay;
	if (window >= 3)
	{
		window = 0;
		for (auto & slot : breaks)
			for (int & b : slot) b = 0;
	}
	for (int i = 0; i < MAX_PLAYER; ++i)
	{
		cheatScore[i] = cheatScore[i] > delay / 10 ? cheatScore[i] - delay / 10 : 0;
		if (cheatScore[i] < gameVar.sv_antiCheatWarn) warned[i] = false;
	}
}
