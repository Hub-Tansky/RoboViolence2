// Client start-up: finds the player's BaboViolent 2 data, or offers to download it on first run (ADR 0016, ADR 0040).
#ifndef CONSOLE

#include "DataDirPicker.h"
#include "DataDownload.h"
#include "Paths.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace
{
	const char * TITLE = "Robo Violence 2";

	// One upstream commit, so every player gets the data tools/original-assets.sha256 describes (ADR 0040)
#define BV2_UPSTREAM_COMMIT "7171c84d9b4d02bb74e846d5617fbb527bfba358"
	const char * ZIP_URL = "https://github.com/Daivuk/BaboViolent2/archive/" BV2_UPSTREAM_COMMIT ".zip";
	const char * ZIP_MAIN = "BaboViolent2-" BV2_UPSTREAM_COMMIT "/BaboViolent2/Content/main";
	const double ZIP_BYTES = 17.0e6; // for the progress bar only
	// The browser fallback keeps the name auto-detect looks for in Downloads (BaboViolent2-master)
	const char * BROWSER_URL = "https://github.com/Daivuk/BaboViolent2/archive/refs/heads/master.zip";

	std::vector<std::string> downloadArgs(const std::string & zip)
	{
#ifdef BV2_PLATFORM_WINDOWS
		const char * root = std::getenv("SystemRoot");
		std::string curl = std::string(root ? root : "C:\\Windows") + "\\System32\\curl.exe";
#else
		std::string curl = "/usr/bin/curl";
#endif
		return { curl, "-fsSL", "--retry", "2", "-o", zip, ZIP_URL };
	}

	std::vector<std::string> extractArgs(const std::string & zip, const std::string & dir)
	{
#ifdef BV2_PLATFORM_WINDOWS
		const char * root = std::getenv("SystemRoot");
		std::string tar = std::string(root ? root : "C:\\Windows") + "\\System32\\tar.exe"; // bsdtar: reads .zip
		return { tar, "-xf", zip, "-C", dir, ZIP_MAIN };
#elif defined(BV2_PLATFORM_MACOS)
		return { "/usr/bin/tar", "-xf", zip, "-C", dir, ZIP_MAIN };
#else
		return { "/usr/bin/unzip", "-q", "-o", zip, std::string(ZIP_MAIN) + "/*", "-d", dir };
#endif
	}

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

	// Runs args until it exits; with sizeOf set, the bar follows that file's size against ZIP_BYTES
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
				fraction = ec ? 0.0 : std::min(0.99, (double)bytes / ZIP_BYTES);
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

	// Downloads the pinned upstream zip, extracts main/ and installs it at originalDataDir() (ADR 0040).
	// "" on success, else why it failed; cancelled is set when the player closed the progress window.
	std::string downloadData(bool & cancelled)
	{
		cancelled = false;
		const fs::path partial = bv2::originalDataDir() + ".partial";
		std::error_code ec;
		fs::remove_all(partial, ec);
		fs::create_directories(partial, ec);
		const std::string zip = (partial / "bv2.zip").string();
		std::string error;
		Step step;
		{
			Progress ui;
			step = runStep(ui, downloadArgs(zip), "Downloading BaboViolent 2 data", zip, error);
			const uintmax_t got = fs::file_size(fs::path(zip), ec);
			if (step == STEP_OK && (ec || got == 0)) // Windows reports no exit code (background process)
			{
				error = "nothing was downloaded from github.com";
				step = STEP_FAILED;
			}
			if (step == STEP_OK)
				step = runStep(ui, extractArgs(zip, partial.string()), "Unpacking BaboViolent 2 data", "", error);
			if (step == STEP_OK)
			{
				ui.show("Checking BaboViolent 2 data", 1.0);
				std::string bad = bv2::installOriginalData((partial / ZIP_MAIN).parent_path().string(), bv2::originalMainHashes());
				if (!bad.empty())
				{
					error = "check failed: " + bad;
					step = STEP_FAILED;
				}
			}
		}
		fs::remove_all(partial, ec);
		cancelled = step == STEP_CANCELLED;
		return step == STEP_OK ? "" : error;
	}

	enum Button { PLACEHOLDERS, DOWNLOAD, BROWSER };

	// The folder the data was installed in; "" when the player picks placeholders
	std::string askDataDir()
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
			"\"Download and play\" downloads it (about 17 MB) from github.com/Daivuk/BaboViolent2.";
		const SDL_MessageBoxData box = { SDL_MESSAGEBOX_INFORMATION, 0, TITLE, text, 2, buttons, 0 };

		const SDL_MessageBoxButtonData retryButtons[3] = {
			{ SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, DOWNLOAD, "Try again" },
			{ 0, BROWSER, "Open in browser" },
			{ SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, PLACEHOLDERS, "Use placeholders" },
		};
		bool ask = true;
		for (;;)
		{
			int button = PLACEHOLDERS;
			if (ask && (!SDL_ShowMessageBox(&box, &button) || button != DOWNLOAD)) // closing the box (-1) counts as placeholders
				return "";
			bool cancelled = false;
			std::string error = downloadData(cancelled);
			if (error.empty())
				return bv2::originalDataDir();
			ask = cancelled;
			if (cancelled)
				continue;
			const std::string failed = "Downloading the BaboViolent 2 data failed:\n" + error + "\n\n"
				"Try again, or download it in your browser, unzip it into Downloads and restart Robo Violence 2.";
			const SDL_MessageBoxData retry = { SDL_MESSAGEBOX_WARNING, 0, TITLE, failed.c_str(), 3, retryButtons, 0 };
			if (!SDL_ShowMessageBox(&retry, &button) || button == PLACEHOLDERS)
				return "";
			if (button == BROWSER)
			{
				SDL_OpenURL(BROWSER_URL);
				return "";
			}
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
