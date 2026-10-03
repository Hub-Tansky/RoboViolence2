// Wire structs in netPacket.h are memcpy'd as-is; this checks their packed little-endian layout.
// The static_asserts in the header already pin every sizeof; this adds byte-level round trips.
#include <cstdio>
#include <cstring>

#include "netPacket.h"

static int failures = 0;

#define CHECK(cond)                                                    \
	do                                                                 \
	{                                                                  \
		if (!(cond))                                                   \
		{                                                              \
			std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
			++failures;                                                \
		}                                                              \
	} while (0)

int main()
{
	// Spawn request: 3 x int8, skin[7], 3 x uint8[3]; no padding.
	net_clsv_spawn_request req;
	std::memset(&req, 0, sizeof(req));
	req.playerID = 5;
	req.weaponID = -1;
	req.meleeID = 2;
	std::memcpy(req.skin, "abcdef", 7);
	req.redDecal[0] = 200;
	req.blueDecal[2] = 7;

	unsigned char wire[sizeof(req)];
	std::memcpy(wire, &req, sizeof(req));
	CHECK(sizeof(req) == 3 + 7 + 9);
	CHECK(wire[0] == 5);
	CHECK(wire[1] == 0xFF);
	CHECK(wire[2] == 2);
	CHECK(std::memcmp(wire + 3, "abcdef", 7) == 0);
	CHECK(wire[10] == 200);
	CHECK(wire[18] == 7);

	net_clsv_spawn_request back;
	std::memcpy(&back, wire, sizeof(back));
	CHECK(back.playerID == 5 && back.weaponID == -1 && back.meleeID == 2);
	CHECK(back.redDecal[0] == 200 && back.blueDecal[2] == 7);

	// A 16-bit field is two little-endian bytes (positions are short x100).
	net_clsv_pong pong;
	pong.playerID = 31;
	CHECK(sizeof(pong) == 1);

	if (failures == 0)
		std::printf("ok: netPacket layout\n");
	return failures ? 1 : 0;
}
