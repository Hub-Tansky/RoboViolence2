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

#include "CNews.h"
#include <string>

namespace
{
	// Greedy word wrap of text to width pixels at size with the bound font, keeping its own line breaks
	std::string wrapText(float size, const char * text, float width)
	{
		std::string wrapped, line, word;
		for (const char * c = text; ; ++c)
		{
			if (*c != ' ' && *c != '\n' && *c != '\0')
			{
				word += *c;
				continue;
			}
			std::string candidate = line.empty() ? word : line + " " + word;
			if (!line.empty() && dkfGetStringWidth(size, &candidate[0]) > width)
			{
				wrapped += line + "\n";
				candidate = word;
			}
			line = candidate;
			word.clear();
			if (*c == ' ') continue;
			wrapped += line;
			if (*c == '\0') return wrapped;
			wrapped += "\n";
			line.clear();
		}
	}
}

CNews::CNews(CControl * in_parent, CControl * in_alignTo)
{
	m_sfxClic = dksCreateSoundFromFile("main/sounds/Button.wav", false);
	m_sfxOver = dksCreateSoundFromFile("main/sounds/ControlOver.wav", false);

	parent = in_parent;

	//--- Da big frame
	instance = new CControl(parent, CVector2i(0,0), CVector2i(736, 506), "", this, "FRAME", in_alignTo, CONTROL_SNAP_BOTTOM);
	//instance->texture = dktCreateTextureFromFile("main/textures/Menu3Back.tga", DKT_FILTER_LINEAR);
	//instance->borderColor.set(1,.5f,.25f);

	lst_news = new CControl(instance, CVector2i(10,10), CVector2i(716,486), "", this, "LISTBOX");
	lst_news->texture = dktCreateTextureFromFile("main/textures/Menu2Back.tga", DKT_FILTER_LINEAR);

	instance->backColor.set(0,.3f,.7f);
	instance->imgColor = instance->backColor;

	lst_news->backColor.set(0,.3f,.7f);
	lst_news->imgColor = lst_news->backColor;

	animY = 0;
	velY = 0;
	originalY = instance->localPos[1];
	AddNews("Robo Violence 2 is now available", "The #1 Robo",
		"The first public version of Robo Violence 2 is here! A free, top-down multiplayer arena shooter for Windows, macOS and Linux.\n"
		"\n"
		"Fan-made revival of BaboViolent 2, Robo Violence 2 brings its fast-paced arena action back for longtime fans and new players alike.\n"
		"\n"
		"Please note that this is an alpha release, not a finished remake. The first version achieved the foundation for the game's future development: a modern technology stack, support for all three major desktop platforms, and improvements to reliability and server-side performance.\n"
		"\n"
		"The arena is open again. See you in-game!",
		"29.10.2026");
}

CNews::~CNews()
{
	dksDeleteSound(m_sfxClic);
	dksDeleteSound(m_sfxOver);
}


void CNews::MouseEnter(CControl * control)
{
	if (control->style == "BUTTON" || control->style == "LISTBOX" || control->style == "CHECK")
	{
		dksPlaySound(m_sfxOver, -1, 200);
	}
}
void CNews::MouseLeave(CControl * control)
{
}
void CNews::MouseDown(CControl * control)
{
}
void CNews::MouseUp(CControl * control)
{
}
void CNews::MouseMove(CControl * control)
{
}
void CNews::Click(CControl * control)
{
	if (control->style == "BUTTON" || control->style == "LISTBOX" || control->style == "CHECK")
	{
		dksPlaySound(m_sfxClic, -1, 200);
	}
}
void CNews::Validate(CControl * control)
{
	if (control->style == "EDIT")
	{
		dksPlaySound(m_sfxClic, -1, 200);
	}
}

void CNews::AddNews(const CString& title, const CString& author, const CString& content, const CString& date)
{
	const int width = 676;
	const int bodySize = 20;
	dkfBindFont(lst_news->font);
	std::string body = wrapText((float)bodySize, content.s, (float)(width - 6)); // labels pad 3 px on each side
	int bodyHeight = (int)dkfGetStringHeight((float)bodySize, &body[0]) + 6;

	CControl* newsRow = new CControl(lst_news, CVector2i(10, 10), CVector2i(width, 45 + bodyHeight + 50), "", this, "LABEL");

	CControl* Title = new CControl(newsRow, CVector2i(0, 5), CVector2i(width,35), title, this, "LABEL");
	Title->textSize = 35;
	Title->textAlign = CONTROL_TEXTALIGN_TOPLEFT;
	CControl* Content = new CControl(newsRow, CVector2i(0, 45), CVector2i(width,bodyHeight), body.c_str(), this, "LABEL");
	Content->textSize = bodySize;
	Content->textAlign = CONTROL_TEXTALIGN_TOPLEFT;
	CControl* Author = new CControl(newsRow, CVector2i(0, 45 + bodyHeight + 5), CVector2i(width,20), CString("by %s", author.s), this, "LABEL");
	Author->textSize = 20;
	CControl* Date = new CControl(newsRow, CVector2i(0, 45 + bodyHeight + 25), CVector2i(width,20), date, this, "LABEL");
	Date->textSize = 20;
}

#endif

