// A player's IP stays on the server: the player info relayed to everyone and the player list a new client gets carry
// no IP (website/privacy.html), while the server keeps it for bans.
#include "server_harness.h"

#include "Game.h"
#include "Scene.h"
#include "Server.h"

#include <cstdio>
#include <cstring>

extern Scene * scene;

int main(int, char ** argv)
{
	if (!harness::start(argv[0])) return 1;
	UINT4 a = harness::join();
	harness::tick();
	UINT4 b = harness::join();
	harness::tick();
	CHECK(harness::slotOf(a) >= 0 && harness::slotOf(b) >= 0);
	CHECK(std::strcmp(scene->server->game->players[harness::slotOf(a)]->playerIP, "127.0.0.1") == 0);

	int infos = 0, states = 0;
	for (const harness::SentPacket & p : harness::sent())
	{
		if (p.typeID == NET_CLSV_SVCL_PLAYER_INFO)
		{
			++infos;
			CHECK(((const net_clsv_svcl_player_info *)p.data.data())->playerIP[0] == '\0');
		}
		if (p.typeID == NET_SVCL_PLAYER_ENUM_STATE)
		{
			++states;
			CHECK(((const net_svcl_player_enum_state *)p.data.data())->playerIP[0] == '\0');
		}
	}
	CHECK(infos >= 2 && states >= 1); // both joins relayed, and b was told about a
	harness::stop();
	std::printf("%d player infos, %d player states\n", infos, states);
	return harness::failures();
}
