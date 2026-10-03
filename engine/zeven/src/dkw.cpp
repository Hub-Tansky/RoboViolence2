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

#include "dkw.h"
#include "dikeys.h"

#include <SDL3/SDL.h>

#include <stdio.h>
#include <string.h>
#include <string>

namespace
{
	SDL_Window * g_window = 0;
	std::string g_lastError;
	std::string g_title;
	CMainLoopInterface * g_mainLoop = 0;
	bool g_done = false;
	bool g_hasFocus = true;
	bool g_wantCapture = false;

	int g_pixelW = 640;
	int g_pixelH = 480;
	float g_pixelScale = 1.0f; // drawable pixels per window coordinate
	CVector2i g_cursorPos;

	DkwMouseState g_mouse;
	unsigned char g_keys[256];

	void setError(const char * msg)
	{
		g_lastError = msg ? msg : "";
	}

	int scancodeToDik(SDL_Scancode sc)
	{
		// SDL scancode (USB HID usage) -> DIK key ID
		switch (sc)
		{
		case SDL_SCANCODE_ESCAPE: return DIK_ESCAPE;
		case SDL_SCANCODE_1: return DIK_1;
		case SDL_SCANCODE_2: return DIK_2;
		case SDL_SCANCODE_3: return DIK_3;
		case SDL_SCANCODE_4: return DIK_4;
		case SDL_SCANCODE_5: return DIK_5;
		case SDL_SCANCODE_6: return DIK_6;
		case SDL_SCANCODE_7: return DIK_7;
		case SDL_SCANCODE_8: return DIK_8;
		case SDL_SCANCODE_9: return DIK_9;
		case SDL_SCANCODE_0: return DIK_0;
		case SDL_SCANCODE_MINUS: return DIK_MINUS;
		case SDL_SCANCODE_EQUALS: return DIK_EQUALS;
		case SDL_SCANCODE_BACKSPACE: return DIK_BACK;
		case SDL_SCANCODE_TAB: return DIK_TAB;
		case SDL_SCANCODE_RETURN: return DIK_RETURN;
		case SDL_SCANCODE_LCTRL: return DIK_LCONTROL;
		case SDL_SCANCODE_SEMICOLON: return DIK_SEMICOLON;
		case SDL_SCANCODE_APOSTROPHE: return DIK_APOSTROPHE;
		case SDL_SCANCODE_GRAVE: return DIK_GRAVE;
		case SDL_SCANCODE_LSHIFT: return DIK_LSHIFT;
		case SDL_SCANCODE_BACKSLASH: return DIK_BACKSLASH;
		case SDL_SCANCODE_COMMA: return DIK_COMMA;
		case SDL_SCANCODE_PERIOD: return DIK_PERIOD;
		case SDL_SCANCODE_SLASH: return DIK_SLASH;
		case SDL_SCANCODE_RSHIFT: return DIK_RSHIFT;
		case SDL_SCANCODE_KP_MULTIPLY: return DIK_MULTIPLY;
		case SDL_SCANCODE_LALT: return DIK_LMENU;
		case SDL_SCANCODE_SPACE: return DIK_SPACE;
		case SDL_SCANCODE_CAPSLOCK: return DIK_CAPITAL;
		case SDL_SCANCODE_NUMLOCKCLEAR: return DIK_NUMLOCK;
		case SDL_SCANCODE_SCROLLLOCK: return DIK_SCROLL;
		case SDL_SCANCODE_KP_7: return DIK_NUMPAD7;
		case SDL_SCANCODE_KP_8: return DIK_NUMPAD8;
		case SDL_SCANCODE_KP_9: return DIK_NUMPAD9;
		case SDL_SCANCODE_KP_MINUS: return DIK_SUBTRACT;
		case SDL_SCANCODE_KP_4: return DIK_NUMPAD4;
		case SDL_SCANCODE_KP_5: return DIK_NUMPAD5;
		case SDL_SCANCODE_KP_6: return DIK_NUMPAD6;
		case SDL_SCANCODE_KP_PLUS: return DIK_ADD;
		case SDL_SCANCODE_KP_1: return DIK_NUMPAD1;
		case SDL_SCANCODE_KP_2: return DIK_NUMPAD2;
		case SDL_SCANCODE_KP_3: return DIK_NUMPAD3;
		case SDL_SCANCODE_KP_0: return DIK_NUMPAD0;
		case SDL_SCANCODE_KP_PERIOD: return DIK_DECIMAL;
		case SDL_SCANCODE_NONUSBACKSLASH: return DIK_OEM_102;
		case SDL_SCANCODE_F11: return DIK_F11;
		case SDL_SCANCODE_F12: return DIK_F12;
		case SDL_SCANCODE_F13: return DIK_F13;
		case SDL_SCANCODE_F14: return DIK_F14;
		case SDL_SCANCODE_F15: return DIK_F15;
		case SDL_SCANCODE_KP_EQUALS: return DIK_NUMPADEQUALS;
		case SDL_SCANCODE_KP_ENTER: return DIK_NUMPADENTER;
		case SDL_SCANCODE_RCTRL: return DIK_RCONTROL;
		case SDL_SCANCODE_KP_DIVIDE: return DIK_DIVIDE;
		case SDL_SCANCODE_PRINTSCREEN: return DIK_SYSRQ;
		case SDL_SCANCODE_RALT: return DIK_RMENU;
		case SDL_SCANCODE_PAUSE: return DIK_PAUSE;
		case SDL_SCANCODE_HOME: return DIK_HOME;
		case SDL_SCANCODE_UP: return DIK_UP;
		case SDL_SCANCODE_PAGEUP: return DIK_PRIOR;
		case SDL_SCANCODE_LEFT: return DIK_LEFT;
		case SDL_SCANCODE_RIGHT: return DIK_RIGHT;
		case SDL_SCANCODE_END: return DIK_END;
		case SDL_SCANCODE_DOWN: return DIK_DOWN;
		case SDL_SCANCODE_PAGEDOWN: return DIK_NEXT;
		case SDL_SCANCODE_INSERT: return DIK_INSERT;
		case SDL_SCANCODE_DELETE: return DIK_DELETE;
		case SDL_SCANCODE_LGUI: return DIK_LWIN;
		case SDL_SCANCODE_RGUI: return DIK_RWIN;
		case SDL_SCANCODE_APPLICATION: return DIK_APPS;
		case SDL_SCANCODE_POWER: return DIK_POWER;
		case SDL_SCANCODE_KP_COMMA: return DIK_NUMPADCOMMA;
		case SDL_SCANCODE_INTERNATIONAL3: return DIK_YEN;
		case SDL_SCANCODE_INTERNATIONAL4: return DIK_CONVERT;
		case SDL_SCANCODE_INTERNATIONAL5: return DIK_NOCONVERT;
		case SDL_SCANCODE_INTERNATIONAL1: return DIK_ABNT_C1;
		case SDL_SCANCODE_INTERNATIONAL2: return DIK_KANA;
		case SDL_SCANCODE_MUTE: return DIK_MUTE;
		case SDL_SCANCODE_VOLUMEDOWN: return DIK_VOLUMEDOWN;
		case SDL_SCANCODE_VOLUMEUP: return DIK_VOLUMEUP;
		case SDL_SCANCODE_STOP: return DIK_STOP;
		case SDL_SCANCODE_MEDIA_PLAY_PAUSE: return DIK_PLAYPAUSE;
		case SDL_SCANCODE_MEDIA_STOP: return DIK_MEDIASTOP;
		case SDL_SCANCODE_MEDIA_NEXT_TRACK: return DIK_NEXTTRACK;
		case SDL_SCANCODE_MEDIA_PREVIOUS_TRACK: return DIK_PREVTRACK;
		case SDL_SCANCODE_AC_HOME: return DIK_WEBHOME;
		case SDL_SCANCODE_AC_SEARCH: return DIK_WEBSEARCH;
		case SDL_SCANCODE_AC_BOOKMARKS: return DIK_WEBFAVORITES;
		case SDL_SCANCODE_AC_REFRESH: return DIK_WEBREFRESH;
		case SDL_SCANCODE_AC_STOP: return DIK_WEBSTOP;
		case SDL_SCANCODE_AC_FORWARD: return DIK_WEBFORWARD;
		case SDL_SCANCODE_AC_BACK: return DIK_WEBBACK;
		case SDL_SCANCODE_SLEEP: return DIK_SLEEP;
		case SDL_SCANCODE_MEDIA_SELECT: return DIK_MEDIASELECT;
		case SDL_SCANCODE_Q: return DIK_Q;
		case SDL_SCANCODE_W: return DIK_W;
		case SDL_SCANCODE_E: return DIK_E;
		case SDL_SCANCODE_R: return DIK_R;
		case SDL_SCANCODE_T: return DIK_T;
		case SDL_SCANCODE_Y: return DIK_Y;
		case SDL_SCANCODE_U: return DIK_U;
		case SDL_SCANCODE_I: return DIK_I;
		case SDL_SCANCODE_O: return DIK_O;
		case SDL_SCANCODE_P: return DIK_P;
		case SDL_SCANCODE_A: return DIK_A;
		case SDL_SCANCODE_S: return DIK_S;
		case SDL_SCANCODE_D: return DIK_D;
		case SDL_SCANCODE_F: return DIK_F;
		case SDL_SCANCODE_G: return DIK_G;
		case SDL_SCANCODE_H: return DIK_H;
		case SDL_SCANCODE_J: return DIK_J;
		case SDL_SCANCODE_K: return DIK_K;
		case SDL_SCANCODE_L: return DIK_L;
		case SDL_SCANCODE_Z: return DIK_Z;
		case SDL_SCANCODE_X: return DIK_X;
		case SDL_SCANCODE_C: return DIK_C;
		case SDL_SCANCODE_V: return DIK_V;
		case SDL_SCANCODE_B: return DIK_B;
		case SDL_SCANCODE_N: return DIK_N;
		case SDL_SCANCODE_M: return DIK_M;
		case SDL_SCANCODE_F1: return DIK_F1;
		case SDL_SCANCODE_F2: return DIK_F2;
		case SDL_SCANCODE_F3: return DIK_F3;
		case SDL_SCANCODE_F4: return DIK_F4;
		case SDL_SCANCODE_F5: return DIK_F5;
		case SDL_SCANCODE_F6: return DIK_F6;
		case SDL_SCANCODE_F7: return DIK_F7;
		case SDL_SCANCODE_F8: return DIK_F8;
		case SDL_SCANCODE_F9: return DIK_F9;
		case SDL_SCANCODE_F10: return DIK_F10;
		case SDL_SCANCODE_LEFTBRACKET: return DIK_LBRACKET;
		case SDL_SCANCODE_RIGHTBRACKET: return DIK_RBRACKET;
		default: return 0;
		}
	}

