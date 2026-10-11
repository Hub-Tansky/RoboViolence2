// Server-side cheat checks (Phase B steps 8, 9). The handlers in ServerRecv.cpp ask the validator after the
// pre-dispatch check bound the sender's slot. Step 8's rules return a kick the handler carries out; step 9's rules
// score through violation() (docs/analysis/ANTI_CHEAT_TUNING.md).
#pragma once
#include "Game.h"

class ServerValidator
{
public:
	enum Verdict { ACCEPT, KICK, DROP };
	// Step 9 rules. Weapon and flood rules drop the message at sv_antiCheat 2; movement rules never do.
	enum Rule
	{
		FIRE_RATE, SHOT_WEAPON, SHOT_DEAD, GRENADE_AMMO, ROCKET_WEAPON, SPAWN_EARLY,
		FLOOD_CHAT, FLOOD_VOTE, FLOOD_NAME, FLOOD_SKIN, FLOOD_PONG, FLOOD_TEAM,
		TELEPORT, PATH_SPEED, WALL, RULE_COUNT
	};

	// Primary and melee weapon whitelist (sv_validateWeapons): nullptr if allowed, else the weapon that kicks,
	// "primary" or "secondary"
	const char * badSpawnWeapon(const net_clsv_spawn_request & spawn) const;
	// Every coord frame from a living player. At the end of each 3 s window (Player::frameSinceLast >= 90) it compares
	// client frames with server ticks and checks the horizontal speed: a window with a violation scores 1, a clean one
	// resets the score, and sv_speedHackKick in a row kick.
	Verdict onCoordFrame(Player & player, const net_clsv_svcl_player_coord_frame & frame);
	// A dead player: the speed score and the check window restart
	void reset(Player & player);
	// A new player in the slot: every score restarts
	void join(Player & player);

	// A broken step 9 rule (sv_antiCheat 1 and 2). Once the rule's tolerance (breaks per 3 s window) is used up, each
	// break scores 1, logged once per rule and window. At sv_antiCheat 2 the score's action applies (a private warning
	// at sv_antiCheatWarn, a kick at sv_antiCheatKick) and a weapon or flood rule returns DROP. KICK: the player is
	// gone, the handler returns.
	Verdict violation(Player & player, Rule rule);
	// Every server frame: the score decays by 1 per 10 s and the tolerance windows restart every 3 s
	void update(float delay);
	float scoreOf(int slot) const { return cheatScore[slot]; }

private:
	int score[MAX_PLAYER] = {}; // step 8 speed rule: bad windows in a row
	float cheatScore[MAX_PLAYER] = {};
	int breaks[MAX_PLAYER][RULE_COUNT] = {};
	bool warned[MAX_PLAYER] = {};
	float window = 0;
};
