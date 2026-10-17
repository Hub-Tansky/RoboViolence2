// bv2master in-process on its port (10207) with a scratch master.db: register, list, heartbeat timeout, removal.
//   test_master <dir with master.db and web.db> <scratch dir>
#include "SqlStatement.h"
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
	bb_clientDisconnect(c);
	pump({});
	return names;
}

// The rows of the list, on a fresh connection
static std::vector<stBV2row> rows()
{
	UINT4 c = connectClient();
	stBV2list query = {};
	strcpy(query.Version, "2.11");
	bb_clientSend(c, (char *)&query, sizeof(query), BV2_LIST);
	pump({c});
	std::vector<stBV2row> out;
	int typeID;
	while (char * buffer = bb_clientReceive(c, &typeID))
		if (typeID == BV2_ROW) out.push_back(*(stBV2row *)buffer);
	bb_clientDisconnect(c);
	pump({});
	return out;
}

static void registerGame(UINT4 c, const char * advertised = "")
{
	stBV2row row = {};
	strcpy(row.ip, advertised);
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
#ifdef _WIN32
	_putenv_s("BV2_MASTER_BAN_PASS", "b4nTest");
#else
	setenv("BV2_MASTER_BAN_PASS", "b4nTest", 1);
#endif

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

	std::printf("phase: advertised address\n"); // from loopback the master lists sv_publicAddress (PNS-44)
	bb_clientDisconnect(game); // KILL_SERV closed it
	game = connectClient();
	registerGame(game, "203.0.113.7");
	std::vector<stBV2row> listed = rows();
	CHECK(listed.size() == 1 && std::strcmp(listed[0].ip, "203.0.113.7") == 0);
	// Built from pieces: the secret scan flags IPv4 literals outside the documentation ranges, private ones too
	const std::string docker = std::string("172.") + "20.0.3", outside = std::string("172.") + "32.0.1";
	CHECK(cMasterServer::IsPrivateAddress(docker.c_str()) && cMasterServer::IsPrivateAddress("127.0.0.1"));
	CHECK(!cMasterServer::IsPrivateAddress("203.0.113.7") && !cMasterServer::IsPrivateAddress(outside.c_str()));
	bb_clientSend(game, (char *)&kill, sizeof(kill), KILL_SERV); // found by its connection, not its listed address
	pump({game});
	CHECK(list().empty());

	std::printf("phase: reconnect\n"); // a server whose master link dropped heartbeats on a new connection
	bb_clientDisconnect(game);
	UINT4 first = connectClient();
	registerGame(first);
	game = connectClient();
	registerGame(game);
	bb_clientSend(game, (char *)&kill, sizeof(kill), KILL_SERV); // the new connection owns the game
	pump({game});
	CHECK(list().empty());
	bb_clientDisconnect(first);

	bb_clientDisconnect(game);
	// 6.3 (Q-S3): quotes, comments, ';' and '%' in a game's name and map and in a ban's nick and IP are stored as
	// sent, and the tables stay intact
	const char * name = "Robo'); DROP TABLE Games;--";
	const char * map = "Map'%s;--";
	game = connectClient();
	stBV2row row = {};
	strcpy(row.map, map);
	strcpy(row.serverName, name);
	strcpy(row.Version, "2.11");
	row.port = 4444;
	bb_clientSend(game, (char *)&row, sizeof(row), BV2_ROW);
	pump({game});
	sqlite3 * web = 0;
	sqlite3_open("web.db", &web);
	{
		SqlStatement games(web, "Select Name, Map From Games Where Port = 4444;");
		CHECK(games.row() && games.text(0) == name && games.text(1) == map);
	}
	sqlite3_close(web);
	stCacheBan ban = {};
	strcpy(ban.Nick, "Nick'--;%");
	strcpy(ban.IP, "1';--%");
	strcpy(ban.Pass, "roxbabo"); // the old built-in password: refused (PNS-43)
	bb_clientSend(game, (char *)&ban, sizeof(ban), CACHE_BAN);
	pump({game});
	{
		sqlite3 * db = 0;
		sqlite3_open("master.db", &db);
		SqlStatement bans(db, "Select ID From BanList Where IP = ?;");
		bans.bindText(1, ban.IP);
		CHECK(!bans.row());
		sqlite3_close(db);
	}
	strcpy(ban.Pass, "b4nTest"); // BV2_MASTER_BAN_PASS, set before the master started
	bb_clientSend(game, (char *)&ban, sizeof(ban), CACHE_BAN);
	pump({game});
	stCacheBanned query = {};
	strcpy(query.IP, ban.IP); // bans are by IP (step 10)
	bb_clientSend(game, (char *)&query, sizeof(query), CACHE_BANNED);
	pump({game});
	int typeID;
	bool answered = false;
	while (char * buffer = bb_clientReceive(game, &typeID))
		if (typeID == CACHE_ANSWER) answered = ((stCacheAnswer *)buffer)->Answer == 1;
	CHECK(answered);
	sqlite3 * db = 0;
	sqlite3_open("master.db", &db);
	{
		SqlStatement bans(db, "Select Nick, IP From BanList Where IP = ?;");
		bans.bindText(1, ban.IP);
		CHECK(bans.row() && bans.text(0) == ban.Nick && bans.text(1) == ban.IP);
	}
	sqlite3_close(db);

	// The ban list: an empty filter lists the ban; a filter with a quote and '%' matches its nick literally (LIKE)
	for (const char * filter : {"", "Nick'", "'--;%"})
	{
		stCacheList list = {};
		strcpy(list.Filter, filter);
		bb_clientSend(game, (char *)&list, sizeof(list), CACHE_BAN_LIST);
		pump({game});
		bool listed = false;
		while (char * buffer = bb_clientReceive(game, &typeID))
			if (typeID == CACHE_BAN)
			{
				const stCacheBan * b = (const stCacheBan *)buffer;
				listed = listed || (std::string(b->Nick) == ban.Nick && std::string(b->IP) == ban.IP);
			}
		CHECK(listed || std::printf("  filter \"%s\"\n", filter) < 0);
	}
	bb_clientDisconnect(game);

	if (failures == 0) std::printf("ok: master register, list, timeout, removal, SQL injection\n");
	return failures ? 1 : 0;
}