	void refreshPixelSize()
	{
		if (!g_window)
			return;
		int w = 0, h = 0;
		SDL_GetWindowSizeInPixels(g_window, &w, &h);
		if (w > 0 && h > 0)
		{
			g_pixelW = w;
			g_pixelH = h;
		}
		float s = SDL_GetWindowPixelDensity(g_window);
		g_pixelScale = (s > 0.0f) ? s : 1.0f;
	}

	void applyMouseCapture()
	{
		if (g_window)
			SDL_SetWindowRelativeMouseMode(g_window, g_wantCapture && g_hasFocus);
	}

	// UTF-8 codepoint to the Latin-1 byte the bitmap font renders; 0 when it has no glyph.
	unsigned int toLatin1(unsigned int cp)
	{
		return (cp >= 32 && cp <= 255 && cp != 127) ? cp : 0;
	}

	void textFromUtf8(const char * s)
	{
		const unsigned char * p = (const unsigned char *)s;
		while (p && *p)
		{
			unsigned int cp = 0;
			int extra = 0;
			if (*p < 0x80) { cp = *p; extra = 0; }
			else if ((*p & 0xE0) == 0xC0) { cp = *p & 0x1F; extra = 1; }
			else if ((*p & 0xF0) == 0xE0) { cp = *p & 0x0F; extra = 2; }
			else if ((*p & 0xF8) == 0xF0) { cp = *p & 0x07; extra = 3; }
			++p;
			for (int i = 0; i < extra && (*p & 0xC0) == 0x80; ++i, ++p)
				cp = (cp << 6) | (*p & 0x3F);
			unsigned int c = toLatin1(cp);
			if (c && g_mainLoop)
				g_mainLoop->textWrite(c);
		}
	}

