// Phase B step 5, ASan: playing a missing music file many times doesn't race miniaudio's stream job thread (R20), and
// deleting a sound after dksShutDown doesn't touch the freed engine (teardown). ROBO-216: with every voice busy with
// 3D sounds (a firefight), a 2D event sound such as the CTF score cheer still plays. Without audio hardware
// miniaudio uses its null device.
#include "dks.h"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <thread>

static void writeSilentWav(const char* path, uint32_t samples)
{
	FILE* f = std::fopen(path, "wb");
	const uint32_t rate = 8000, dataSize = samples * 2, riffSize = 36 + dataSize, fmtSize = 16, byteRate = rate * 2;
	const uint16_t pcm = 1, channels = 1, align = 2, bits = 16;
	std::fwrite("RIFF", 1, 4, f);
	std::fwrite(&riffSize, 4, 1, f);
	std::fwrite("WAVEfmt ", 1, 8, f);
	std::fwrite(&fmtSize, 4, 1, f);
	std::fwrite(&pcm, 2, 1, f);
	std::fwrite(&channels, 2, 1, f);
	std::fwrite(&rate, 4, 1, f);
	std::fwrite(&byteRate, 4, 1, f);
	std::fwrite(&align, 2, 1, f);
	std::fwrite(&bits, 2, 1, f);
	std::fwrite("data", 1, 4, f);
	std::fwrite(&dataSize, 4, 1, f);
	for (uint32_t i = 0; i < samples; ++i)
	{
		int16_t zero = 0;
		std::fwrite(&zero, 2, 1, f);
	}
	std::fclose(f);
}

// ROBO-216: 16 voices (the s_maxSoftwareChannels default) all held by long 3D sounds, then a 2D cheer
static int voiceStarvation()
{
	dksInit(44100, 16);
	writeSilentWav("dks-long.wav", 8000 * 30); // 30 s, so nothing ends while the test runs
	DksSound* shot = dksCreateSoundFromFile((char*)"dks-long.wav", false);
	DksSound* cheer = dksCreateSoundFromFile((char*)"dks-long.wav", false);
	DksSound* weather = dksCreateSoundFromFile((char*)"dks-long.wav", true);
	if (!shot || !cheer || !weather)
	{
		std::printf("FAIL: the long WAV didn't load\n");
		return 1;
	}
	int fail = 0;
	int rain = dksPlaySound(weather, -1, 50); // a looping 2D voice, never stolen
	CVector3f pos(1, 2, 0);
	int firstShot = -1;
	for (int i = 0; i < 15; ++i)
	{
		int h = dksPlay3DSoundPitch(shot, 5, pos, 255, 1.0f);
		if (i == 0)
			firstShot = h;
	}
	if (dksPlay3DSoundPitch(shot, 5, pos, 255, 1.0f) < 0)
	{
		std::printf("FAIL: a 3D sound with every voice busy was dropped, it should take the oldest 3D voice\n");
		fail = 1;
	}
	int cheerHandle = dksPlaySound(cheer, -1, 255);
	if (cheerHandle < 0)
	{
		std::printf("FAIL: the 2D score cheer was dropped with every voice busy with 3D sounds\n");
		fail = 1;
	}
	if (firstShot >= 0 && cheerHandle >= 0 && (firstShot & 0xFF) == (cheerHandle & 0xFF))
	{
		std::printf("FAIL: the oldest 3D voice went to the second 3D sound, not the cheer: wrong order\n");
		fail = 1;
	}
	// Fill the rest with 2D sounds; 3D sounds may not steal them, the looping one is never stolen
	for (int i = 0; i < 15; ++i)
		dksPlaySound(cheer, -1, 255);
	if (dksPlay3DSoundPitch(shot, 5, pos, 255, 1.0f) >= 0)
	{
		std::printf("FAIL: a 3D sound took a 2D voice\n");
		fail = 1;
	}
	dksStopSound(rain);
	if (dksPlaySound(weather, -1, 50) < 0)
	{
		std::printf("FAIL: no voice after stopping one\n");
		fail = 1;
	}
	dksShutDown();
	std::remove("dks-long.wav");
	return fail;
}

int main()
{
	if (voiceStarvation())
		return 1;

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
	if (!sound)
	{
		std::printf("FAIL: no audio engine or the WAV didn't load, so nothing was tested\n");
		return 1;
	}
	dksShutDown();
	dksDeleteSound(sound); // what the global menus do at exit
	std::remove("dks-test.wav");
	std::printf("ok: voice stealing, missing music, delete after shutdown\n");
	return 0;
}
