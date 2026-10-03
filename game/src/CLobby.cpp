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
#include "CLobby.h"
#include "Zeven.h"
#include "CMaster.h" // SBrowsableGame

// The lobby stores SBrowsableGame objects as void*.
static void freeGame(void * game)
{
	delete (SBrowsableGame *)game;
}


CLobby* lobby = 0;



//
//--- Constructor
//
CLobby::CLobby()
{
	lastSent = 0;
}



//
//--- Destructor
//
CLobby::~CLobby()
{
	clearLobby();
	if (lastSent)
	{
		freeGame(lastSent);
		lastSent = 0;
	}
}



//
//--- Push a game in
//
void CLobby::pushGame(void* in_game)
{
	m_games.push_back(in_game);
}



//
//--- Retrieve a game in front
//    If there is no game, this function return null.
//
void* CLobby::getNext()
{
	if (lastSent)
	{
		freeGame(lastSent);
		lastSent = 0;
	}

	if (m_games.size() > 0)
	{
		lastSent = m_games[0];
		m_games.erase(m_games.begin());
		return lastSent;
	}
	else
	{
		return 0;
	}
}



//
//--- Clear all games
//
void CLobby::clearLobby()
{
	for (size_t i = 0; i < m_games.size(); ++i)
		freeGame(m_games[i]);
	m_games.clear();
	if (lastSent)
	{
		freeGame(lastSent);
		lastSent = 0;
	}
}

#endif

