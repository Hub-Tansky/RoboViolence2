// Server-side cheat checks (Phase B steps 8, 9). The handlers in ServerRecv.cpp ask the validator after the
// pre-dispatch check bound the sender's slot. Rules, limits and tuning: docs/analysis/ANTI_CHEAT_TUNING.md.
#pragma once
#include "Game.h"

class ServerValidator
{
public:
	// KICKED: the validator kicked the player, who is deleted; the handler returns at once
	enum Verdict { ACCEPT, DROP, KICKED };
	// Step 9 rules (their settings: `rules` in ServerValidator.cpp)
	enum Rule
	{
		FIRE_RATE, SHOT_WEAPON, SHOT_DEAD, GRENADE_AMMO, ROCKET_WEAPON, SPAWN_EARLY,
		FLOOD_CHAT, FLOOD_VOTE, FLOOD_NAME, FLOOD_SKIN, FLOOD_PONG, FLOOD_TEAM,
		TELEPORT, PATH_SPEED, WALL, RULE_COUNT
	};

	// Primary and melee weapon whitelist (sv_validateWeapons): nullptr if allowed, else the weapon that kicks,
	// "primary" or "secondary"
	const char * badSpawnWeapon(const net_clsv_spawn_request & spawn) const;
	// A spawn request (SPAWN_EARLY), and a spawn it led to (the movement rules start over)
	Verdict onSpawnRequest(Player & player);
	void onSpawned(Player & player);
	// A shot or non-grenade projectile from a living player: SHOT_WEAPON, and FIRE_RATE against a budget at the held
	// weapon's fire rate
	Verdict onShot(Player & player, int weaponID);
	// The pickup handler swapped the player's weapon: shots in flight with the old one are not SHOT_WEAPON
	void onPickup(Player & player, int oldWeaponID);
	// A shot or projectile from a dead player (SHOT_DEAD)
	void onDeadShot(Player & player);
	// Every coord frame from a living player. Step 8: at the end of each 3 s window (Player::frameSinceLast >= 90) it
	// compares client frames with server ticks and checks the reported speed; sv_speedHackKick bad windows in a row
	// kick (sv_antiCheat 1 and 2). Step 9: TELEPORT, WALL and PATH_SPEED.
	Verdict onCoordFrame(Player & player, const net_clsv_svcl_player_coord_frame & frame);
	// A dead player: the step 8 windows and the movement rules start over
	void reset(Player & player);
	// A new player in the slot: everything starts over
	void join(Player & player);

	// A broken step 9 rule (sv_antiCheat 1 and 2). Past the rule's tolerance (breaks per 3 s window) it scores 1 and
	// logs, once per rule and window. At sv_antiCheat 2 the score's action applies (a private warning at
	// sv_antiCheatWarn, a kick at sv_antiCheatKick), and a rule that drops returns DROP.
	Verdict violation(Player & player, Rule rule);
	// A message of a rate-limited type, named by its flood rule: past the type's budget it breaks the rule
	Verdict budget(Player & player, Rule flood);
	// Every server frame: budgets refill, the score decays by 1 per 10 s, tolerance windows restart every 3 s
	void update(float delay);
	float scoreOf(int slot) const { return slots[slot].score; }

private:
	struct Track // movement rules: the last frame's position and time, the path walked in the step 8 window
	{
		bool known = false;
		float x = 0, y = 0, time = 0, timeAlive = 0, path = 0;
	};
	struct Slot
	{
		int badWindows = 0; // step 8
		float score = 0;
		bool warned = false;
		int breaks[RULE_COUNT] = {};
		float tokens[RULE_COUNT] = {}; // flood budgets
		float fireTokens = 0, fireTime = 0; // FIRE_RATE budget, refilled at the last shot
		int oldWeapon = -1;
		float pickupTime = -10;
		Track track;
	} slots[MAX_PLAYER];
	float window = 0, time = 0;

	Verdict kick(Player & player, const char * why);
	Verdict onMove(Player & player, const net_clsv_svcl_player_coord_frame & frame);
};
