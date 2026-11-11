// ROBO-201: a team change made between a client's player info and the end of its join handshake (admin `move`,
// autobalance) is broadcast while the client still drops game packets, so the handshake resends the joiner its own team.
#include "server_harness.h"

#include "Console.h"
#include "Game.h"
#include "Scene.h"
#include "Server.h"

#include <cstdio>

extern Scene* scene;

// The team the server last sent client `id` for its own slot after `from` in sent() and after that client's
// SERVER_INFO (the client drops game packets before it); -2 when none
static int ownTeamSent(UINT4 id, int slot, size_t from)
{
	int team = -2;
	bool welcomed = false;
	for (size_t i = from; i < harness::sent().size(); ++i)
	{
		const harness::SentPacket& p = harness::sent()[i];
		if (p.destination != (INT4)id)
			continue;
		welcomed |= p.typeID == NET_SVCL_SERVER_INFO;
		if (p.typeID != NET_CLSV_SVCL_TEAM_REQUEST || !welcomed)
			continue;
		const net_clsv_svcl_team_request* r = (const net_clsv_svcl_team_request*)p.data.data();
		if (r->playerID == slot)
			team = r->teamRequested;
	}
	return team;
}

static void acceptVersion(UINT4 id)
{
	net_clsv_gameversion_accepted accepted = {};
	harness::deliver(id, NET_CLSV_GAMEVERSION_ACCEPTED, accepted);
	harness::tick();
}

int main(int, char** argv)
{
	if (!harness::start(argv[0]))
		return 1;

	// The real client sends its player info before it accepts the version (ClientRecv.cpp), and the server logs
	// "joined the game" on the info. An admin moves the player right then.
	UINT4 a = harness::connect();
	net_clsv_svcl_player_info info = {};
	std::snprintf(info.playerName, sizeof(info.playerName), "%s", "Mover");
	harness::deliver(a, NET_CLSV_SVCL_PLAYER_INFO, info);
	harness::tick();
	int slot = harness::slotOf(a);
	CHECK(slot >= 0);
	console->sendCommand("move 0 Mover");
	CHECK(slot >= 0 && scene->server->game->players[slot]->teamID == PLAYER_TEAM_BLUE);

	// The handshake ends with the joiner's own team, sent to it after SERVER_INFO
	size_t before = harness::sent().size();
	acceptVersion(a);
	CHECK(ownTeamSent(a, slot, before) == PLAYER_TEAM_BLUE);

	// A joiner still in spectator gets no team message: nothing changed for it
	UINT4 b = harness::connect();
	net_clsv_svcl_player_info infoB = {};
	std::snprintf(infoB.playerName, sizeof(infoB.playerName), "%s", "Watcher");
	harness::deliver(b, NET_CLSV_SVCL_PLAYER_INFO, infoB);
	harness::tick();
	before = harness::sent().size();
	acceptVersion(b);
	CHECK(harness::slotOf(b) >= 0 && ownTeamSent(b, harness::slotOf(b), before) == -2);

	harness::stop();
	if (harness::failures() == 0)
		std::printf("ok: join team\n");
	return harness::failures() ? 1 : 0;
}
