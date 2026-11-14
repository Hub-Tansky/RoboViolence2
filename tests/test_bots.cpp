// Bots for empty servers (ROBO-123): the roster rules, then bots on the in-process CTF server.
#include "server_harness.h"

#include "Bot.h"
#include "BotRoster.h"
#include "Console.h"
#include "Game.h"
#include "GameVar.h"
#include "Scene.h"
#include "Server.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <vector>
#include <string>

extern Scene* scene;

namespace
{
	Server& server()
	{
		return *scene->server;
	}
	Game& game()
	{
		return *scene->server->game;
	}

	int botsOn(int team)
	{
		int n = 0;
		for (int i = 0; i < MAX_PLAYER; ++i)
			if (game().players[i] && game().players[i]->isBot && game().players[i]->teamID == team)
				++n;
		return n;
	}

	void rosterRules()
	{
		// Empty server: the target, split evenly, one slot below max always free
		bots::Teams t = bots::wanted(0, 0, 0, 8, 10);
		CHECK(t.blue == 4 && t.red == 4);
		t = bots::wanted(0, 0, 0, 12, 10);
		CHECK(t.blue + t.red == 9);
		// Each human replaces one bot, and bots fill the smaller team
		t = bots::wanted(1, 0, 1, 8, 10);
		CHECK(t.blue == 3 && t.red == 4);
		t = bots::wanted(2, 0, 2, 8, 10);
		CHECK(t.blue == 2 && t.red == 4);
		// A spectator or a connecting human replaces no bot, but takes a slot: one below max stays free
		t = bots::wanted(0, 0, 1, 8, 10);
		CHECK(t.blue + t.red == 8);
		t = bots::wanted(0, 0, 2, 8, 10);
		CHECK(t.blue + t.red == 7);
		// No bots left at the target, none with sv_bots 0
		t = bots::wanted(4, 4, 8, 8, 10);
		CHECK(t.blue + t.red == 0);
		t = bots::wanted(5, 4, 9, 8, 10); // 9 or 10 humans: still no bots, the count is the humans
		CHECK(t.blue + t.red == 0);
		t = bots::wanted(5, 5, 10, 8, 10);
		CHECK(t.blue + t.red == 0);
		t = bots::wanted(0, 0, 0, 0, 10);
		CHECK(t.blue + t.red == 0);

		CHECK(bots::name({"Servo"}, 0) == "Servo [bot]");
		CHECK(bots::name({"Servo", "Tin Can"}, 3) == "Tin Can [bot]");
		CHECK(bots::name({}, 2) == "Bot 3 [bot]");
		CHECK(bots::name({std::string(40, 'x')}, 0) == std::string(25, 'x') + " [bot]"); // fits playerName[32]
		const std::vector<std::string> three = {"Servo", "Tin Can", "Bolt"};
		CHECK(bots::pickName(three, {}, 0) == 0 && bots::pickName(three, {}, 5) == 2); // the roll picks among free names
		CHECK(bots::pickName(three, {"Servo [bot]"}, 0) == 1);                        // a taken name is skipped
		CHECK(bots::pickName(three, {"Servo [bot]", "Bolt [bot]"}, 7) == 1);
		CHECK(bots::pickName(three, {"Servo [bot]", "Tin Can [bot]", "Bolt [bot]"}, 0) == -1);
		CHECK(bots::pickName({}, {}, 3) == -1);
		const char* path = "bots-test-names.txt";
		std::ofstream(path) << "# comment\nServo\r\n\nTin Can \n";
		const std::vector<std::string> names = bots::loadNames(path);
		CHECK(names.size() == 2 && names[0] == "Servo" && names[1] == "Tin Can");
		std::remove(path);
		CHECK(bots::loadNames("no-such-file.txt").empty());
	}

