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

/// \file dkw.h
/// Window, OpenGL context host and event pump, on SDL3 (Windows, macOS and Linux). Replaces the Win32/WGL window
/// and the SDL 1.2 window. The main loop calls CMainLoopInterface::paint() once per iteration; the game runs its
/// fixed 30 Hz update from there through dkc.

#ifndef DKW_H
#define DKW_H

#include "platform.h"
#include "CVector.h"

/// Implemented by the application: paint() runs one cycle (updates plus a render), textWrite() receives
/// typed characters as Latin-1 codes (the bitmap font renders by byte value) plus 8, 9, 13 for backspace, tab, enter.
class CMainLoopInterface
{
public:
	virtual void paint() = 0;
	virtual void textWrite(unsigned int caracter) = 0;
};

/// Creates the window. width and height are in pixels (the drawable size, also on HiDPI displays).
/// colorDepth 16 or 32; refreshRate only matters for exclusive fullscreen (-1 = desktop default).
/// Returns 1 on success, 0 on failure (see dkwGetLastError).
int				dkwInit(int width, int height, int colorDepth, char *title, CMainLoopInterface *mMainLoopObject, bool fullScreen, int refreshRate = -1);

/// Asks the main loop to end.
void			dkwForceQuit();

/// The SDL_Window* (as void* to keep SDL out of the public headers).
void *			dkwGetWindow();

char*			dkwGetLastError();

/// Mouse position in drawable pixels, from the window's top-left corner.
CVector2i		dkwGetCursorPos();

/// Drawable size in pixels. Use it for glViewport and all screen-space math.
CVector2i		dkwGetResolution();

/// Runs until the window closes: pumps events, then calls paint(). Always returns 0.
int				dkwMainLoop();

void			dkwShutDown();

/// Pumps pending events once (call during long loads to keep the window responsive).
void			dkwUpdate();

/// Captures the mouse (relative mode: the pointer is hidden and cannot leave the window) or releases it.
void			dkwClipMouse( bool abEnabled = true );

/// Shows a modal error box (replaces MessageBox).
void			dkwShowMessage(const char* title, const char* text);

/// Accumulated mouse movement since the last call, wheel in 120 per notch (the convention the game was written for), buttons 0x80 when down.
struct DkwMouseState
{
	int lX;
	int lY;
	int lZ;
	unsigned char rgbButtons[8];
};
void			dkwGetMouseState(DkwMouseState * aMouseState);

/// Key state indexed by DIK_* (dikeys.h): 0x80 when down. aSize is at most 256.
void			dkwGetKeysState(unsigned char * aState, int aSize);

#endif
