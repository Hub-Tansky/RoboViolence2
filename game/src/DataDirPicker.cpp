// Client start-up: finds the player's BaboViolent 2 data, or asks for it on first run (Phase C step 1, ADR 0016).
#ifndef CONSOLE

#include "DataDirPicker.h"
#include "Paths.h"

#include <SDL3/SDL.h>

#include <atomic>
#include <cstdlib>
#include <cstring>
#include <string>

namespace
{
	const char * TITLE = "Robo Violence 2";

	struct FolderChoice
	{
		std::atomic<bool> done{false};
		std::string path; // "" when cancelled or failed
	};

	// SDL may call this on another thread
	void SDLCALL onFolder(void * userdata, const char * const * files, int)
	{
		FolderChoice * choice = (FolderChoice *)userdata;
		if (files && files[0])
			choice->path = files[0];
		choice->done = true;
	}

	std::string askFolder()
	{
		FolderChoice choice;
		SDL_ShowOpenFolderDialog(onFolder, &choice, 0, 0, false);
		while (!choice.done)
		{
			SDL_PumpEvents();
			SDL_Delay(10);
		}
		return choice.path;
	}

	// "" when the player picks placeholders
	std::string askDataDir()
	{
		const SDL_MessageBoxButtonData buttons[2] = {
			{ SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Choose folder..." },
			{ SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "Use placeholders" },
		};
		const SDL_MessageBoxData box = { SDL_MESSAGEBOX_INFORMATION, 0, TITLE,
			"Robo Violence 2 plays with your own copy of the BaboViolent 2 data, which is not included.\n\n"
			"Choose the folder that contains main/ (with textures, sounds and maps).\n\n"
			"Without it the game starts with placeholder graphics and no music.",
			2, buttons, 0 };
		for (;;)
		{
			int button = 0;
			if (!SDL_ShowMessageBox(&box, &button) || button != 1)
				return "";
			std::string picked = askFolder();
			if (picked.empty())
				continue;
			std::string data = bv2::findDataIn(picked);
			if (!data.empty())
				return data;
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, TITLE,
				"That folder has no BaboViolent 2 data (main/textures). Choose the folder that contains main/.", 0);
		}
	}
}

namespace bv2
{
	void chooseDataDir(int argc, char ** argv)
	{
		const char * env = std::getenv("BV2_DATA_DIR");
		if (env && env[0])
			return;
		for (int i = 1; i < argc; ++i)
			if (std::strcmp(argv[i], "--placeholders") == 0)
				return;

		std::string dir = savedDataDir();
		if (dir.empty())
			dir = detectDataDir();
		if (dir.empty() && SDL_Init(SDL_INIT_VIDEO))
		{
			dir = askDataDir();
			SDL_QuitSubSystem(SDL_INIT_VIDEO);
		}
		if (!dir.empty() && setDataRoot(dir))
			saveDataDir(dir);
	}
}

#endif
