/*
	Copyright 2012 bitHeads inc.

	This file is part of the BaboViolent 2 source code.

	The BaboViolent 2 source code is free software: you can redistribute it and/or 
	modify it under the terms of the GNU General Public License as published by the 
	Free Software Foundation, either version 3 of the License, or (at your option) 
	any later version.

	The BaboViolent 2 source code is distributed in the hope that it will be useful, 
	but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
	FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License along with the 
	BaboViolent 2 source code. If not, see http://www.gnu.org/licenses/.
*/

#include "dks.h"

#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>

// OGG needs stb_vorbis (vcpkg port "stb"); miniaudio decodes it when the implementation is compiled in.
#ifdef BV2_HAVE_STB_VORBIS
	#define STB_VORBIS_HEADER_ONLY
	#include "stb_vorbis.c"
#endif

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#ifdef BV2_HAVE_STB_VORBIS
	#undef STB_VORBIS_HEADER_ONLY
	#include "stb_vorbis.c"
#endif

struct DksSound
{
	std::string path;
	bool loop;
	bool loaded;
	ma_sound master; // decoded data; never played, voices are copies that share it
};

namespace
{
	struct Voice
	{
		ma_sound sound;
		DksSound * owner;
		unsigned int serial;
		bool used;
	};

	ma_engine g_engine;
	ma_sound_group g_sfxGroup;
	bool g_ready = false;
	std::vector<Voice *> g_voices;
	std::vector<DksSound *> g_sounds;
	unsigned int g_serial = 1;

	ma_sound g_music;
	bool g_musicPlaying = false;

	int makeHandle(int slot, unsigned int serial)
	{
		return (int)((serial & 0x7FFFFFu) << 8) | slot;
	}

	void releaseVoice(Voice * v)
	{
		if (v->used)
		{
			ma_sound_uninit(&v->sound);
			v->used = false;
			v->owner = 0;
		}
	}

	// A free slot, or one whose sound ended; -1 when all are busy.
	int findVoice()
	{
		for (size_t i = 0; i < g_voices.size(); ++i)
		{
			Voice * v = g_voices[i];
			if (v->used && ma_sound_at_end(&v->sound))
				releaseVoice(v);
			if (!v->used)
				return (int)i;
		}
		return -1;
	}

	Voice * startVoice(DksSound * sound, int channel, int volume, bool spatial, int & handle)
	{
		handle = -1;
		if (!g_ready || !sound || !sound->loaded)
			return 0;
		int slot = (channel >= 0 && channel < (int)g_voices.size()) ? channel : findVoice();
		if (slot < 0)
			return 0;
		Voice * v = g_voices[slot];
		releaseVoice(v);
		ma_uint32 flags = MA_SOUND_FLAG_DECODE;
		if (ma_sound_init_copy(&g_engine, &sound->master, flags, &g_sfxGroup, &v->sound) != MA_SUCCESS)
			return 0;
		v->used = true;
		v->owner = sound;
		v->serial = g_serial++;
		ma_sound_set_looping(&v->sound, sound->loop ? MA_TRUE : MA_FALSE);
		ma_sound_set_volume(&v->sound, volume / 255.0f);
		ma_sound_set_spatialization_enabled(&v->sound, spatial ? MA_TRUE : MA_FALSE);
		handle = makeHandle(slot, v->serial);
		return v;
	}
}

bool			dksInit(int mixrate, int maxsoftwarechannels)
{
	(void)mixrate; // miniaudio mixes at the device's native rate
	ma_engine_config config = ma_engine_config_init();
	if (ma_engine_init(&config, &g_engine) != MA_SUCCESS)
	{
		fprintf(stderr, "dks: no audio device, running silent\n");
		return true; // the game works without sound
	}
	ma_sound_group_init(&g_engine, 0, 0, &g_sfxGroup);
	int count = maxsoftwarechannels < 16 ? 16 : (maxsoftwarechannels > 256 ? 256 : maxsoftwarechannels);
	for (int i = 0; i < count; ++i)
	{
		Voice * v = new Voice();
		v->used = false;
		v->owner = 0;
		v->serial = 0;
		g_voices.push_back(v);
	}
	g_ready = true;
	return true;
}

void			dksShutDown()
{
	if (!g_ready)
		return;
	dksStopMusic();
	for (size_t i = 0; i < g_voices.size(); ++i)
	{
		releaseVoice(g_voices[i]);
		delete g_voices[i];
	}
	g_voices.clear();
	ma_sound_group_uninit(&g_sfxGroup);
	ma_engine_uninit(&g_engine);
	g_ready = false;
}