	void handleEvent(const SDL_Event & e)
	{
		switch (e.type)
		{
		case SDL_EVENT_QUIT:
		case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
			g_done = true;
			break;

		case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
		case SDL_EVENT_WINDOW_RESIZED:
		case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
			refreshPixelSize();
			break;

		case SDL_EVENT_WINDOW_FOCUS_GAINED:
			g_hasFocus = true;
			applyMouseCapture();
			break;

		case SDL_EVENT_WINDOW_FOCUS_LOST:
			g_hasFocus = false;
			applyMouseCapture();
			memset(g_keys, 0, sizeof(g_keys));
			break;

		case SDL_EVENT_MOUSE_MOTION:
			g_cursorPos[0] = (int)(e.motion.x * g_pixelScale);
			g_cursorPos[1] = (int)(e.motion.y * g_pixelScale);
			g_mouse.lX += (int)(e.motion.xrel * g_pixelScale);
			g_mouse.lY += (int)(e.motion.yrel * g_pixelScale);
			break;

		case SDL_EVENT_MOUSE_BUTTON_DOWN:
		case SDL_EVENT_MOUSE_BUTTON_UP:
		{
			unsigned char v = (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) ? 0x80 : 0;
			switch (e.button.button)
			{
			case SDL_BUTTON_LEFT: g_mouse.rgbButtons[0] = v; break;
			case SDL_BUTTON_RIGHT: g_mouse.rgbButtons[1] = v; break;
			case SDL_BUTTON_MIDDLE: g_mouse.rgbButtons[2] = v; break;
			case SDL_BUTTON_X1: g_mouse.rgbButtons[3] = v; break;
			case SDL_BUTTON_X2: g_mouse.rgbButtons[4] = v; break;
			}
			break;
		}

		case SDL_EVENT_MOUSE_WHEEL:
			g_mouse.lZ += (int)(e.wheel.y * 120.0f);
			break;

		case SDL_EVENT_KEY_DOWN:
		case SDL_EVENT_KEY_UP:
		{
			int dik = scancodeToDik(e.key.scancode);
			bool down = (e.type == SDL_EVENT_KEY_DOWN);
			if (dik)
				g_keys[dik] = down ? 0x80 : 0;
			// Control characters arrive as text on Windows (WM_CHAR); SDL reports them as keys.
			if (down && g_mainLoop)
			{
				switch (e.key.scancode)
				{
				case SDL_SCANCODE_BACKSPACE: g_mainLoop->textWrite(8); break;
				case SDL_SCANCODE_TAB: g_mainLoop->textWrite(9); break;
				case SDL_SCANCODE_RETURN:
				case SDL_SCANCODE_KP_ENTER: g_mainLoop->textWrite(13); break;
				default: break;
				}
			}
			break;
		}

		case SDL_EVENT_TEXT_INPUT:
			textFromUtf8(e.text.text);
			break;

		default:
			break;
		}
	}

