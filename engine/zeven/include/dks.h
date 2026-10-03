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

/// \file dks.h
/// Sound effects and music on miniaudio (WASAPI, Core Audio, PipeWire/PulseAudio/ALSA).
/// Effects are decoded WAV files; music streams (WAV, MP3, and OGG when built with stb_vorbis).
/// Volumes are 0..255 as before. A channel is a handle returned by the play functions; -1 means "no sound".

#ifndef DKS_H
#define DKS_H

#include "CVector.h"

/// An effect loaded by dksCreateSoundFromFile.
struct DksSound;

bool			dksInit(int mixrate, int maxsoftwarechannels);
void			dksShutDown();

/// Loads an effect (decoded in memory). Returns 0 if the file is missing or unreadable; every other call accepts 0.
DksSound *		dksCreateSoundFromFile(char* filename, bool loop=false);
void			dksDeleteSound(DksSound * sound);

/// Plays a flat (2D) sound. channel -1 picks a free one. Returns the channel handle.
int				dksPlaySound(DksSound * sound, int channel=-1, int volume=255);

/// Plays a sound at a position; it is at full volume within `range` and fades beyond it.
void			dksPlay3DSound(DksSound * sound, int channel, float range, const CVector3f & position, int volume=255);

/// Same, with a pitch multiplier (1 = recorded speed). Returns the channel handle.
int				dksPlay3DSoundPitch(DksSound * sound, float range, const CVector3f & position, int volume, float pitch);

/// Stops a channel returned by the play functions (no effect if it already ended or was reused).
void			dksStopSound(int channel);

void			dksPlayMusic(char* filename, int channel=-1, int volume=255);
void			dksStopMusic();

void			dksSet3DListenerAttributes(const CVector3f * pos, const CVector3f * vel, const CVector3f * forward, const CVector3f * up);

/// Master volume of the effects, 0..1 (music is not affected).
void			dksSetSfxMasterVolume(float volume);

/// Call once per frame: frees finished voices.
void			dksUpdate();

#endif
