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

#include "dki.h"
#include "dkw.h"

#include <SDL3/SDL.h>

#include <string.h>

namespace
{
	const int KEYS = 256;
	const int MOUSE_BUTTONS = 8;
	const int JOY_BUTTONS = 128;
	const int ALL = KEYS + MOUSE_BUTTONS + JOY_BUTTONS;

	int g_allState[ALL];
	unsigned char g_keyboard[KEYS];
	DkwMouseState g_mouse;
	int g_mouseX = 0;
	int g_mouseY = 0;

	SDL_Gamepad * g_pad = 0;
	int g_padCheck = 0;
	CVector3f g_joyL;
	CVector3f g_joyR;

	// Same level-to-edge state machine for every input: NOTHING -> DOWN -> HOLD -> UP -> NOTHING.
	void step(int & state, bool down)
	{
		if (down)
			state = (state == DKI_NOTHING || state == DKI_UP) ? DKI_DOWN : DKI_HOLD;
		else
			state = (state == DKI_DOWN || state == DKI_HOLD) ? DKI_UP : DKI_NOTHING;
	}

	float axis(SDL_Gamepad * pad, SDL_GamepadAxis a)
	{
		float v = SDL_GetGamepadAxis(pad, a) / 32767.0f;
		return v < -1.0f ? -1.0f : v;
	}

	void openPad()
	{
		int count = 0;
		SDL_JoystickID * ids = SDL_GetGamepads(&count);
		if (ids && count > 0)
			g_pad = SDL_OpenGamepad(ids[0]);
		SDL_free(ids);
	}
}

// First input that went down since the last update (used by the key binding screen)
int				dkiGetFirstDown()
{
	for (int i = 0; i < ALL; i++)
		if (g_allState[i] == DKI_DOWN)
			return i;
	return DKI_NOKEY;
}

int				dkiGetMouseWheelVel()
{
	return g_mouse.lZ;
}

CVector2i		dkiGetMouse()
{
	return CVector2i(g_mouseX, g_mouseY);
}

CVector2i		dkiGetMouseVel()
{
	return CVector2i(g_mouse.lX, g_mouse.lY);
}

int				dkiGetState(int inputID)
{
	if (inputID == DKI_NOKEY || inputID < 0 || inputID >= ALL)
		return DKI_NOTHING;
	return g_allState[inputID];
}

CVector3f		dkiGetJoy()
{
	return g_joyL;
}

CVector3f		dkiGetJoyR()
{
	return g_joyR;
}

CVector3f		dkiGetJoyVel()
{
	return CVector3f(0, 0, 0);
}

int				dkiInit()
{
	memset(g_allState, 0, sizeof(g_allState));
	memset(g_keyboard, 0, sizeof(g_keyboard));
	memset(&g_mouse, 0, sizeof(g_mouse));
	g_mouseX = 0;
	g_mouseY = 0;
	SDL_InitSubSystem(SDL_INIT_GAMEPAD); // optional: no gamepad is not an error
	openPad();
	return 1;
}

void			dkiShutDown()
{
	if (g_pad)
	{
		SDL_CloseGamepad(g_pad);
		g_pad = 0;
	}
	SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
}

void			dkiUpdate(float elapsef, int width, int height)
{
	(void)elapsef;

	dkwGetKeysState(g_keyboard, KEYS);
	for (int i = 0; i < KEYS; i++)
	{
		step(g_allState[i], (g_keyboard[i] & 0x80) != 0);
	}

	dkwGetMouseState(&g_mouse);
	for (int i = 0; i < MOUSE_BUTTONS; i++)
		step(g_allState[DKI_MOUSE_BUTTON1 + i], (g_mouse.rgbButtons[i] & 0x80) != 0);

	// Virtual cursor: accumulated movement, clamped to the screen
	g_mouseX += g_mouse.lX;
	g_mouseY += g_mouse.lY;
	if (g_mouseX > width - 1) g_mouseX = width - 1;
	if (g_mouseX < 0) g_mouseX = 0;
	if (g_mouseY > height - 1) g_mouseY = height - 1;
	if (g_mouseY < 0) g_mouseY = 0;

	// Gamepad: hot-plug check about once a second, the first pad wins
	if (g_pad && !SDL_GamepadConnected(g_pad))
	{
		SDL_CloseGamepad(g_pad);
		g_pad = 0;
	}
	if (!g_pad && ++g_padCheck >= 30)
	{
		g_padCheck = 0;
		openPad();
	}

	// Button order the key binds were written for (XInput pad): A B X Y LB RB Back Start LS RS
	static const SDL_GamepadButton order[10] = {
		SDL_GAMEPAD_BUTTON_SOUTH, SDL_GAMEPAD_BUTTON_EAST, SDL_GAMEPAD_BUTTON_WEST, SDL_GAMEPAD_BUTTON_NORTH,
		SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER,
		SDL_GAMEPAD_BUTTON_BACK, SDL_GAMEPAD_BUTTON_START,
		SDL_GAMEPAD_BUTTON_LEFT_STICK, SDL_GAMEPAD_BUTTON_RIGHT_STICK };
	for (int i = 0; i < JOY_BUTTONS; i++)
		step(g_allState[DKI_JOY_BUTTON1 + i], g_pad && i < 10 && SDL_GetGamepadButton(g_pad, order[i]));

	if (g_pad)
	{
		g_joyL = CVector3f(axis(g_pad, SDL_GAMEPAD_AXIS_LEFTX), axis(g_pad, SDL_GAMEPAD_AXIS_LEFTY), 0);
		g_joyR = CVector3f(axis(g_pad, SDL_GAMEPAD_AXIS_RIGHTX), axis(g_pad, SDL_GAMEPAD_AXIS_RIGHTY), 0);
	}
	else
	{
		g_joyL = CVector3f(0, 0, 0);
		g_joyR = CVector3f(0, 0, 0);
	}
}

void			dkiSetMouse(CVector2i & mousePos)
{
	g_mouseX = mousePos[0];
	g_mouseY = mousePos[1];
}
