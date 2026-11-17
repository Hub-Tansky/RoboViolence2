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
#include "Writting.h"
#include "Helper.h"


// Le pointeur sur le writting qui a présentemnt le focus
Writting * writting;


//
// Constructeur
//
Writting::Writting()
{
	m_cursorPos = 0;
	m_cursorAnim = 0;
	m_haveFocus = false;
	m_isActivated = false;
	m_sfxWrite = dksCreateSoundFromFile("main/sounds/Write.wav", false);
	m_maxCarac = 79;
}



//
// Destructeur
//
Writting::~Writting()
{
	loseFocus();
	dksDeleteSound(m_sfxWrite);
}



//
// Pour écrire dedans
//
void Writting::writeText(unsigned int caracter)
{
	// caracter is already a font byte (dkw maps typed letters through FontCodes.h, ROBO-238)

	// On reset notre anim bien sur
	m_cursorAnim = 0;
	dksPlaySound(m_sfxWrite, -1, 200);

	// Si on fait backspace
	if (caracter == 8)
	{
		if (m_cursorPos > 0)
		{
			remove(m_cursorPos-1);
			m_cursorPos--;
		}
		return;
	}
	
	// Enter
	if (caracter == 13)
	{
		m_isActivated = true;
		return;
	}

	// On check pour changer la couleur
	if (caracter >= '1' && caracter <= '9')
	{
		if (m_cursorPos > 0)
		{
			if (s[m_cursorPos-1] == '^')
			{
				remove(m_cursorPos-1);
				m_cursorPos--;

				if (caracter == '1') insert(CString("\x1"), m_cursorPos);
				if (caracter == '2') insert(CString("\x2"), m_cursorPos);
				if (caracter == '3') insert(CString("\x3"), m_cursorPos);
				if (caracter == '4') insert(CString("\x4"), m_cursorPos);
				if (caracter == '5') insert(CString("\x5"), m_cursorPos);
				if (caracter == '6') insert(CString("\x6"), m_cursorPos);
				if (caracter == '7') insert(CString("\x7"), m_cursorPos);
				if (caracter == '8') insert(CString("\x8"), m_cursorPos);
				if (caracter == '9') insert(CString("\x9"), m_cursorPos);
				m_cursorPos++;
				return;
			}
		}
	}

	// On écris notre caractère
	if (caracter >= 32 && caracter <= 159)
	{
		insert(CString("%c", caracter), m_cursorPos);
		m_cursorPos++;
	}

	if (len() > m_maxCarac) 
	{
		m_cursorPos = m_maxCarac;
		resize(m_maxCarac);
	}
}


//
// Replace text with new string
//
void Writting::replaceText(CString str)
{
	clear();
	set("%s", str.s);
	m_cursorPos = len();
}



//
// Pour updater
//
void Writting::updateWritting(float delay)
{
	// On anim le curseur
	m_cursorAnim += delay;
	while (m_cursorAnim >= 1) m_cursorAnim -= 1;

	// Si on déplace avec les flèches
	if (dkiGetState(DIK_LEFT) == DKI_DOWN)
	{
		dksPlaySound(m_sfxWrite, -1, 200);
		m_cursorAnim = 0;
		m_cursorPos--;
		if (m_cursorPos < 0) m_cursorPos = 0;
	}
	if (dkiGetState(DIK_RIGHT) == DKI_DOWN)
	{
		dksPlaySound(m_sfxWrite, -1, 200);
		m_cursorAnim = 0;
		m_cursorPos++;
		if (m_cursorPos > len()) m_cursorPos = len();
	}
	if (dkiGetState(DIK_HOME) == DKI_DOWN)
	{
		dksPlaySound(m_sfxWrite, -1, 200);
		m_cursorAnim = 0;
		m_cursorPos = 0;
	}
	if (dkiGetState(DIK_END) == DKI_DOWN)
	{
		dksPlaySound(m_sfxWrite, -1, 200);
		m_cursorAnim = 0;
		m_cursorPos = len();
	}
	if (dkiGetState(DIK_DELETE) == DKI_DOWN)
	{
		dksPlaySound(m_sfxWrite, -1, 200);
		m_cursorAnim = 0;
		if (m_cursorPos < len()) 
		{
			remove(m_cursorPos);
			if (m_cursorPos > len()) m_cursorPos = len();
		}
	}
}



//
// pour lui donner le focus du writing
//
void Writting::giveFocus()
{
	if (writting) writting->loseFocus();
	writting = this;
	m_cursorPos = len();
	m_cursorAnim = 0;
	m_haveFocus = true;
}



//
// Pour lui enlever le focus du writing
//
void Writting::loseFocus()
{
	if (writting == this) writting = 0;
	m_haveFocus = false;
}



//
// Pour l'afficher
//
void Writting::print(float size, float x, float y, float z)
{
	// On écris le text normalement
	printLeftText(x,y,size,CString("%s", s));
//	dkfPrint(size, x, y, z, s);

	// On dessine le curseur
	if (m_haveFocus && m_cursorAnim < .5f)
	{
		CString tmp("%s", s);
		tmp.resize(m_cursorPos);
		float posX = dkfGetStringWidth(size, tmp.s);
		dkfPrint(size, x + posX, y, z, "_");
	}
}



//
// pour le reseter
//
void Writting::clear()
{
	reset();
	m_cursorPos = 0;
	m_cursorAnim = 0;
}
#endif