DksSound *		dksCreateSoundFromFile(char* filename, bool loop)
{
	if (!g_ready || !filename)
		return 0;
	DksSound * s = new DksSound();
	s->path = filename;
	s->loop = loop;
	s->loaded = false;
	if (ma_sound_init_from_file(&g_engine, filename, MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_NO_SPATIALIZATION, 0, 0, &s->master) == MA_SUCCESS)
		s->loaded = true;
	else
		fprintf(stderr, "dks: cannot load %s\n", filename);
	g_sounds.push_back(s);
	return s;
}

void			dksDeleteSound(DksSound * sound)
{
	if (!sound)
		return;
	// Only pointers this module handed out (and not yet deleted) are accepted
	size_t found = g_sounds.size();
	for (size_t i = 0; i < g_sounds.size(); ++i)
		if (g_sounds[i] == sound)
			found = i;
	if (found == g_sounds.size())
		return;
	for (size_t i = 0; i < g_voices.size(); ++i)
		if (g_voices[i]->used && g_voices[i]->owner == sound)
			releaseVoice(g_voices[i]);
	g_sounds.erase(g_sounds.begin() + found);
	if (sound->loaded)
		ma_sound_uninit(&sound->master);
	delete sound;
}

int				dksPlaySound(DksSound * sound, int channel, int volume)
{
	int handle;
	Voice * v = startVoice(sound, channel, volume, false, handle);
	if (v)
		ma_sound_start(&v->sound);
	return handle;
}

int				dksPlay3DSoundPitch(DksSound * sound, float range, const CVector3f & position, int volume, float pitch)
{
	int handle;
	Voice * v = startVoice(sound, -1, volume, true, handle);
	if (!v)
		return -1;
	ma_sound_set_min_distance(&v->sound, range);
	ma_sound_set_max_distance(&v->sound, 10000000.0f);
	ma_sound_set_position(&v->sound, position.s[0], position.s[1], position.s[2]);
	ma_sound_set_pitch(&v->sound, pitch);
	ma_sound_start(&v->sound);
	return handle;
}

void			dksPlay3DSound(DksSound * sound, int channel, float range, const CVector3f & position, int volume)
{
	(void)channel;
	dksPlay3DSoundPitch(sound, range, position, volume, 1.0f);
}

void			dksStopSound(int channel)
{
	if (channel < 0 || !g_ready)
		return;
	size_t slot = (size_t)(channel & 0xFF);
	unsigned int serial = ((unsigned int)channel >> 8) & 0x7FFFFFu;
	if (slot < g_voices.size() && g_voices[slot]->used && (g_voices[slot]->serial & 0x7FFFFFu) == serial)
		releaseVoice(g_voices[slot]);
}

void			dksPlayMusic(char* filename, int channel, int volume)
{
	(void)channel;
	dksStopMusic();
	if (!g_ready || !filename)
		return;
	if (ma_sound_init_from_file(&g_engine, filename, MA_SOUND_FLAG_STREAM | MA_SOUND_FLAG_NO_SPATIALIZATION, 0, 0, &g_music) != MA_SUCCESS)
	{
		fprintf(stderr, "dks: cannot play music %s\n", filename);
		return;
	}
	g_musicPlaying = true;
	ma_sound_set_looping(&g_music, MA_TRUE);
	ma_sound_set_volume(&g_music, volume / 255.0f);
	ma_sound_start(&g_music);
}

void			dksStopMusic()
{
	if (g_musicPlaying)
	{
		ma_sound_uninit(&g_music);
		g_musicPlaying = false;
	}
}

void			dksSet3DListenerAttributes(const CVector3f * pos, const CVector3f * vel, const CVector3f * forward, const CVector3f * up)
{
	if (!g_ready)
		return;
	if (pos)
		ma_engine_listener_set_position(&g_engine, 0, pos->s[0], pos->s[1], pos->s[2]);
	if (vel)
		ma_engine_listener_set_velocity(&g_engine, 0, vel->s[0], vel->s[1], vel->s[2]);
	if (forward)
		ma_engine_listener_set_direction(&g_engine, 0, forward->s[0], forward->s[1], forward->s[2]);
	if (up)
		ma_engine_listener_set_world_up(&g_engine, 0, up->s[0], up->s[1], up->s[2]);
}

void			dksSetSfxMasterVolume(float volume)
{
	if (g_ready)
		ma_sound_group_set_volume(&g_sfxGroup, volume < 0.0f ? 0.0f : volume);
}

void			dksUpdate()
{
	if (!g_ready)
		return;
	for (size_t i = 0; i < g_voices.size(); ++i)
		if (g_voices[i]->used && ma_sound_at_end(&g_voices[i]->sound))
			releaseVoice(g_voices[i]);
}