	// ROBO-212: with friendly fire on in a team mode, bots don't hit their own team
	void holdsForTeammate()
	{
		bots::World w;
		w.gameType = GAME_TYPE_CTF;
		for (int i = 0; i < 3; ++i)
			w.alive[i] = true;
		w.team[0] = w.team[1] = PLAYER_TEAM_BLUE; // slot 0 shoots, slot 1 is its teammate
		w.team[2] = PLAYER_TEAM_RED;
		const CVector3f me(2, 5, 0), enemy(8, 5, 0), between(5, 5, 0), aside(5, 8, 0), behind(10, 5, 0);
		w.pos[0] = me, w.pos[2] = enemy;
		const bool savedFriendlyFire = gameVar.sv_friendlyFire;
		gameVar.sv_friendlyFire = true;
		w.pos[1] = between;
		CHECK(bots::holdsForTeammate(w, 0, me, enemy, WEAPON_SMG));
		w.pos[1] = aside;
		CHECK(!bots::holdsForTeammate(w, 0, me, enemy, WEAPON_SMG));
		CHECK(bots::holdsForTeammate(w, 0, me, enemy, WEAPON_GRENADE) && bots::holdsForTeammate(w, 0, me, enemy, WEAPON_COCKTAIL_MOLOTOV));
		w.pos[1] = behind;
		CHECK(!bots::holdsForTeammate(w, 0, me, enemy, WEAPON_SMG)); // behind the target: the bullet stops at it
		CHECK(bots::holdsForTeammate(w, 0, me, enemy, WEAPON_PHOTON_RIFLE)); // the beam goes through
		CHECK(bots::holdsForTeammate(w, 0, me, enemy, WEAPON_BAZOOKA)); // within 3 of the blast
		w.alive[1] = false;
		CHECK(!bots::holdsForTeammate(w, 0, me, enemy, WEAPON_PHOTON_RIFLE)); // a dead teammate blocks nothing
		w.alive[1] = true;
		w.pos[1] = between;
		w.gameType = GAME_TYPE_DM; // no teams
		CHECK(!bots::holdsForTeammate(w, 0, me, enemy, WEAPON_SMG) && !bots::holdsForTeammate(w, 0, me, enemy, WEAPON_GRENADE));
		w.gameType = GAME_TYPE_CTF;
		gameVar.sv_friendlyFire = false;
		CHECK(!bots::holdsForTeammate(w, 0, me, enemy, WEAPON_SMG) && !bots::holdsForTeammate(w, 0, me, enemy, WEAPON_GRENADE));
		gameVar.sv_friendlyFire = savedFriendlyFire;
	}
}

