// In-process dedicated server for tests (Phase B step 3): game/src's server on a fake babonet (fake_babonet.cpp).
#pragma once
#include "platform.h"
#include <cstdio>
#include <vector>

namespace harness
{
	struct SentPacket
	{
		int typeID;
		INT4 destination; // babonetID, 0 = broadcast
		std::vector<char> data;
	};

	// Starts the server like bv2dedicated's main with `execute <launchScript>`. argv0 locates main/. Once per process:
	// the engine globals (dksvar, dkc, console, scene) are not reset by stop().
	bool start(const char * argv0, const char * launchScript = "CTF");
	// Runs `frames` 30 Hz frames of the game loop (network receive, game update, sends).
	void tick(int frames = 1);
	// A new TCP client: the server creates a player for it on the next frame. Returns its babonetID.
	UINT4 connect();
	// A new client that has completed the join handshake (version accepted, player info "Tester"). Returns its babonetID.
	UINT4 join();
	// The player slot of a connected client, -1 if it has none
	int slotOf(UINT4 id);
	// The client's TCP connection closes: the server frees its player on the next frame.
	void disconnect(UINT4 id);
	// One message from a client, exactly `size` bytes, handled on the next frame.
	void deliver(UINT4 from, int typeID, const void * data, int size);
	std::vector<SentPacket> & sent();
	void stop();

	// One message from a client: a wire struct
	template <typename T> void deliver(UINT4 from, int typeID, const T & m) { deliver(from, typeID, &m, sizeof(m)); }
	// A joined client that has spawned on blue with weaponID and knives (team request, spawn, 60 frames past the fire
	// delay). Returns its babonetID.
	UINT4 spawned(int weaponID);

	// Test checks: CHECK counts failures, which main returns
	inline int & failures() { static int n = 0; return n; }
#define CHECK(cond)                                                                       \
	do                                                                                    \
	{                                                                                     \
		if (!(cond))                                                                      \
		{                                                                                 \
			std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);                    \
			++harness::failures();                                                        \
		}                                                                                 \
	} while (0)

	// Internal: fake_babonet.cpp's queues, filled by connect() and deliver()
	void queueConnect(UINT4 id);
	void queueMessage(UINT4 from, int typeID, const void * data, int size);
}
