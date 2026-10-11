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

ServerValidator::Verdict ServerValidator::onCoordFrame(Player & player, const net_clsv_svcl_player_coord_frame & frame)
{
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

	int & s = score[player.playerID];
	s = frameHack || speedHack ? s + 1 : 0;
	if (s >= gameVar.sv_speedHackKick && gameVar.sv_antiCheat > 0) return KICK;
	player.frameSinceLast = player.lastFrame = player.currentFrame = 0; // the next window
	return ACCEPT;
}

void ServerValidator::reset(Player & player)
{
	score[player.playerID] = 0;
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
