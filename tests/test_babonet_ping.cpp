// PNS-56: the server browser's ping (game/src/CPing.cpp) gets an answer even when another process on this host holds
// babonet's LAN port, as when master, server and client run on one machine. This process holds the port, starts itself
// as the game server (bb_peerBindPort) and pings it the way CPing does, within CPing's 1 s budget.
#include "baboNet.h"

#ifdef _WIN32
#include <winsock2.h>
#define closesocket_ closesocket
#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#define closesocket_ close
#endif

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>

static const unsigned short PORT = 10209;
static const unsigned short LAN_PORT = 11111; // BB_BROADCAST_PORT in engine/babonet/src/cPeer2Peer.h
static const int FPS = 30;
static const int SKIP = 77; // SKIP_RETURN_CODE in tests/CMakeLists.txt
static const std::chrono::milliseconds FRAME(1000 / FPS);

static int serve()
{
	bb_init();
	if (bb_peerBindPort(PORT)) return 1;
	for (int frame = 0; frame < 6 * FPS; ++frame)
	{
		bool isNew = false;
		bb_peerUpdate(1.0f / FPS, isNew);
		INT4 from;
		int typeID;
		while (bb_peerReceive(&from, &typeID)) {}
		std::this_thread::sleep_for(FRAME);
	}
	return 0;
}

// One ping as CPing sends it, one frame per real frame (babonet gives up on the peer after 2 s of frames and reports it
// then too); true when the server confirms it within CPing's 1 s
static bool ping()
{
	char ip[] = "127.0.0.1";
	int id = bb_peerSend(ip, PORT, 0, 0, 0, true);
	if (id <= 0) return false;
	bb_peerDelete(id);
	for (int frame = 0; frame < FPS; ++frame)
	{
		bool isNew = false;
		if (bb_peerUpdate(1.0f / FPS, isNew) == id && !isNew) return true;
		std::this_thread::sleep_for(FRAME);
	}
	bb_peerDelete(id, true);
	return false;
}

int main(int argc, char ** argv)
{
	if (argc > 1 && !std::strcmp(argv[1], "--serve")) return serve();

	bb_init();
	int lanHolder = (int)socket(AF_INET, SOCK_DGRAM, 0); // no SO_REUSEADDR: babonet's own bind of the port fails
#ifdef _WIN32
	BOOL exclusive = TRUE; // else babonet's SO_REUSEADDR takes the port over
	setsockopt(lanHolder, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (const char *)&exclusive, sizeof(exclusive));
#endif
	sockaddr_in lan;
	std::memset(&lan, 0, sizeof(lan));
	lan.sin_family = AF_INET;
	lan.sin_addr.s_addr = htonl(INADDR_ANY);
	lan.sin_port = htons(LAN_PORT);
	if (bind(lanHolder, (sockaddr *)&lan, sizeof(lan)))
	{
		std::printf("SKIP: LAN port %d already held by another process\n", LAN_PORT);
		return SKIP;
	}
	char query[1] = {0};
	bb_peerSend(-1, query, 1, sizeof(query)); // a LAN query with the port taken is dropped, not queued forever

	std::string server = std::string("\"") + argv[0] + "\" --serve";
	std::thread serverThread([&] { std::system(server.c_str()); });
	bool answered = false;
	for (int attempt = 0; attempt < 5 && !answered; ++attempt) answered = ping(); // the first ones may beat its bind
	serverThread.join();
	closesocket_(lanHolder);
	if (!answered)
	{
		std::printf("FAIL: no ping answer within 1 s\n");
		return 1;
	}
	std::printf("ok: ping answered with the LAN port taken\n");
	return 0;
}
