// Headless bot match (ROBO-123, ADR 0049): bots only, on the in-process server, with a fixed seed. Prints a JSON
// summary and exits 1 when the baseline verdict fails, so CI catches a brain that stops playing.
// Usage: bot_sim [--mode CTF|TDM] [--bots N] [--seconds S] [--seed N]; BOT_SIM_DEBUG=1 logs bots standing still.
#include "server_harness.h"

#include "Console.h"
#include "Game.h"
#include "Scene.h"
#include "Server.h"

#include <cstdlib>
#include <cstring>
#include <string>

extern Scene* scene;

namespace
{
	// Verdict: a CTF match has a capture, a TDM one a kill, and no bot stands still away from its goal this long
	const float MAX_STUCK_SECONDS = 10.0f;
	const float STUCK_DISTANCE = 0.2f; // moved less than this in one second: standing still
	const float AT_GOAL = 0.5f; // a defender on its post or a carrier waiting at its pod is holding, not stuck

	// A match counter that survives the game resetting scores (score limit reached, new game)
	struct Total
	{
		int banked = 0, last = 0;
		void sample(int now)
		{
			if (now < last)
				banked += last;
			last = now;
		}
		int value() const { return banked + last; }
	};
}

int main(int argc, char** argv)
{
	std::string mode = "CTF";
	int botCount = 8, seconds = 300;
	unsigned seed = 1;
	for (int i = 1; i < argc; i += 2)
	{
		const char* value = i + 1 < argc ? argv[i + 1] : "";
		if (!std::strcmp(argv[i], "--mode"))
			mode = value;
		else if (!std::strcmp(argv[i], "--bots"))
			botCount = std::atoi(value);
		else if (!std::strcmp(argv[i], "--seconds"))
			seconds = std::atoi(value);
		else if (!std::strcmp(argv[i], "--seed"))
			seed = (unsigned)std::strtoul(value, 0, 10);
		else
		{
			std::fprintf(stderr, "usage: bot_sim [--mode CTF|TDM] [--bots N] [--seconds S] [--seed N]\n");
			return 2;
		}
	}
	if (!harness::start(argv[0], mode.c_str()))
	{
		std::fprintf(stderr, "server did not start\n");
		return 1;
	}
	std::srand(seed); // after start: the server seeds rand() with the clock
	const bool debug = std::getenv("BOT_SIM_DEBUG") != 0;
	console->sendCommand(CString("set sv_bots %d", botCount));

	Server& server = *scene->server;
	Game& game = *server.game;
	const bool ctf = game.gameType == GAME_TYPE_CTF;
	float stateTime[bots::STATE_COUNT] = {}, roleTime[bots::ROLE_COUNT] = {};
	float stuckFor[MAX_PLAYER] = {}, maxStuck = 0;
	CVector3f lastPos[MAX_PLAYER];
	bool wasAlive[MAX_PLAYER] = {};
	Total kills, deaths, returns, captures;
	for (int frame = 0; frame < seconds * 30; ++frame)
	{
		harness::tick();
		harness::sent().clear(); // nothing reads them, and a long match fills memory
		int k = 0, d = 0, r = 0;
		for (int i = 0; i < MAX_PLAYER; ++i)
		{
			const Player* p = game.players[i];
			if (!p)
				continue;
			k += p->kills;
			d += p->deaths;
			r += p->returns;
			const Bot* bot = server.bots[i].get();
			if (!bot || p->status != PLAYER_STATUS_ALIVE)
				continue;
			stateTime[bot->brain.state] += 1 / 30.0f;
			if (ctf && bot->brain.role >= 0)
				roleTime[bot->brain.role] += 1 / 30.0f;
		}
		kills.sample(k);
		deaths.sample(d);
		returns.sample(r);
		captures.sample(ctf ? game.blueScore + game.redScore : 0);

		if (frame % 30)
			continue;
		for (int i = 0; i < MAX_PLAYER; ++i)
		{
			const Player* p = game.players[i];
			const Bot* bot = server.bots[i].get();
			// Frozen between rounds is not stuck
			const bool alive = p && bot && p->status == PLAYER_STATUS_ALIVE && game.roundState == GAME_PLAYING;
			if (alive)
			{
				const CVector3f& at = p->currentCF.position;
				const CVector3f goal(bot->brain.goal.x, bot->brain.goal.y, at[2]);
				stuckFor[i] = wasAlive[i] && distance(lastPos[i], at) < STUCK_DISTANCE && distance(goal, at) > AT_GOAL ? stuckFor[i] + 1 : 0;
				if (debug && stuckFor[i] >= 5)
					std::fprintf(stderr, "t=%d slot %d still %.0f s: state %s role %s at %.2f,%.2f goal %.2f,%.2f waypoint %.2f,%.2f\n",
								 frame / 30, i, stuckFor[i], bots::stateName(bot->brain.state),
								 bot->brain.role >= 0 ? bots::roleName(bot->brain.role) : "none", at[0], at[1], goal[0],
								 goal[1], bot->brain.navWaypoint.x, bot->brain.navWaypoint.y);
				lastPos[i] = at;
			}
			else
				stuckFor[i] = 0;
			if (stuckFor[i] > maxStuck)
				maxStuck = stuckFor[i];
			wasAlive[i] = alive;
		}
	}

	unsigned shots = 0, occluded = 0, stuckEvents = 0, stateChanges = 0;
	int present = 0;
	for (int i = 0; i < MAX_PLAYER; ++i)
		if (const Bot* bot = server.bots[i].get())
		{
			++present;
			shots += bot->brain.shotsFired;
			occluded += bot->brain.occludedShots;
			stuckEvents += bot->brain.stuckEvents;
			stateChanges += bot->brain.stateChanges;
		}
	const bool pass = present == botCount && shots > 0 && (ctf ? captures.value() >= 1 : kills.value() >= 1) &&
					  maxStuck <= MAX_STUCK_SECONDS;

	std::printf("{\n  \"mode\": \"%s\", \"seed\": %u, \"seconds\": %d, \"bots\": %d,\n", mode.c_str(), seed, seconds, present);
	std::printf("  \"stateSeconds\": {");
	for (int s = 0; s < bots::STATE_COUNT; ++s)
		std::printf("%s\"%s\": %.0f", s ? ", " : "", bots::stateName(s), stateTime[s]);
	std::printf("},\n  \"roleSeconds\": {");
	for (int r = 0; r < bots::ROLE_COUNT; ++r)
		std::printf("%s\"%s\": %.0f", r ? ", " : "", bots::roleName(r), roleTime[r]);
	std::printf("},\n  \"stateChanges\": %u, \"shots\": %u, \"occludedShotsHeld\": %u, \"kills\": %d, \"deaths\": %d,\n",
				stateChanges, shots, occluded, kills.value(), deaths.value());
	std::printf("  \"captures\": %d, \"returns\": %d, \"stuckEvents\": %u, \"maxStuckSeconds\": %.0f,\n", captures.value(),
				returns.value(), stuckEvents, maxStuck);
	std::printf("  \"verdict\": \"%s\"\n}\n", pass ? "pass" : "fail");

	console->sendCommand("set sv_bots 0");
	harness::tick();
	harness::stop();
	return pass ? 0 : 1;
}
