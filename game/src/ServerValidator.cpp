#include "ServerValidator.h"
#include "GameVar.h"

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
	if (s >= gameVar.sv_speedHackKick) return KICK;
	player.frameSinceLast = player.lastFrame = player.currentFrame = 0; // the next window
	return ACCEPT;
}

void ServerValidator::reset(Player & player)
{
	score[player.playerID] = 0;
	player.frameSinceLast = player.lastFrame = player.currentFrame = 0;
}