int main(int, char** argv)
{
	rosterRules();
	holdsForTeammate();
	if (!harness::start(argv[0]))
	{
		std::printf("FAIL: server did not start\n");
		return 1;
	}

	// The shipped names file loads
	CHECK(server().botNames.size() == 18);

	// sv_bots 8 on an empty CTF server (sv_maxPlayer 10): 4 bots per team, shown as players
	console->sendCommand("set sv_bots 8");
	harness::tick(2);
	CHECK(botsOn(PLAYER_TEAM_BLUE) == 4 && botsOn(PLAYER_TEAM_RED) == 4);
	CHECK(game().countPlayers().bots == 8 && game().countPlayers().total() == 8);
	std::vector<std::string> botNames; // each from the list, none twice
	for (int i = 0; i < MAX_PLAYER; ++i)
		if (Player* p = game().players[i])
		{
			const std::string name = p->name.s;
			CHECK(name.size() > 6 && name.substr(name.size() - 6) == " [bot]");
			CHECK(p->ping == 0);
			bool listed = false;
			for (int n = 0; n < (int)server().botNames.size(); ++n)
				listed = listed || name == bots::name(server().botNames, n);
			CHECK(listed);
			CHECK(std::find(botNames.begin(), botNames.end(), name) == botNames.end());
			botNames.push_back(name);
		}

	// They spawn after the respawn delay and move, shoot, and are never kicked by the join timeout or the anti-cheat.
	// A bot that spawned may already be dead again (the shipped CTF script plays Pro damage, ADR 0052).
	CVector3f start[MAX_PLAYER];
	harness::tick(30 * 7);
	int alive = 0, spawned = 0;
	for (int i = 0; i < MAX_PLAYER; ++i)
		if (Player* p = game().players[i])
		{
			alive += p->status == PLAYER_STATUS_ALIVE;
			spawned += p->status == PLAYER_STATUS_ALIVE || p->deaths > 0;
			start[i] = p->currentCF.position;
		}
	CHECK(spawned == 8);
	harness::tick(30 * 30);
	int moved = 0;
	unsigned shots = 0;
	for (int i = 0; i < MAX_PLAYER; ++i)
		if (Player* p = game().players[i])
		{
			moved += distance(start[i], p->currentCF.position) > 0.5f;
			if (server().bots[i])
				shots += server().bots[i]->brain.shotsFired;
		}
	CHECK(game().countPlayers().bots == 8);
	CHECK(moved >= 4);
	CHECK(shots > 0);
	std::printf("bots: %d alive, %d moved, %u shots\n", alive, moved, shots);

	// Nothing is sent to a bot's babonetID: bots have no connection
	int toBots = 0;
	for (const harness::SentPacket& p : harness::sent())
		if ((UINT4)p.destination >= Server::BOT_BABONET_ID)
			++toBots;
	CHECK(toBots == 0);

	// A human joins blue: one blue bot leaves, the count stays 8 (8/10 in the browser)
	UINT4 human = harness::spawned(WEAPON_SMG);
	harness::tick(2);
	CHECK(botsOn(PLAYER_TEAM_BLUE) == 3 && botsOn(PLAYER_TEAM_RED) == 4);
	CHECK(game().countPlayers().humans == 1 && game().countPlayers().total() == 8);
	// The human's team chat and a name change go to its team, not to the bots' network ids
	harness::sent().clear();
	net_clsv_svcl_chat chat = {};
	chat.teamID = PLAYER_TEAM_BLUE;
	std::snprintf(chat.message, sizeof(chat.message), "Tester: go");
	harness::deliver(human, NET_CLSV_SVCL_CHAT, chat);
	net_clsv_svcl_player_change_name rename = {};
	rename.playerID = (int8_t)harness::slotOf(human);
	std::snprintf(rename.playerName, sizeof(rename.playerName), "Renamed");
	harness::deliver(human, NET_CLSV_SVCL_PLAYER_CHANGE_NAME, rename);
	harness::tick(2);
	int chats = 0, toBotsLater = 0;
	for (const harness::SentPacket & p : harness::sent())
	{
		chats += p.typeID == NET_CLSV_SVCL_CHAT || p.typeID == NET_CLSV_SVCL_PLAYER_CHANGE_NAME;
		toBotsLater += (UINT4)p.destination >= Server::BOT_BABONET_ID;
	}
	CHECK(chats >= 1 && toBotsLater == 0);

	// The human leaves: the bot comes back
	harness::disconnect(human);
	harness::tick(2);
	CHECK(botsOn(PLAYER_TEAM_BLUE) == 4 && game().countPlayers().total() == 8);

	// Bots are never written to stats or accounts: those need a logged-in user id
	for (int i = 0; i < MAX_PLAYER; ++i)
		if (Player* p = game().players[i])
			CHECK(p->userID == 0);

	// Bots stay through a map change, and play deathmatch (the server rebalances them into one team)
	console->sendCommand("set sv_gameType 0");
	console->sendCommand("changemap CTF-Placeholder");
	harness::tick(30 * 20);
	CHECK(game().gameType == GAME_TYPE_DM);
	CHECK(game().countPlayers().bots == 8);
	unsigned before = 0, after = 0;
	int mostAlive = 0; // deathmatch on a small arena kills fast: respawning keeps most of them up at some point
	for (int i = 0; i < MAX_PLAYER; ++i)
		if (server().bots[i])
			before += server().bots[i]->brain.shotsFired;
	for (int second = 0; second < 20; ++second)
	{
		harness::tick(30);
		int aliveNow = 0;
		for (int i = 0; i < MAX_PLAYER; ++i)
			if (server().bots[i])
				aliveNow += game().players[i]->status == PLAYER_STATUS_ALIVE;
		if (aliveNow > mostAlive)
			mostAlive = aliveNow;
	}
	for (int i = 0; i < MAX_PLAYER; ++i)
		if (server().bots[i])
			after += server().bots[i]->brain.shotsFired;
	std::printf("deathmatch: up to %d alive, %u shots\n", mostAlive, after - before);
	CHECK(mostAlive >= 4 && after > before);

	// An admin moves everyone to spectator: the bots ask to rejoin their teams, and no extra bots fill slots
	console->sendCommand("allwatch");
	harness::tick(3);
	CHECK(game().countPlayers().bots == 8);
	harness::tick(30 * 2);
	CHECK(botsOn(PLAYER_TEAM_BLUE) == 4 && botsOn(PLAYER_TEAM_RED) == 4 && game().countPlayers().bots == 8);

	// sv_bots changes take effect at once
	console->sendCommand("set sv_bots 4");
	harness::tick(1);
	CHECK(game().countPlayers().bots == 4);
	console->sendCommand("set sv_bots 0");
	harness::tick(1);
	CHECK(game().countPlayers().bots == 0 && game().countPlayers().total() == 0);

	harness::stop();
	return harness::failures();
}
