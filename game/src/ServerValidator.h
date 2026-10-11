// Server-side cheat checks (Phase B step 8). The handlers in ServerRecv.cpp ask the validator after the pre-dispatch
// check bound the sender's slot, and carry out the kick it returns with the existing kick and disconnect messages.
#pragma once
#include "Game.h"

class ServerValidator
{
public:
	enum Verdict { ACCEPT, KICK };

	// Primary and melee weapon whitelist (sv_validateWeapons). Sets `which` to "primary" or "secondary" on a kick.
	Verdict onSpawnRequest(const net_clsv_spawn_request & spawn, const char *& which) const;
	// Once per 3 s check window (Player::frameSinceLast >= 90): client frames vs server ticks, and horizontal speed.
	// Each window with a violation scores 1, a clean one resets the score; sv_speedHackKick in a row kick.
	Verdict onCoordFrame(int slot, const Player & player, const net_clsv_svcl_player_coord_frame & frame);
	// A new player in the slot, or a dead player (the windows restart)
	void reset(int slot) { score[slot] = 0; }

private:
	int score[MAX_PLAYER] = {};
};
