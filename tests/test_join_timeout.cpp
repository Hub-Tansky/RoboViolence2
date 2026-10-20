// A connection that never answers the join handshake (a port scanner) loses its slot after Server::JOIN_TIMEOUT, so
// it can't keep /status above 0 and block staging's deploy. A client that passed the handshake keeps its slot while
// it loads, however long that takes.
#include "server_harness.h"

#include "Server.h"

#include <cstdio>

static const int FPS = 30; // the harness ticks 1/30 s per frame

static bool disconnectSentFor(int slot)
{
	for (const harness::SentPacket & p : harness::sent())
		if (p.typeID == NET_SVCL_PLAYER_DISCONNECT && ((const net_svcl_player_disconnect *)p.data.data())->playerID == slot)
			return true;
	return false;
}

int main(int, char ** argv)
{
	if (!harness::start(argv[0])) return 1;

	UINT4 silent = harness::connect();
	int slot = harness::slotOf(silent);
	CHECK(slot >= 0);
	harness::tick(int(Server::JOIN_TIMEOUT * FPS) - FPS);
	CHECK(harness::slotOf(silent) == slot);
	harness::tick(2 * FPS);
	CHECK(harness::slotOf(silent) == -1);
	CHECK(disconnectSentFor(slot));

	UINT4 joined = harness::join();
	CHECK(harness::slotOf(joined) >= 0);
	harness::tick(int(Server::JOIN_TIMEOUT * FPS) * 3);
	CHECK(harness::slotOf(joined) >= 0);

	harness::stop();
	if (harness::failures() == 0) std::printf("ok: join timeout\n");
	return harness::failures() ? 1 : 0;
}
