// Server-side cheat checks (Phase B step 8). The handlers in ServerRecv.cpp ask the validator after the pre-dispatch
// check bound the sender's slot, and carry out the kick it returns with the existing kick and disconnect messages.
#pragma once
#include "Game.h"

class ServerValidator
{
public:
	enum Verdict { ACCEPT, KICK };

	// Primary and melee weapon whitelist (sv_validateWeapons): nullptr if allowed, else the weapon that kicks,
	// "primary" or "secondary"
	const char * badSpawnWeapon(const net_clsv_spawn_request & spawn) const;
	// Every coord frame from a living player. At the end of each 3 s window (Player::frameSinceLast >= 90) it compares
	// client frames with server ticks and checks the horizontal speed: a window with a violation scores 1, a clean one
	// resets the score, and sv_speedHackKick in a row kick.
	Verdict onCoordFrame(Player & player, const net_clsv_svcl_player_coord_frame & frame);
	// A new player in the slot, or a dead player: the score and the check window restart
	void reset(Player & player);

private:
	int score[MAX_PLAYER] = {};
};