	void pumpEvents()
	{
		SDL_Event e;
		while (SDL_PollEvent(&e))
			handleEvent(e);
	}
}

int dkwInit(int width, int height, int colorDepth, char* mTitle, CMainLoopInterface *mMainLoopObject, bool fullScreen, int refreshRate)
{
	g_mainLoop = mMainLoopObject;
	g_pixelW = width;
	g_pixelH = height;
	g_title = mTitle ? mTitle : "";
	memset(g_keys, 0, sizeof(g_keys));
	memset(&g_mouse, 0, sizeof(g_mouse));

	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		setError(SDL_GetError());
		return 0;
	}

	// OpenGL 2.1 compatibility: the renderer is fixed-function (ADR 0003).
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, colorDepth > 16 ? 8 : 5);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, colorDepth > 16 ? 8 : 6);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, colorDepth > 16 ? 8 : 5);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, colorDepth > 16 ? 8 : 0);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	// Windows are sized in screen coordinates; the game works in pixels. Create at the requested size,
	// then correct it once the display's pixel density is known.
	SDL_WindowFlags flags = SDL_WINDOW_OPENGL | SDL_WINDOW_HIGH_PIXEL_DENSITY;
	g_window = SDL_CreateWindow(g_title.c_str(), width, height, flags);
	if (!g_window)
	{
		setError(SDL_GetError());
		return 0;
	}

	if (fullScreen)
	{
		SDL_DisplayID display = SDL_GetDisplayForWindow(g_window);
		SDL_DisplayMode mode;
		if (SDL_GetClosestFullscreenDisplayMode(display, width, height, refreshRate > 0 ? (float)refreshRate : 0.0f, true, &mode))
			SDL_SetWindowFullscreenMode(g_window, &mode);
		SDL_SetWindowFullscreen(g_window, true);
	}
	else
	{
		float density = SDL_GetWindowPixelDensity(g_window);
		if (density > 1.0f)
			SDL_SetWindowSize(g_window, (int)(width / density + 0.5f), (int)(height / density + 0.5f));
		SDL_SetWindowPosition(g_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	}

	SDL_HideCursor();
	SDL_StartTextInput(g_window);
	SDL_SyncWindow(g_window);
	refreshPixelSize();
	g_done = false;
	return 1;
}

