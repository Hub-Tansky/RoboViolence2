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

#ifndef CONSOLE
#include "IntroScreen.h"
#include "GameVar.h"
#include "Helper.h"



//
// Constructeur
//
IntroScreen::IntroScreen(unsigned int in_font) : font(in_font)
{
	showDelay = 3;
}



//
// Update
//
void IntroScreen::update(float delay)
{
	if (dkiGetFirstDown() != DKI_NOKEY) showDelay = 0;
	showDelay -= delay;
}



//
// Render: "LOADING" fading in and out (PNS-53)
//
void IntroScreen::render()
{
	renderLoading(font, (showDelay > 2) ? 1 - (showDelay - 2) : (showDelay > 1) ? 1 : showDelay);
}



void IntroScreen::renderLoading(unsigned int font, float brightness)
{
	dkglPushOrtho(800, 600);
		glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			dkfBindFont(font);
			glColor3f(brightness, brightness, brightness);
			printCenterText(400, 268, 64, CString("LOADING"));
		glPopAttrib();
	dkglPopOrtho();
}



void IntroScreen::presentLoadingFrame(unsigned int font)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	CVector2i res = dkwGetResolution();

	if(gameVar.r_widescreen > 1)
		glViewport((GLint)((res[0] - res[1]*1.333f)/2.0f), 0, (GLsizei)(res[1]*1.333f), (GLsizei)res[1]);
	else
		glViewport(0, 0, res[0], res[1]);
	dkglSetProjection(60, 1, 50, (float)res[1]*1.333f, (float)res[1]);

	// Default state the loaders expect
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glDisable(GL_TEXTURE_2D);
	glColor3f(1,1,1);

	renderLoading(font, 1);

	dkglSwapBuffers();
}
#endif

