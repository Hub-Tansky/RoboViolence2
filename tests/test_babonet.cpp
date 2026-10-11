// R7 (Phase B step 7): a message larger than babonet's 3072-byte receive buffer arrives over several reads and is
// reassembled in order. A loopback server and client in one process.
#include "baboNet.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <thread>
#include <vector>

int main()
{
	const unsigned short port = 10208;
	if (bb_init() || bb_serverCreate(false, 4, port))
	{
		std::printf("FAIL: no server\n");
		return 1;
	}
	UINT4 client = bb_clientConnect("127.0.0.1", port);
	std::vector<char> sent(10000);
	for (size_t i = 0; i < sent.size(); ++i) sent[i] = (char)(i * 7 + i / 251); // no repeating 3072-byte pattern
	bool queued = false, received = false, intact = false;
	for (int frame = 0; frame < 300 && !received; ++frame)
	{
		char ip[16];
		bb_serverUpdate(1 / 30.0f, UPDATE_SEND_RECV, ip);
		bb_clientUpdate(client, 1 / 30.0f, UPDATE_SEND_RECV);
		if (!queued && frame > 10)
		{
			bb_clientSend(client, sent.data(), (int)sent.size(), 42);
			queued = true;
		}
		UINT4 from;
		int typeID, size = 0;
		while (char * data = bb_serverReceive(from, typeID, &size))
			if (typeID == 42)
			{
				received = true;
				intact = size == (int)sent.size() && std::equal(sent.begin(), sent.end(), data);
			}
		std::this_thread::sleep_for(std::chrono::milliseconds(2));
	}
	if (!received || !intact)
	{
		std::printf("FAIL: received %d, intact %d\n", received, intact);
		return 1;
	}
	std::printf("ok: 10000-byte message reassembled\n");
	return 0;
}