void dkwForceQuit()
{
	g_done = true;
	dkwClipMouse(false);
}

void * dkwGetWindow()
{
	return g_window;
}

char* dkwGetLastError()
{
	return (char*)g_lastError.c_str();
}

CVector2i dkwGetCursorPos()
{
	return g_cursorPos;
}

CVector2i dkwGetResolution()
{
	return CVector2i(g_pixelW, g_pixelH);
}

void dkwClipMouse(bool abEnabled)
{
	g_wantCapture = abEnabled;
	applyMouseCapture();
}

void dkwShowMessage(const char* title, const char* text)
{
	if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title, text, g_window))
		fprintf(stderr, "%s: %s\n", title, text);
}

int dkwMainLoop()
{
	while (!g_done)
	{
		pumpEvents();
		if (g_done)
			break;
		if (g_mainLoop)
			g_mainLoop->paint();
	}
	return 0;
}

void dkwUpdate()
{
	pumpEvents();
}

void dkwGetMouseState(DkwMouseState * aMouseState)
{
	if (!aMouseState)
		return;
	*aMouseState = g_mouse;
	// Movement and wheel are deltas since the last read; buttons are levels.
	g_mouse.lX = 0;
	g_mouse.lY = 0;
	g_mouse.lZ = 0;
}

void dkwGetKeysState(unsigned char * aState, int aSize)
{
	if (aState && aSize > 0 && aSize <= (int)sizeof(g_keys))
		memcpy(aState, g_keys, aSize);
}

void dkwShutDown()
{
	dkwClipMouse(false);
	if (g_window)
	{
		SDL_StopTextInput(g_window);
		SDL_DestroyWindow(g_window);
		g_window = 0;
	}
	SDL_Quit();
}
