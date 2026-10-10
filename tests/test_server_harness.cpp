// The in-process server starts, accepts a client and sends it the game state.
#include "server_harness.h"

#include <cstdio>

int main(int, char ** argv)
{
	if (!harness::start(argv[0]))
	{
		std::printf("FAIL: server did not start\n");
		return 1;
	}
	UINT4 id = harness::connect();
	harness::tick(30);
	int toClient = 0;
	for (const harness::SentPacket & p : harness::sent())
		if (p.destination == (INT4)id) ++toClient;
	harness::stop();
	std::printf("%d packets to client %u\n", toClient, id);
	return toClient > 0 ? 0 : 1;
}
