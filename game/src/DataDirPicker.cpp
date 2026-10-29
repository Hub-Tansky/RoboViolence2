// Client start-up: finds the player's BaboViolent 2 data, or offers to download it on first run (ADR 0016, ADR 0040).
#ifndef CONSOLE

#include "DataDirPicker.h"
#include "DataDownload.h"
#include "Paths.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace
{
	const char * TITLE = "Robo Violence 2";


	// A small window with a progress bar; closing it cancels
	struct Progress
	{
		SDL_Window * window = 0;
		SDL_Renderer * renderer = 0;

		Progress() { SDL_CreateWindowAndRenderer(TITLE, 480, 40, 0, &window, &renderer); }
		~Progress() { SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); }

		// false when the player closed the window
		bool show(const std::string & title, double fraction)
		{
			SDL_Event e;
			while (SDL_PollEvent(&e))
				if (e.type == SDL_EVENT_QUIT || e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
					return false;
			SDL_SetWindowTitle(window, title.c_str());
			SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
			SDL_RenderClear(renderer);
			SDL_FRect bar = { 8, 8, (float)(464 * std::clamp(fraction, 0.0, 1.0)), 24 };
			SDL_SetRenderDrawColor(renderer, 230, 160, 40, 255);
			SDL_RenderFillRect(renderer, &bar);
			SDL_RenderPresent(renderer);
			return true;
		}
	};

	enum Step { STEP_OK, STEP_FAILED, STEP_CANCELLED };

	// Runs args until it exits; with sizeOf set, the bar follows that file's size against ORIGINAL_ZIP_BYTES
	Step runStep(Progress & ui, const std::vector<std::string> & args, const std::string & label,
		const std::string & sizeOf, std::string & error)
	{
		std::vector<const char *> argv;
		for (size_t i = 0; i < args.size(); ++i)
			argv.push_back(args[i].c_str());
		argv.push_back(0);
		SDL_PropertiesID props = SDL_CreateProperties();
		SDL_SetPointerProperty(props, SDL_PROP_PROCESS_CREATE_ARGS_POINTER, (void *)argv.data());
#ifdef BV2_PLATFORM_WINDOWS
		SDL_SetBooleanProperty(props, SDL_PROP_PROCESS_CREATE_BACKGROUND_BOOLEAN, true); // no console window
#endif
		SDL_Process * process = SDL_CreateProcessWithProperties(props);
		SDL_DestroyProperties(props);
		if (!process)
		{
			error = "could not start " + args[0] + ": " + SDL_GetError();
			return STEP_FAILED;
		}
		Step result = STEP_OK;
		int exitCode = 0;
		while (!SDL_WaitProcess(process, false, &exitCode))
		{
			double fraction = 1.0;
			std::string title = label;
			if (!sizeOf.empty())
			{
				std::error_code ec;
				uintmax_t bytes = fs::file_size(fs::path(sizeOf), ec);
				fraction = ec ? 0.0 : std::min(0.99, (double)bytes / bv2::ORIGINAL_ZIP_BYTES);
				title += " " + std::to_string((int)(fraction * 100)) + "%";
			}
			if (!ui.show(title, fraction))
			{
				SDL_KillProcess(process, true);
				SDL_WaitProcess(process, true, &exitCode);
				result = STEP_CANCELLED;
				break;
			}
			SDL_Delay(50);
		}
		SDL_DestroyProcess(process);
		if (result == STEP_OK && exitCode != 0)
		{
			error = args[0] + " exited with code " + std::to_string(exitCode);
			result = STEP_FAILED;
		}
		return result;
	}

	// Downloads the pinned upstream zip into partial, extracts main/ and installs it at originalDataDir() (ADR 0040).
	// On Windows the tools report no exit code (background process), so the hash check is the verdict there.
	Step fetchAndInstall(Progress & ui, const fs::path & partial, std::string & error)
	{
		const std::string zip = (partial / "bv2.zip").string();
		Step step = runStep(ui, bv2::downloadArgs(zip), "Downloading BaboViolent 2 data", zip, error);
		if (step != STEP_OK)
			return step;
		step = runStep(ui, bv2::extractArgs(zip, partial.string()), "Unpacking BaboViolent 2 data", "", error);
		if (step != STEP_OK)
			return step;
		ui.show("Checking BaboViolent 2 data", 1.0);
		const std::string data = bv2::extractedDataDir(partial.string());
		if (!(error = bv2::firstMismatch(data, bv2::originalMainHashes())).empty())
		{
			error = "the download or unpacking failed (" + error + " missing or wrong)";
			return STEP_FAILED;
		}
		if (!(error = bv2::replaceDir(data, bv2::originalDataDir())).empty())
		{
			error = "could not install into " + bv2::originalDataDir() + ": " + error;
			return STEP_FAILED;
		}
		return STEP_OK;
	}

	// fetchAndInstall in a fresh <originalDataDir>.partial folder, removed afterwards
	Step downloadData(std::string & error)
	{
		const fs::path partial = bv2::originalDataDir() + ".partial";
		std::error_code ec;
		fs::remove_all(partial, ec);
		fs::create_directories(partial, ec);
		Step step;
		{
			Progress ui;
			step = fetchAndInstall(ui, partial, error);
		}
		fs::remove_all(partial, ec);
		return step;
	}

	enum Button { PLACEHOLDERS, DOWNLOAD, BROWSER };

	Button firstRunBox()
	{
		const SDL_MessageBoxButtonData buttons[2] = {
			{ SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, DOWNLOAD, "Download and play" },
			{ SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, PLACEHOLDERS, "Use placeholders" },
		};
		// explicit line breaks: the X11 message box doesn't wrap
		const char * text =
			"Robo Violence 2 is in alpha stage. This is in-progress experimental version with placeholder art.\n\n"
			"The GPL covers the code only. The original assets (maps, models, textures, sounds, music, font)\n"
			"and the non-English translations are not part of this project. Replacements will be created\n"
			"by this project later.\n\n"
			"For now we recommend using your own original BaboViolent 2 data.\n\n"
			"\"Download and play\" downloads it (about 17 MB) from github.com/Daivuk/BaboViolent2."; // ORIGINAL_ZIP_BYTES
		const SDL_MessageBoxData box = { SDL_MESSAGEBOX_INFORMATION, 0, TITLE, text, 2, buttons, 0 };
		int button = PLACEHOLDERS;
		return SDL_ShowMessageBox(&box, &button) ? (Button)button : PLACEHOLDERS; // closing the box (-1) counts as placeholders
	}

	Button retryBox(const std::string & error)
	{
		const SDL_MessageBoxButtonData buttons[3] = {
			{ SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, DOWNLOAD, "Try again" },
			{ 0, BROWSER, "Open in browser" },
			{ SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, PLACEHOLDERS, "Use placeholders" },
		};
		const std::string text = "Downloading the BaboViolent 2 data failed:\n" + error + "\n\n"
			"Try again, or download it in your browser, unzip it into Downloads and restart Robo Violence 2.";
		const SDL_MessageBoxData box = { SDL_MESSAGEBOX_WARNING, 0, TITLE, text.c_str(), 3, buttons, 0 };
		int button = PLACEHOLDERS;
		return SDL_ShowMessageBox(&box, &button) ? (Button)button : PLACEHOLDERS;
	}

	// The folder the data was installed in; "" when the player picks placeholders or the browser
	std::string askDataDir()
	{
		Button next = firstRunBox();
		while (next == DOWNLOAD)
		{
			std::string error;
			Step step = downloadData(error);
			if (step == STEP_OK)
				return bv2::originalDataDir();
			next = step == STEP_CANCELLED ? firstRunBox() : retryBox(error);
		}
		if (next == BROWSER)
			SDL_OpenURL(bv2::BROWSER_ZIP_URL);
		return "";
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
