// Client: downloads the Robo Violence 2 art pack in the background and stacks it (ROBO-260, ADR ROBO-260)
#pragma once

namespace bv2
{
	// Stacks the asset packs (assetPackStack): the build's packs (ROBO-261), artPackDir() when it holds this build's art
	// pack, then the data root, then the base pack
	void stackAssetPacks();
	// The first stack at start: also notes --placeholders, which leaves out the build's packs for the whole run
	void stackAssetPacks(int argc, char** argv);

	// Starts the background download when this build pins an art pack, neither the build nor artPackDir() holds it, and the player didn't turn
	// the download off. Call after pathsInit().
	void artDownloadStart(int argc, char** argv);

	// While the download runs: shows the free splash with a progress line until it ends, timeoutSeconds pass or the
	// player presses Esc, then stacks the art pack if it arrived. Call after the GL context and dktInit, before the
	// game loads its assets. The download continues in the background after a timeout or Esc.
	void artSplashWait(double timeoutSeconds);

	// Stops a running download (kills curl or tar) and waits up to 5 s for it to end; at shutdown
	void artDownloadStop();
}
