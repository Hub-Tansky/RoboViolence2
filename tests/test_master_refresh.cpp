// The server browser's Refresh lists a server hosted after the browser first listed (ROBO-217).
// The game's CMaster talks to the real master in-process through a TCP relay. The relay then goes silent on the
// open session, as a home router does when it drops an idle connection without telling either end, and a server
// registers. Refresh has to list it without a restart.
//   test_master_refresh <dir with master.db and web.db> <scratch dir>
#include "CMaster.h"
#include "Console.h"
#include "GameVar.h"
#include "Paths.h"
#include "Scene.h"
#include "cNetManager.h"

#ifdef _WIN32
#	include <winsock2.h>
typedef SOCKET Socket;
typedef int socklen_t;
#	define closeSocket closesocket
#else
#	include <arpa/inet.h>
#	include <fcntl.h>
#	include <netinet/in.h>
#	include <sys/select.h>
#	include <sys/socket.h>
#	include <unistd.h>
typedef int Socket;
#	define INVALID_SOCKET (-1)
#	define closeSocket close
#endif
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

// Globals bv2dedicated's main.cpp defines
Scene* scene = 0;
char* bbNetVersion;
void dkwForceQuit() {}

namespace
{
	class StringInterface : public CStringInterface
	{
	public:
		virtual void updateString(CString* string, char* newValue) { *string = newValue; }
	} stringInterface;

	int failures = 0;

#define CHECK(cond)                                                     \
	do                                                                  \
	{                                                                   \
		if (!(cond))                                                    \
		{                                                               \
			std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
			++failures;                                                 \
		}                                                               \
	} while (0)

	void nonBlocking(Socket s)
	{
#ifdef _WIN32
		u_long on = 1;
		ioctlsocket(s, FIONBIO, &on);
#else
		fcntl(s, F_SETFL, fcntl(s, F_GETFL, 0) | O_NONBLOCK);
#endif
	}

	// Forwards each accepted connection to the master; silence() makes the open ones swallow traffic both ways
	// while new connections still work.
	struct Relay
	{
		struct Pair
		{
			Socket client, master;
			bool silent;
		};
		Socket listener = INVALID_SOCKET;
		unsigned short port = 0;
		std::vector<Pair> pairs;
		int accepted = 0;

		bool start(unsigned short masterPort)
		{
			target = masterPort;
			listener = socket(AF_INET, SOCK_STREAM, 0);
			sockaddr_in addr = {};
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
			addr.sin_port = 0;
			if (bind(listener, (sockaddr*)&addr, sizeof(addr)) != 0 || listen(listener, 8) != 0)
				return false;
			socklen_t len = sizeof(addr);
			getsockname(listener, (sockaddr*)&addr, &len);
			port = ntohs(addr.sin_port);
			nonBlocking(listener);
			return true;
		}

		void silence()
		{
			for (Pair& p : pairs)
				p.silent = true;
		}

		void pump()
		{
			Socket c = accept(listener, 0, 0);
			if (c != INVALID_SOCKET)
			{
				Socket m = socket(AF_INET, SOCK_STREAM, 0);
				sockaddr_in addr = {};
				addr.sin_family = AF_INET;
				addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
				addr.sin_port = htons(target);
				if (connect(m, (sockaddr*)&addr, sizeof(addr)) == 0)
				{
					nonBlocking(c);
					nonBlocking(m);
					pairs.push_back({c, m, false});
					++accepted;
				}
				else
				{
					closeSocket(c);
					closeSocket(m);
				}
			}
			for (Pair& p : pairs)
			{
				forward(p.client, p.master, p.silent);
				forward(p.master, p.client, p.silent);
			}
		}

	private:
		unsigned short target = 0;

		// A silent pair reads and drops: nothing arrives and nothing is closed
		static void forward(Socket from, Socket to, bool silent)
		{
			char buffer[4096];
			for (;;)
			{
				int n = (int)recv(from, buffer, sizeof(buffer), 0);
				if (n <= 0)
					return;
				if (!silent)
					send(to, buffer, n, 0);
			}
		}
	};

	cNetManager* masterServer = 0;
	Relay relay;
	std::vector<UINT4> hosts;

	// About `frames` frames of 1/30 s for the master, the relay, the browsing game and the hosts
	void pump(int frames)
	{
		for (int i = 0; i < frames; ++i)
		{
			masterServer->Update(1 / 30.0f);
			relay.pump();
			master->update(1 / 30.0f);
			for (UINT4 h : hosts)
				bb_clientUpdate(h, 1 / 30.0f, UPDATE_SEND_RECV);
			relay.pump();
			std::this_thread::sleep_for(std::chrono::milliseconds(2));
		}
	}

	// A hosted server's heartbeat, straight to the master on its own connection
	void host(const char* name, unsigned short port)
	{
		UINT4 h = bb_clientConnect("127.0.0.1", 10207);
		hosts.push_back(h);
		pump(15);
		stBV2row row = {};
		strcpy(row.map, "CTF-Placeholder");
		strcpy(row.serverName, name);
		strcpy(row.Version, "2.11");
		row.port = port;
		row.maxPlayer = 10;
		bb_clientSend(h, (char*)&row, sizeof(row), BV2_ROW);
		pump(15);
	}
}

int main(int argc, char** argv)
{
	if (argc != 3)
	{
		std::printf("usage: test_master_refresh <dir with master.db and web.db> <scratch dir>\n");
		return 1;
	}
	std::setvbuf(stdout, 0, _IONBF, 0);
	fs::create_directories(argv[2]);
	for (const char* db : {"master.db", "web.db"})
		fs::copy_file(fs::path(argv[1]) / db, fs::path(argv[2]) / db, fs::copy_options::overwrite_existing);
	fs::current_path(argv[2]); // the master opens master.db and web.db in its working directory

	masterServer = new cNetManager();
	masterServer->Init(); // bb_init too
	if (!relay.start(10207))
	{
		std::printf("FAIL relay: no loopback port\n");
		return 1;
	}

	const char* args[] = {argv[0]};
	bv2::pathsInit(argv[0]);
	dksvarInit(&stringInterface);
	bv2::loadConfigLayers(1, args);
	bbNetVersion = bb_getVersion();
	console = new Console();
	console->init();
	gameVar.cl_masterServers = CString("127.0.0.1:%i", (int)relay.port);
	master = new CMaster();

	std::printf("phase: first list\n");
	host("First Server", 3333);
	master->requestGames();
	pump(30);
	CHECK(master->gamesFound() == 1);
	CHECK(relay.accepted == 1);

	std::printf("phase: the router drops the idle session, a new server registers, Refresh\n");
	relay.silence();
	host("Second Server", 3334);
	master->requestGames();
	pump(30);
	CHECK(master->gamesFound() == 2);
	CHECK(relay.accepted == 2);

	delete master;
	master = 0;
	for (UINT4 h : hosts)
		bb_clientDisconnect(h);
	delete masterServer;

	if (failures)
		return 1;
	std::printf("ok: Refresh lists a server hosted after the first list\n");
	return 0;
}
