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

#include "Paths.h"
#include "Scene.h"
#include "Console.h"
#ifndef CONSOLE
#include "JoinGame.h"
#include "CStatus.h"
#endif
#include <cstdio>
#include <ctime>


//extern unsigned long masterServerID;



#ifndef CONSOLE
//
// Pour cr�r, join, disconnecter d'une game
//
void Scene::host(CString mapName)
{
	disconnect();
	console->add("\x3> Creating Server");
	server = new Server(new Game(mapName)); // On connect notre jeu au server
	console->add("\x3> Creating Client");
	client = new Client(new Game()); // On connect notre jeu au client
	client->isServer = true;

	// On start le server
	if (!server->host())
	{
		disconnect();
		console->add("\x4> Error creating server");
	//	menu->show();
		menuManager.root->visible = true;
		return;
	}
	else
	{
		console->add(CString("\x3> Server Created on port %i", gameVar.sv_port));
	}

	// Voil� maintenant on le join
	if (!client->join("127.0.0.1", gameVar.sv_port))
	{
		disconnect();
		console->add("\x4> Error connecting to server. Verify the IP adress");
	//	menu->show();
		menuManager.root->visible = true;
		return;
	}
	else
	{
	//	console->add("\x3> Connection successfull");
	//	menu->hide();
		menuManager.root->visible = false;
	}
}
#endif

void Scene::dedicate(CString mapName)
{
	disconnect();
	console->add("\x3> Creating Server");
	server = new Server(new Game(mapName)); // On connect notre jeu au server

	// On start le server
	if (!server->host())
	{
		disconnect();
		console->add("\x4> Error creating server");
#ifndef CONSOLE
	//	menu->show();
		menuManager.root->visible = true;
#endif
	}
	else
	{
		console->add(CString("\x3> Server Created on port %i", gameVar.sv_port));
#ifndef CONSOLE
	//	menu->hide();
		menuManager.root->visible = false;
#endif
	}
}

#ifndef CONSOLE

void Scene::join(CString IPAddress, int port, CString password/*, CString adminRequest*/)
{
	disconnect();
	console->add("\x3> Creating Client");
	client = new Client(new Game()); // On connect notre jeu au client

/*	if (!adminRequest.isNull())
	{
		client->adminUserPassRequest = adminRequest;
		client->requestedAdmin = true;
	}*/

	// Voil� maintenant on le join
	if (!client->join(IPAddress, port, password))
	{
		disconnect();
		console->add("\x4> Error connecting to server. Verify the IP address");
	//	menu->show();
		menuManager.root->visible = true;
	}
	else
	{
	//	console->add("\x3> Connection successfull");
	//	menu->hide();
		menuManager.root->visible = false;
	}
}



void Scene::edit(CString command)
{
	CString mapName = command.getFirstToken(' ');
	int w,h;
	CString token = command.getFirstToken(' ');
	if (token.isNull()) w = 0;
	else w = token.toInt();
	token = command.getFirstToken(' ');
	if (token.isNull()) h = 0;
	else h = token.toInt();
	disconnect();
	console->add("\x3> Creating Editor");
//	ZEVEN_SAFE_DELETE(menuManager.root);
	editor = new Editor(mapName, font, w, h); // On connect notre jeu au client
//	menu->hide();
	menuManager.root->visible = false;
}
#endif


void Scene::disconnect()
{
#ifndef CONSOLE
	if (client && !server) dksvarLoadConfigSVOnly((char*)bv2::configFile().c_str());

	status->set(CStatus::ONLINE);

	bool wrongVersion = false;
	if (client) wrongVersion = client->wrongVersionReason;
	console->add("\x3> Disconnecting from current server");
	if (client) console->add("\x3> Shutdowning Client");
	ZEVEN_SAFE_DELETE(client);
#endif
	bool wasServer = false;
	if (server) {console->add("\x3> Shutdowning Server");wasServer=true;}
	ZEVEN_SAFE_DELETE(server);
#ifndef CONSOLE
	if (editor) console->add("\x3> Shutdowning Editor");
	ZEVEN_SAFE_DELETE(editor);
	console->unlock(); // Petit bug quand on chattait ;)
//	menu->show();
	if (menuManager.root) menuManager.root->visible = true; // null while the intro screen is showing
//	if (wrongVersion) menu->currentMenu = MENU_WRONG_VERSION;
#endif
}



// Pour chatter
void Scene::sayall(CString message)
{
#ifndef CONSOLE
	if (client) client->sayall(message);
	else 
#endif
		if (server) server->sayall(message);
}

#ifndef CONSOLE
void Scene::sayteam(CString message)
{
	if (client) client->sayteam(message);
}
#endif


// pour kicker un FUCKING cheateux
void Scene::kick(CString playerName)
{
	if (server)
	{
		for (int i=0;i<MAX_PLAYER;++i)
		{
			if (server->game)
			{
				if (server->game->players[i])
				{
					if (textColorLess(server->game->players[i]->name) == textColorLess(playerName))
					{
						console->add(CString("\x3> Disconnecting client %s (%s), kicked by server", server->game->players[i]->name.s, server->game->players[i]->playerIP), true);
						server->dropSlot(i, "kick");
						continue;
					}
				}
			}
		}
	}
}


void Scene::kick(int ID)
{
	if (server)
	{
		if (server->game)
		{
			if (server->game->players[ID])
			{
				console->add(CString("\x3> Disconnecting client %s (%s), kicked by server", server->game->players[ID]->name.s, server->game->players[ID]->playerIP), true);
				server->dropSlot(ID, "kick");
			}
		}
	}
}

void Scene::ban(CString playerName)
{
	if (server)
	{
		for (int i=0;i<MAX_PLAYER;++i)
		{
			if (server->game)
				if (server->game->players[i])
			{
				if (textColorLess(server->game->players[i]->name) == textColorLess(playerName))
				{
					ban(server->game->players[i]->playerID);
				}
			}
		}
	}
}

void Scene::ban(int ID)
{
	if (server)
		if (server->game)
			if (server->game->players[ID])
	{

		server->bans.add(server->game->players[ID]->name.s, server->game->players[ID]->playerIP, (long long)time(nullptr));

		console->add(CString("\x3> Disconnecting client %s (%s), banned by server", server->game->players[ID]->name.s, server->game->players[ID]->playerIP), true);
		server->dropSlot(ID, "ban");
	}
}

void Scene::banIP(CString playerIP)
{
	if (server)
	{
		server->bans.add("MANUAL-IP-BAN", playerIP.s, (long long)time(nullptr));
	}
}

void Scene::unban(int banID)
{
	if(server)
		if (server->game)
	{
		server->bans.remove(banID);
	}
}


