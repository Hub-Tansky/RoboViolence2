// R20 (Phase B step 5): playing a missing music file many times doesn't race miniaudio's stream job thread.
// ASan builds report the old heap-use-after-free; miniaudio falls back to its null device without audio hardware.
#include "dks.h"

#include <chrono>
#include <cstdio>
#include <thread>

int main()
{
	dksInit(44100, 32);
	for (int i = 0; i < 200; ++i)
	{
		dksPlayMusic((char *)"no-such-dir/Menu.ogg", -1, 200);
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	dksStopMusic();
	dksShutDown();
	std::printf("ok: missing music\n");
	return 0;
}
