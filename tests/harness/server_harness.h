// In-process dedicated server for tests (Phase B step 3): game/src's server on a fake babonet (fake_babonet.cpp).
#pragma once
#include "platform.h"
#include <vector>

namespace harness
{
	struct SentPacket
	{
		int typeID;
		INT4 destination; // babonetID, 0 = broadcast
		std::vector<char> data;
	};

	// Starts the server like bv2dedicated's main with `execute <launchScript>`. argv0 locates main/.
	bool start(const char * argv0, const char * launchScript = "CTF");
	// Runs `frames` 30 Hz frames of the game loop (network receive, game update, sends).
	void tick(int frames = 1);
	// A new TCP client: the server creates a player for it on the next frame. Returns its babonetID.
	UINT4 connect();
	// One message from a client, exactly `size` bytes, handled on the next frame.
	void deliver(UINT4 from, int typeID, const void * data, int size);
	std::vector<SentPacket> & sent();
	void stop();

	// fake_babonet.cpp
	void queueConnect(UINT4 id);
	void queueMessage(UINT4 from, int typeID, const void * data, int size);
}
