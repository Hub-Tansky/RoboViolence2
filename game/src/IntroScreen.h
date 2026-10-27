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

#ifndef INTROSCREEN_H
#define INTROSCREEN_H


#include "Zeven.h"


class IntroScreen
{
public:
	// Le delay qui reste à l'afficher
	float showDelay;

	// The menu font (owned by Scene)
	unsigned int font;

public:
	// Constructeur
	explicit IntroScreen(unsigned int in_font);

	// Update
	void update(float delay);

	// Render
	void render();

	// Clears the screen, draws "LOADING" and swaps: the frame Scene and Map show while they load
	static void presentLoadingFrame(unsigned int font);

private:
	// "LOADING" in the big menu font
	static void renderLoading(unsigned int font, float brightness);
};


#endif

