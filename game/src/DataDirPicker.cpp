// Client start-up: finds the player's BaboViolent 2 data, or asks for it on first run (Phase C step 1, ADR 0016).
#ifndef CONSOLE

#include "DataDirPicker.h"
#include "Paths.h"

#include <SDL3/SDL.h>

#include <atomic>
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

	const char * DATA_URL = "https://github.com/Daivuk/BaboViolent2/archive/refs/heads/master.zip";

	enum Button { PLACEHOLDERS, CHOOSE, DOWNLOAD };

	// "" when the player picks placeholders
	std::string askDataDir()
	{
		const SDL_MessageBoxButtonData buttons[3] = {
			{ SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, CHOOSE, "Choose folder..." },
			{ 0, DOWNLOAD, "Download BV2 data" },
			{ SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, PLACEHOLDERS, "Use placeholders" },
		};
		// explicit line breaks: the X11 message box doesn't wrap
		const std::string text = std::string(
			"[Temporary workaround] Robo Violence 2 looks better with your own copy of the original\n"
			"BaboViolent 2 data, which is not included.\n\n"
			"It is strongly recommended to download the original game from GitHub:\n") + DATA_URL + "\n"
			"or \"Download BV2 data\" below.\n\n"
			"Unzip it, then choose the BaboViolent2-master folder. Or after download of BV2 to\n"
			"Downloads, Desktop or Documents restart Robo2 - it is found on the next start automatically.\n\n"
			"Without it the game starts with placeholder graphics and no music :(";
		const SDL_MessageBoxData box = { SDL_MESSAGEBOX_INFORMATION, 0, TITLE, text.c_str(), 3, buttons, 0 };
		for (;;)
		{
			int button = PLACEHOLDERS;
			bool shown = SDL_ShowMessageBox(&box, &button);
			if (shown && button == DOWNLOAD)
			{
				SDL_OpenURL(DATA_URL);
				continue;
			}
			if (!shown || button != CHOOSE) // closing the box (-1) counts as placeholders
				return "";
			std::string picked = askFolder();
			if (picked.empty())
				continue;
			std::string data = bv2::findDataIn(picked);
			if (!data.empty())
				return data;
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, TITLE,
				"That folder has no BaboViolent 2 data (main/textures).\n"
				"Choose the unzipped BaboViolent2-master folder or a folder inside it that contains main/.", 0);
		}
	}
}

namespace bv2
{
	void chooseDataDir(int argc, char ** argv)
	{
		if (dataFromEnv())
			return;
		for (int i = 1; i < argc; ++i)
			if (std::strcmp(argv[i], "--placeholders") == 0)
				return;

		std::string dir = savedDataDir();
		if (!dir.empty())
		{
			setDataRoot(dir);
			return;
		}
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
