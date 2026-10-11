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

#ifndef CLIENT_H
#define CLIENT_H

#define GAME_VERSION_CL 22000

#define MIN_TIME_BETWEEN_QMSG 0.9f


#include "Game.h"
#include "ControlListener.h"
#include "Button.h"
#include "Writting.h"
#include "CListener.h"


// Nos message qu'on affiche à l'écran
struct TimedMessage
{
	CString message;
	float duration; // Sa vie quil lui reste, on laisse les message 10sec ( si sont rendu trop haut, on les force à closer )
	TimedMessage(CString pMessage)
	{
		duration = 10;
		message = pMessage;
	}
};


class Client :/* public ControlListener, */public CListener
{
public:
	// Le son pour quand on clic
	DksSound * m_sfxClic;

	// Le son pour quand on passe au dessus
	DksSound * m_sfxOver;

	// Notre jeu
	Game * game;

	CString adminRequest;
	bool requestedAdmin;

	CString m_password;

	// La font
	unsigned int font;

	bool isAdmin;

	float autoBalanceTimer;

	bool isDownloadingMap;

	float timeSinseLastQMsg;
	// Les messages chat ou events à printer à l'écran
	std::vector<TimedMessage> chatMessages;
	std::vector<TimedMessage> eventMessages;

	// Si le client run
	bool isRunning;
   bool proServer;

	// une erreur ou de quoi, on doit shutdowner
	bool needToShutDown;
	bool wrongVersionReason;

	// NET_SVCL_GAMEVERSION arrived before our NET_SVCL_NEWPLAYER slot; answer once the slot exists.
	bool pendingVersionAccept;

	// Si on montre le menu
	bool showMenu;

	// Si on est connecté
	bool isConnected;

	CString server_ip;

	CControl * clientRoot;

	// Nos items du menu
	CControl * btn_disconnect;
	CControl * btn_autoAssign;
	CControl * btn_blueTeam;
	CControl * btn_redTeam;
	CControl * btn_spectator;
	CControl * btn_mainMenu;

	// Les boutons pour les guns
	CControl * btn_guns[10];
	CControl * btn_meleeguns[10];
	CControl* currentGun;
	CControl* currentMelee;

	// Si on clic sur un bouton
	void onClick(Control * control);
	void Click(CControl * control);

	// Si il cohabite avec un server
	bool isServer;

	// Quand on écris
	Writting chatting;
	bool isChattingTeam;

	// Le son pour quand un chat message arrive
	DksSound * m_sfxChat;
	DksSound * sfxHit;
	DksSound * sfxShotyReload;

	// Le bink pour sa vie (et peut etre le reloading kin)
	float blink;

	// Le hit indicator
	float hitIndicator;

	// Quand on se fait toucher, l'écran devient rouge
	unsigned int tex_screenHit;
	unsigned int tex_grenadeLeft;
	unsigned int tex_shotgunLeft;
	unsigned int tex_molotovLeft;
	unsigned int tex_blueFlag;
	unsigned int tex_redFlag;
	unsigned int tex_crossHit;

	// Il a reçu le gamestate
	bool gotGameState;

	long serverFrameID;

	//--- Le client ID
	unsigned long uniqueClientID;

public:
	// Constructeur
	Client(Game * pGame);

	// Destructeur
	virtual ~Client();

	// Pour l'updater
	void update(float delay);

	// Pour l'afficher
	void render(float & alphaScope);

	// Pour rejoindre un server
	int join(CString IPAddress, int port, CString password = "");

	// Pour chatter
	void sayall(CString message);
	void sayteam(CString message);

	// Pour printer un chat message
	void printMessage(CString message);

	// On a reçu un message yéé !
	void recvPacket(char * buffer, int size, int typeID);

	// Sends PLAYER_INFO + GAMEVERSION_ACCEPTED (needs game->thisPlayer)
	void sendJoinHandshake();

	void MouseEnter(CControl * control);
};



#endif

