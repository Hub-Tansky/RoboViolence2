// Phase B step 5, ASan: playing a missing music file many times doesn't race miniaudio's stream job thread (R20), and
// deleting a sound after dksShutDown doesn't touch the freed engine (teardown). Without audio hardware miniaudio
// uses its null device.
#include "dks.h"

#include <chrono>
#include <cstdint>
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

	// A 0.1 s silent mono 16-bit WAV, so the sound really loads
	FILE * f = std::fopen("dks-test.wav", "wb");
	const uint32_t rate = 8000, samples = 800, dataSize = samples * 2, riffSize = 36 + dataSize, fmtSize = 16, byteRate = rate * 2;
	const uint16_t pcm = 1, channels = 1, align = 2, bits = 16;
	std::fwrite("RIFF", 1, 4, f); std::fwrite(&riffSize, 4, 1, f); std::fwrite("WAVEfmt ", 1, 8, f);
	std::fwrite(&fmtSize, 4, 1, f); std::fwrite(&pcm, 2, 1, f); std::fwrite(&channels, 2, 1, f); std::fwrite(&rate, 4, 1, f);
	std::fwrite(&byteRate, 4, 1, f); std::fwrite(&align, 2, 1, f); std::fwrite(&bits, 2, 1, f);
	std::fwrite("data", 1, 4, f); std::fwrite(&dataSize, 4, 1, f);
	for (uint32_t i = 0; i < samples; ++i) { int16_t zero = 0; std::fwrite(&zero, 2, 1, f); }
	std::fclose(f);
	DksSound * sound = dksCreateSoundFromFile((char *)"dks-test.wav", false);
	dksShutDown();
	dksDeleteSound(sound); // what the global menus do at exit
	std::remove("dks-test.wav");
	std::printf("ok: missing music, delete after shutdown\n");
	return 0;
}
