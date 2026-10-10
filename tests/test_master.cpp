// bv2master in-process on its port (10207) with a scratch master.db: register, list, heartbeat timeout, removal.
//   test_master <dir with master.db and web.db> <scratch dir>
#include "cNetManager.h"

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

static cNetManager * master;
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

// Runs the master and the clients for about `frames` frames of 1/30 s.
static void pump(const std::vector<UINT4> & clients, int frames = 30)
{
	for (int i = 0; i < frames; ++i)
	{
		master->Update(1 / 30.0f);
		for (UINT4 c : clients) bb_clientUpdate(c, 1 / 30.0f, UPDATE_SEND_RECV);
		std::this_thread::sleep_for(std::chrono::milliseconds(2));
	}
}

static UINT4 connectClient()
{
	UINT4 c = bb_clientConnect("127.0.0.1", 10207);
	pump({c});
	return c;
}

// Asks for the list on a fresh connection; returns the server names it gets.
static std::vector<std::string> list()
{
	UINT4 c = connectClient();
	stBV2list query = {};
	strcpy(query.Version, "2.11");
	bb_clientSend(c, (char *)&query, sizeof(query), BV2_LIST);
	pump({c});
	std::vector<std::string> names;
	int typeID;
	while (char * buffer = bb_clientReceive(c, &typeID))
		if (typeID == BV2_ROW) names.push_back(((stBV2row *)buffer)->serverName);
	std::printf("listed %d\n", (int)names.size());
	return names; // the connection stays open (closing it crashes on Windows, R28)
}

static void registerGame(UINT4 c)
{
	stBV2row row = {};
	strcpy(row.map, "CTF-Placeholder");
	strcpy(row.serverName, "Test Server");
	strcpy(row.Version, "2.11");
	row.port = 3333;
	row.maxPlayer = 10;
	bb_clientSend(c, (char *)&row, sizeof(row), BV2_ROW);
	pump({c});
}

int main(int argc, char ** argv)
{
	if (argc != 3)
	{
		std::printf("usage: test_master <dir with master.db and web.db> <scratch dir>\n");
		return 1;
	}
	std::setvbuf(stdout, 0, _IONBF, 0); // a crash keeps the phase lines below
	fs::create_directories(argv[2]);
	for (const char * db : {"master.db", "web.db"})
		fs::copy_file(fs::path(argv[1]) / db, fs::path(argv[2]) / db, fs::copy_options::overwrite_existing);
	fs::current_path(argv[2]); // the master opens master.db and web.db in its working directory

	master = new cNetManager();
	master->Init();

	std::printf("phase: register, list\n");
	UINT4 game = connectClient();
	registerGame(game);
	std::vector<std::string> names = list();
	CHECK(names.size() == 1 && names[0] == "Test Server");

	std::printf("phase: heartbeat timeout\n"); // 61 s without an update drops the game (GAME_TIMEOUT 60)
	master->Update(61.0f);
	pump({game});
	CHECK(list().empty());

	std::printf("phase: removal\n"); // a registered game sends KILL_SERV for its port
	bb_clientDisconnect(game);
	game = connectClient();
	registerGame(game);
	CHECK(list().size() == 1);
	stKillServ kill = {3333};
	bb_clientSend(game, (char *)&kill, sizeof(kill), KILL_SERV);
	pump({game});
	CHECK(list().empty());

	bb_clientDisconnect(game);
	if (failures == 0) std::printf("ok: master register, list, timeout, removal\n");
	return failures ? 1 : 0;
}
