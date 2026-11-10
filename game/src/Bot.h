// Server-side bots (ROBO-123, ADR 0049). A bot is a player slot with no connection: each frame the server runs its
// brain, and the bot sends the same messages a player's client sends (team, spawn, coord frame, shoot, projectile,
// melee) through Server::recvPacket, so the server checks and handles them like a human's. Clients need no change.
// The brain is a port of BaboViolent/babo-dedicated-server server/src/game/bots.rs (GPL-3, commit 7c7d05c):
// utility-scored tactical states, CTF team roles, tile navigation with stuck recovery, human-like aim.
#pragma once
#include "Player.h"

#include <array>
#include <cstdint>

class Game;
class Map;
class Server;

namespace bots
{
	enum State
	{
		WANDER,
		ATTACK,
		ATTACK_OBJECTIVE,
		CAPTURE,
		ESCORT,
		RECOVER,
		RETREAT,
		STATE_COUNT
	};
	enum Role
	{
		ATTACKER,
		DEFENDER,
		ESCORTER,
		RECOVERER,
		INTERCEPTOR,
		CARRIER,
		ROLE_COUNT,
		NO_ROLE = -1
	};
	const char* stateName(int state);
	const char* roleName(int role);

	// Every tuning number in one place, so a later skill setting changes values here only (seconds, map units)
	struct Tuning
	{
		float speed = 2.8f;
		float aimRange = 14.0f;
		float aimError = 0.055f; // times the distance, swung by a slow sine
		float acquireTime = 0.12f; // a visible enemy is shot at after this long
		float enemyMemory = 1.5f; // a lost enemy is remembered this long
		float perceptionInterval = 0.08f;
		float hearingRange = 8.0f;
		float audibleShotAge = 0.2f;
		float goalRefresh = 0.35f;
		float minCommitment = 0.8f; // a state or role is kept at least this long unless urgent
		float stuckThreshold = 0.04f; // moved less than this in a frame: not moving
		float stuckTime = 0.45f;
		float goalStaleTime = 2.2f;
		float navNoProgressTime = 0.45f;
		float navProgressEpsilon = 0.025f;
		float navWaypointCommit = 0.75f;
		float separationRadius = 0.9f;
		float separationWeight = 1.35f;
		float defendPressureRange = 6.8f;
		float carrierDirectCapture = 3.5f;
		float escapeStep = 1.8f;
		int carrierEscorts = 2;
		int missingFlagRecoverers = 2;
		float utilityCapture = 1000, utilityRecover = 900, utilityEscort = 70, utilityAttack = 40,
			  utilityObjective = 30, utilityWander = 5, utilityRetreatHealth = 180;
	};
	extern const Tuning tuning;

	struct V2
	{
		float x = 0, y = 0;
	};

	// What every bot sees this frame, built once per frame (positions are the server's view)
	struct World
	{
		Game* game = 0;
		Map* map = 0;
		int gameType = 0;
		bool alive[32] = {}, isBot[32] = {};
		int team[32];
		CVector3f pos[32], vel[32];
		float shotAge[32];
		bool ctf = false;
		int flagState[2] = {-2, -2}; // per flag (0 blue, 1 red): -2 on its pod, -1 dropped, else the carrier's slot
		CVector3f flagPos[2], flagPod[2];
		int roles[2][32]; // per team: each bot's role this frame, NO_ROLE when none
		bool defendPressure[2] = {};
		bool hasPredictedSpawn[2] = {};
		V2 predictedEnemySpawn[2];
		bool enemyRecentlyDead[2] = {};
	};
	World buildWorld(Game& game);

	// The brain's memory (bots.rs BotBrain)
	struct Brain
	{
		int role = ATTACKER, state = WANDER;
		float committedFor = 0, roleCommittedFor = 0.8f;
		uint32_t stateChanges = 0, shotsFired = 0, occludedShots = 0, stuckEvents = 0;
		int perceivedTarget = -1;
		V2 lastKnownEnemy;
		float enemyMemoryLeft = 0, targetVisibleFor = 0, perceptionElapsed = 0;
		V2 aimPoint, aimVelocity;
		bool aimSet = false;
		float recoveryDir = 1, recoveryTimer = 0;
		int recoveryTries = 0, burstLeft = 0;
		float burstPause = 0, spawnPressure = 0, goalStaleFor = 0;
		V2 goal, navGoal, navWaypoint;
		bool hasNavGoal = false;
		int navFromTile[2] = {-1000000, -1000000};
		float navWaypointTime = 0, navBestDistance = 1e9f, navNoProgressTime = 0;
		float goalRefresh = 0, wanderAngle = 0, stuckTime = 0;
		float fireCooldown = 0, grenadeWait = 0, molotovWait = 0, meleeWait = 0;
	};

	// One frame's decision: where to move, where to look, what to fire
	struct Action
	{
		V2 velocity, lookAt;
		enum Fire
		{
			NONE,
			SHOOT,
			ROCKET,
			GRENADE,
			MOLOTOV,
			MELEE
		} fire = NONE;
		V2 fireAt;
	};
	// The brain's frame for the bot in `slot` at `position` (bots.rs tick_bots, one bot)
	Action think(Brain& brain, const World& world, int slot, const CVector3f& position, const Player& self, float delay);

	// Map queries the brain uses (bots.rs map/mod.rs)
	bool passable(const Map& map, int x, int y);
	bool canSee(Map& map, const CVector3f& from, const CVector3f& target, float range);
	V2 resolveGoal(Map& map, V2 goal);
	V2 navigateToward(Map& map, V2 from, V2 goal);
}

// The virtual client of one bot slot
class Bot
{
public:
	Bot(int slot, int rosterIndex, int team)
		: slot(slot), rosterIndex(rosterIndex), team(team) {}
	// One server frame: spawn when allowed, think, move, and send this frame's messages
	void update(Server& server, const bots::World& world, float delay);

	int slot, rosterIndex;
	int team; // blue or red: the team it plays for, kept while an admin moves it to spectator
	bots::Brain brain;

private:
	void send(Server& server, int typeID, const void* data, int size);
	CoordFrame cf, lastCF;
	long frameID = 1;
	bool alive = false;
	int sendFrame = 0;
	float spawnWait = 0;
};
