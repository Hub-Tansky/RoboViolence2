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

#include "dkc.h"

#include <chrono>
#include <thread>

namespace
{
	typedef std::chrono::steady_clock Clock;

	// The fixed step is 1/framePerSeconde. A long stall (a breakpoint, a window drag, a slow load)
	// would otherwise make the game run hundreds of catch-up updates in one go; cap it.
	const int MAX_CATCH_UP_STEPS = 5;

	INT4 g_frame = 0; // The frame we are at
	bool g_started = false; // lastTick is valid
	Clock::time_point g_lastTick; // Time at the last dkcUpdateTimer
	float g_perSecond = 1.0f / 25.0f; // Duration of one frame
	int g_framePerSecond = 25;
	float g_currentFrameDelay = 0; // Time accumulated towards the next frame
	float g_fps = 0;
	int g_oneSecondFrameCount = 0;
	float g_oneSecondElapsed = 0;
}

//
// The elapsed time in seconds of one fixed frame
//
float			dkcGetElapsedf()
{
	return g_perSecond;
}

//
// Obtenir le frame per second
//
float			dkcGetFPS()
{
	return g_fps;
}

//
// Pour obtenir le nb de frame ou on est rendu
//
INT4			dkcGetFrame()
{
	return g_frame;
}

//
// Init the timer (do at your program start)
//
void			dkcInit(int framePerSecond)
{
	g_framePerSecond = framePerSecond;
	g_perSecond = 1.0f / (float)g_framePerSecond;
}

//
// To step a couple of frame or to init it to 0
//
void			dkcJumpToFrame(int frame)
{
	g_frame = frame;
	g_currentFrameDelay = 0;
	g_started = false;
	g_fps = 0;
	g_oneSecondFrameCount = 0;
	g_oneSecondElapsed = 0;
}

//
// Returns how many fixed frames to animate now
//
INT4			dkcUpdateTimer()
{
	Clock::time_point now = Clock::now();
	float elapsed = 0;
	if (g_started)
		elapsed = std::chrono::duration<float>(now - g_lastTick).count();
	g_lastTick = now;
	g_started = true;

	g_currentFrameDelay += elapsed;
	INT4 nbFrameAdded = 0;
	while (g_currentFrameDelay >= g_perSecond)
	{
		g_currentFrameDelay -= g_perSecond;
		g_frame++;
		nbFrameAdded++;
	}
	if (nbFrameAdded > MAX_CATCH_UP_STEPS)
	{
		nbFrameAdded = MAX_CATCH_UP_STEPS;
		g_currentFrameDelay = 0; // drop the backlog instead of replaying it
	}

	// On update pour le fps
	g_oneSecondElapsed += elapsed;
	g_oneSecondFrameCount++;
	while (g_oneSecondElapsed >= 1)
	{
		g_oneSecondElapsed -= 1;
		g_fps = (float)g_oneSecondFrameCount;
		g_oneSecondFrameCount = 0;
	}

	return nbFrameAdded;
}

void			dkcSleep(INT4 ms)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
