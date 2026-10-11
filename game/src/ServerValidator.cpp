#include "ServerValidator.h"
#include "GameVar.h"

#include <cmath>

ServerValidator::Verdict ServerValidator::onSpawnRequest(const net_clsv_spawn_request & spawn, const char *& which) const
{
	if (!gameVar.sv_validateWeapons) return ACCEPT;
	switch (spawn.weaponID)
	{
	case WEAPON_SMG: case WEAPON_SHOTGUN: case WEAPON_SNIPER: case WEAPON_DUAL_MACHINE_GUN: case WEAPON_CHAIN_GUN:
	case WEAPON_BAZOOKA: case WEAPON_PHOTON_RIFLE: case WEAPON_FLAME_THROWER:
		break;
	default:
		which = "primary";
		return KICK;
	}
	switch (spawn.meleeID)
	{
	case WEAPON_KNIVES: case WEAPON_NUCLEAR: case WEAPON_SHIELD: case WEAPON_MINIBOT:
		return ACCEPT;
	default:
		which = "secondary";
		return KICK;
	}
}

ServerValidator::Verdict ServerValidator::onCoordFrame(int slot, const Player & player,
	const net_clsv_svcl_player_coord_frame & frame)
{
	// Frame check: client frames elapsed vs server ticks in the same window. Allow 5 frames + 10% for packet jitter
	// and the client's catch-up ticks after a stall; a real speed hack still drifts far beyond that.
	long clientFrames = player.currentFrame - player.lastFrame;
	bool frameHack = clientFrames > player.frameSinceLast + 5 + player.frameSinceLast / 10;

	// Speed check: horizontal speed vs the client clamp (3.25) plus the char*10 quantisation margin. Knockback from a
	// hit legitimately exceeds the clamp, so skip it for 2 s after the server registers one.
	float vx = frame.vel[0] / 10.0f, vy = frame.vel[1] / 10.0f;
	bool speedHack = player.framesSinceKnockback > 60 && sqrtf(vx * vx + vy * vy) > 3.25f + 0.15f;

	if (!frameHack && !speedHack)
	{
		score[slot] = 0;
		return ACCEPT;
	}
	return ++score[slot] >= gameVar.sv_speedHackKick ? KICK : ACCEPT;
}
