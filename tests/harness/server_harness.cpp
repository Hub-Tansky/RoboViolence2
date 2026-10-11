#include "server_harness.h"

#include "Zeven.h"
#include "Scene.h"
#include "Console.h"
#include "CMaster.h"
#include "GameVar.h"
#include "Paths.h"
#include "netPacket.h"

#include <cstring>

// Globals bv2dedicated's main.cpp defines
Scene * scene = 0;
char * bbNetVersion;
void dkwForceQuit() {}

namespace
{
	class StringInterface : public CStringInterface
	{
	public:
		virtual void updateString(CString * string, char * newValue) { *string = newValue; }
	} stringInterface;

	UINT4 nextClientID = 1;
}

namespace harness
{
	bool start(const char * argv0, const char * launchScript)
	{
		const char * argv[] = {argv0};
		bv2::pathsInit(argv0);
		dksvarInit(&stringInterface);
		bv2::loadConfigLayers(1, argv);
		FetchDBInfos();
		dkcInit(30);
		bb_init();
		bbNetVersion = bb_getVersion();
		console = new Console();
		console->init();
		master = new CMaster();
		scene = new Scene();
		console->sendCommand(CString("execute %s", launchScript));
		return scene->server && scene->server->isRunning;
	}

	void tick(int frames)
	{
		for (int i = 0; i < frames; ++i)
		{
			console->update(1 / 30.0f);
			scene->update(1 / 30.0f);
		}
	}

	UINT4 connect()
	{
		UINT4 id = nextClientID++;
		queueConnect(id);
		tick();
		return id;
	}

	UINT4 join()
	{
		UINT4 id = connect();
		net_clsv_gameversion_accepted accepted = {};
		deliver(id, NET_CLSV_GAMEVERSION_ACCEPTED, &accepted, sizeof(accepted));
		net_clsv_svcl_player_info info = {};
		strcpy(info.playerName, "Tester");
		deliver(id, NET_CLSV_SVCL_PLAYER_INFO, &info, sizeof(info));
		return id;
	}

	int slotOf(UINT4 id)
	{
		for (int i = 0; i < MAX_PLAYER; ++i)
			if (scene->server->game->players[i] && scene->server->game->players[i]->babonetID == (INT4)id) return i;
		return -1;
	}

	void deliver(UINT4 from, int typeID, const void * data, int size)
	{
		queueMessage(from, typeID, data, size);
		tick();
	}

	void stop()
	{
		delete scene;
		delete master;
		delete console;
		scene = 0;
		master = 0;
		console = 0;
	}
}
