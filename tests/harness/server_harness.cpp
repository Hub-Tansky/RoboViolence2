#include "server_harness.h"

#include "Zeven.h"
#include "Scene.h"
#include "Console.h"
#include "CMaster.h"
#include "GameVar.h"
#include "Paths.h"

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
