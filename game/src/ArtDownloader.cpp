// Client: the art pack's background download and the first-run splash (ROBO-260, ADR ROBO-260)
#include "ArtDownloader.h"
#include "ArtPack.h"
#include "AssetRegistry.h"
#include "Helper.h"
#include "Paths.h"
#include "Zeven.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

namespace
{
	enum State
	{
		IDLE,
		RUNNING,
		DONE,
		FAILED
	};
	std::atomic<int> g_state(IDLE);
	// Packs the build fetched (RV2_ASSETS, ROBO-261): used unless --placeholders; g_buildArt when its art pack is stacked.
	// Main thread only (stackAssetPacks); the download thread touches only g_state and g_stop.
	bool g_useBuildPacks = true;
	bool g_buildArt = false;
	std::atomic<bool> g_stop(false);
	std::string g_archive; // the main thread's copy, for the progress line; the download thread has its own

	// Runs args to the end; false when it could not start, exited non-zero or was stopped
	bool run(const std::vector<std::string>& args)
	{
		std::vector<const char*> argv;
		for (const std::string& a : args)
			argv.push_back(a.c_str());
		argv.push_back(0);
		SDL_PropertiesID props = SDL_CreateProperties();
		SDL_SetPointerProperty(props, SDL_PROP_PROCESS_CREATE_ARGS_POINTER, (void*)argv.data());
#ifdef BV2_PLATFORM_WINDOWS
		SDL_SetBooleanProperty(props, SDL_PROP_PROCESS_CREATE_BACKGROUND_BOOLEAN, true); // no console window
#endif
		SDL_Process* process = SDL_CreateProcessWithProperties(props);
		SDL_DestroyProperties(props);
		if (!process)
			return false;
		int exitCode = 0;
		while (!SDL_WaitProcess(process, false, &exitCode))
		{
			if (g_stop)
			{
				SDL_KillProcess(process, true);
				SDL_WaitProcess(process, true, &exitCode);
				SDL_DestroyProcess(process);
				return false;
			}
			SDL_Delay(50);
		}
		SDL_DestroyProcess(process);
#ifdef BV2_PLATFORM_WINDOWS
		return true; // background processes report no exit code there: the hash and pack.tsv checks are the verdict
#else
		return exitCode == 0;
#endif
	}

	// Runs on the download thread with its own copies of the paths (main may return before it ends)
	void download(std::string partial, std::string target)
	{
		const std::string error = bv2::fetchArtPack(partial, bv2::ART_PACK_URL, bv2::ART_PACK_SHA256, bv2::ART_PACK_VERSION,
													target, run, []
													{ return g_stop.load(); });
		if (!error.empty())
			std::fprintf(stderr, "art pack %s: %s; playing without it, retrying on the next start\n", bv2::ART_PACK_VERSION,
						 error.c_str());
		g_state = error.empty() ? DONE : FAILED;
	}

	double progress()
	{
		std::error_code ec;
		uintmax_t bytes = fs::file_size(fs::path(g_archive), ec);
		return ec ? 0.0 : std::min(0.99, (double)bytes / bv2::ART_PACK_BYTES);
	}

	// One splash frame: the splash texture over the window, a thin progress line at the bottom
	void splashFrame(unsigned int splash, double fraction)
	{
		glClearColor(0, 0, 0, 1);
		glClear((GLbitfield)GL_COLOR_BUFFER_BIT | (GLbitfield)GL_DEPTH_BUFFER_BIT);
		CVector2i res = dkwGetResolution();
		glViewport(0, 0, res[0], res[1]);
		dkglPushOrtho(800, 600);
		glPushAttrib((GLbitfield)GL_ENABLE_BIT | (GLbitfield)GL_CURRENT_BIT);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glColor3f(1, 1, 1);
		if (splash)
			renderTexturedQuadSmooth(0, 0, 800, 600, splash);
		glDisable(GL_TEXTURE_2D);
		glColor3f(0.9f, 0.63f, 0.16f);
		glBegin(GL_QUADS);
		glVertex2f(0, 596);
		glVertex2f(0, 600);
		glVertex2f(800 * (float)fraction, 600);
		glVertex2f(800 * (float)fraction, 596);
		glEnd();
		glPopAttrib();
		dkglPopOrtho();
		dkglSwapBuffers();
	}
}

namespace bv2
{
	void stackAssetPacks()
	{
		const PackStack stack = assetPackStack(buildPacksDir(packageRoot()), g_useBuildPacks, artPackDir(), ART_PACK_VERSION,
											   dataRoot(), packageRoot());
		g_buildArt = stack.buildArt;
		setAssetPacks(stack.roots, stack.cwdPack); // ADR 0069: art pack over the player's BV2 data over the base pack
	}

	void stackAssetPacks(int argc, char** argv)
	{
		for (int i = 1; i < argc; ++i)
			if (std::strcmp(argv[i], "--placeholders") == 0)
				g_useBuildPacks = false; // tests and screenshots: only what the pref dir holds, as before ROBO-261
		stackAssetPacks();
	}

	void artDownloadStart(int argc, char** argv)
	{
		if (*ART_PACK_VERSION == 0 || g_buildArt || artPackInstalled(artPackDir(), ART_PACK_VERSION) ||
			artDownloadOff(argc, (const char* const*)argv, prefDir()))
			return;
		g_archive = artArchivePath(artPackDir() + ".partial");
		g_state = RUNNING;
		std::thread(download, artPackDir() + ".partial", artPackDir()).detach(); // artDownloadStop ends it
	}

	void artSplashWait(double timeoutSeconds)
	{
		if (g_state == DONE) // a fast download can finish before the window exists
			stackAssetPacks();
		if (g_state != RUNNING)
			return;
		unsigned int splash = dktCreateTextureFromFile(assetPath("textures/splash"), DKT_FILTER_LINEAR);
		const uint64_t end = SDL_GetTicks() + (uint64_t)(timeoutSeconds * 1000);
		bool skip = false;
		while (g_state == RUNNING && SDL_GetTicks() < end && !skip)
		{
			SDL_Event e;
			while (SDL_PollEvent(&e))
				if (e.type == SDL_EVENT_QUIT || e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
				{
					SDL_PushEvent(&e); // the main loop still has to see it and quit
					skip = true;
					break;
				}
				else if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE)
					skip = true;
			splashFrame(splash, progress());
			SDL_Delay(33);
		}
		if (splash)
			dktDeleteTexture(&splash);
		if (g_state == DONE)
			stackAssetPacks(); // before the game loads its assets: not a hot reload (ADR 0042)
	}

	void artDownloadStop()
	{
		g_stop = true;
		for (int i = 0; i < 100 && g_state == RUNNING; ++i) // curl or tar is killed within one 50 ms poll
			SDL_Delay(50);
	}
}
