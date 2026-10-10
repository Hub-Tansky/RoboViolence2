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


#include <algorithm>
#include <cstddef>
#include <set>
#include "Server.h"
#include "netPacket.h"
#include "Console.h"
#include "Scene.h"
#include "CCurl.h"
#include <stdio.h>
#include <string.h>
extern Scene* scene;

	#include "md5class.h"

using std::min;


namespace
{
	// Pre-dispatch check (Phase B step 4: Q-S1, Q-S2): what a client may send. size: exact payload bytes, 0 for text
	// (at least one byte); playerID: offset of the packet's playerID, overwritten with the sender's own slot (-1: none).
	// A new client message type needs an entry here.
	struct ClientMessage
	{
		int typeID;
		int size;
		int playerID;
	};
	const ClientMessage clientMessages[] = {
		{NET_CLSV_PONG, sizeof(net_clsv_pong), offsetof(net_clsv_pong, playerID)},
		{NET_CLSV_SPAWN_REQUEST, sizeof(net_clsv_spawn_request), offsetof(net_clsv_spawn_request, playerID)},
		{NET_CLSV_PLAYER_SHOOT, sizeof(net_clsv_player_shoot), offsetof(net_clsv_player_shoot, playerID)},
		{NET_CLSV_GAMEVERSION_ACCEPTED, sizeof(net_clsv_gameversion_accepted), offsetof(net_clsv_gameversion_accepted, playerID)},
		{NET_CLSV_PICKUP_REQUEST, sizeof(net_clsv_pickup_request), offsetof(net_clsv_pickup_request, playerID)},
		{NET_CLSV_ADMIN_REQUEST, sizeof(net_clsv_admin_request), -1},
		{NET_CLSV_VOTE, sizeof(net_clsv_vote), offsetof(net_clsv_vote, playerID)},
		{NET_CLSV_MAP_LIST_REQUEST, sizeof(net_clsv_map_list_request), offsetof(net_clsv_map_list_request, playerID)},
		{NET_CLSV_SVCL_PLAYER_INFO, sizeof(net_clsv_svcl_player_info), offsetof(net_clsv_svcl_player_info, playerID)},
		{NET_CLSV_SVCL_CHAT, sizeof(net_clsv_svcl_chat), -1},
		{NET_CLSV_SVCL_TEAM_REQUEST, sizeof(net_clsv_svcl_team_request), offsetof(net_clsv_svcl_team_request, playerID)},
		{NET_CLSV_SVCL_PLAYER_COORD_FRAME, sizeof(net_clsv_svcl_player_coord_frame), offsetof(net_clsv_svcl_player_coord_frame, playerID)},
		{NET_CLSV_SVCL_PLAYER_CHANGE_NAME, sizeof(net_clsv_svcl_player_change_name), offsetof(net_clsv_svcl_player_change_name, playerID)},
		{NET_CLSV_SVCL_PLAYER_PROJECTILE, sizeof(net_clsv_svcl_player_projectile), offsetof(net_clsv_svcl_player_projectile, playerID)},
		{NET_CLSV_SVCL_PLAYER_SHOOT_MELEE, sizeof(net_clsv_svcl_player_shoot_melee), offsetof(net_clsv_svcl_player_shoot_melee, playerID)},
		{NET_CLSV_SVCL_VOTE_REQUEST, sizeof(net_clsv_svcl_vote_request), offsetof(net_clsv_svcl_vote_request, playerID)},
		{NET_CLSV_MAP_REQUEST, sizeof(net_clsv_map_request), -1},
		{NET_CLSV_SVCL_PLAYER_UPDATE_SKIN, sizeof(net_clsv_svcl_player_update_skin), offsetof(net_clsv_svcl_player_update_skin, playerID)},
		{NET_SVCL_PLAY_SOUND, sizeof(net_svcl_play_sound), -1},
		{NET_SVCL_CONSOLE, 0, -1},
	};

	// A weaponID the server knows (indexes gameVar.weapons[20])
	bool validWeapon(int weaponID)
	{
		return weaponID >= 0 && weaponID < 20 && gameVar.weapons[weaponID];
	}

	// Logs a rejected packet once per sender and message type, so a flood can't fill the log.
	void logRejected(unsigned long bbnetID, int typeID, const char * why)
	{
		static std::set<std::pair<unsigned long, int>> logged;
		if (logged.insert(std::make_pair(bbnetID, typeID)).second)
			console->add(CString("\x9> Dropped message %i from client %lu: %s", typeID, bbnetID, why), true);
	}
}

// Runs before the recvPacket switch: rejects unknown types, wrong sizes and senders without a player slot. On success,
// `out` holds the payload with the sender's own slot as playerID (text is NUL-terminated) and the slot is returned.
int Server::checkPacket(const char * buffer, int size, int typeID, unsigned long bbnetID, std::vector<char> & out)
{
	const ClientMessage * rule = 0;
	for (const ClientMessage & m : clientMessages)
		if (m.typeID == typeID) rule = &m;
	if (!rule) { logRejected(bbnetID, typeID, "unknown type"); return -1; }
	if (rule->size ? size != rule->size : size < 1) { logRejected(bbnetID, typeID, "wrong size"); return -1; }

	int slot = -1;
	for (int i = 0; i < MAX_PLAYER; ++i)
		if (game->players[i] && game->players[i]->babonetID == (INT4)bbnetID) slot = i;
	if (slot < 0) { logRejected(bbnetID, typeID, "no player"); return -1; }

	out.assign(buffer, buffer + size);
	if (rule->size == 0) out.push_back('\0');
	if (rule->playerID >= 0) out[rule->playerID] = (char)slot;
	return slot;
}

//
// On a re� un message y� !
//
void Server::recvPacket(char * wire, int size, int typeID, unsigned long bbnetID)
{
    int i;
	std::vector<char> checked;
	if (checkPacket(wire, size, typeID, bbnetID, checked) < 0) return;
	char * buffer = checked.data();
	switch (typeID)
	{
	case NET_CLSV_MAP_REQUEST:
		{
			net_clsv_map_request request;
			memcpy(&request, buffer, sizeof(net_clsv_map_request));
			request.mapName[15] = '\0';
			if (!request.mapName[0] || strspn(request.mapName, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-") != strlen(request.mapName))
				return; // Q-S6: a map name only, never a path

			SMapTransfer mtrans;
            mtrans.chunkNum = 0;
			mtrans.mapName = request.mapName;
			mtrans.uniqueClientID = bbnetID;
			if (gameVar.c_netlog)
				console->add(CString("server> [net] MAP_REQUEST map=\"%s\" bbnetID=%lu", mtrans.mapName.s, (unsigned long)bbnetID), true);

			// Add to list, server will send chunks on each update
			mapTransfers.push_back(mtrans);
            
			break;
		};
	case NET_CLSV_VOTE:
		{
			if (!gameVar.sv_enableVote) return; // <- LÀ
			net_clsv_vote vote;
			memcpy(&vote, buffer, sizeof(net_clsv_vote));
			vote.value = buffer[offsetof(net_clsv_vote, value)] != 0; // any byte, not just a valid bool
			if (game)
			{
				if (game->players[vote.playerID])
				{
					std::vector<int>::iterator it = std::find(game->voting.activePlayersID.begin(),
						game->voting.activePlayersID.end(), game->players[vote.playerID]->babonetID);
					if ( (!game->players[vote.playerID]->voted) && (it != game->voting.activePlayersID.end()) )
					{
						game->players[vote.playerID]->voted = true;

						if (vote.value)
						{
							game->voting.votingResults[0]++;
						}
						else
						{
							game->voting.votingResults[1]++;
						}

						//--- Give the vote update to others
						net_svcl_update_vote updateVote;
						updateVote.nbYes = (char)game->voting.votingResults[0];
						updateVote.nbNo = (char)game->voting.votingResults[1];
						bb_serverSend((char*)(&updateVote), sizeof(net_svcl_update_vote), NET_SVCL_UPDATE_VOTE);
					}
				}
			}
			break;
		};
	case NET_CLSV_SVCL_VOTE_REQUEST:
		{
			net_clsv_svcl_vote_request voteRequest;
			memcpy(&voteRequest, buffer, sizeof(net_clsv_svcl_vote_request));
			voteRequest.vote[79] = '\0';
			if (game)
			{
				if (game->players[voteRequest.playerID])
				{
					//--- Is there a voting in progress?
					if (game->voting.votingInProgress)
					{
						//--- Cancel that
						console->add(CString("\x9> Warning, player %s trying to cast a vote, there is already a vote in progress", game->players[voteRequest.playerID]->name.s));
					}
					else
					{
						//--- Check the validity of the vote, then cast it
						if (!validateVote(voteRequest.vote))
						{
							console->add(CString("\x9> Warning, player %s trying to cast an invalid vote", game->players[voteRequest.playerID]->name.s));
						}
						else
						{
							for (int i=0;i<MAX_PLAYER;++i)
							{
								if (game->players[i])
								{
									game->players[i]->voted = false;
								}
							}
							game->voting.castVote(game, voteRequest);
							/*for (i=0;i<MAX_PLAYER;++i)
							{
								if ( (game->players[i]) && (game->players[i]->teamID != PLAYER_TEAM_AUTO_ASSIGN) &&
									(game->players[i]->teamID != PLAYER_TEAM_AUTO_ASSIGN) )
									++game->voting.nbActivePlayers;
							}*/
							game->voting.votingFrom = textColorLess(game->players[voteRequest.playerID]->name);
							bb_serverSend((char*)(&voteRequest), sizeof(net_clsv_svcl_vote_request), NET_CLSV_SVCL_VOTE_REQUEST);
						}
					}
				}
			}
			break;
		}
	case NET_CLSV_SVCL_PLAYER_SHOOT_MELEE:
		{
			net_clsv_svcl_player_shoot_melee playerShootMelee;
			memcpy(&playerShootMelee, buffer, sizeof(net_clsv_svcl_player_shoot_melee));
			if (game->players[playerShootMelee.playerID])
			{
				if (game->players[playerShootMelee.playerID]->status == PLAYER_STATUS_ALIVE)
				{
					if (game->players[playerShootMelee.playerID]->meleeWeapon)
					{
						game->players[playerShootMelee.playerID]->meleeWeapon->shootMeleeSV(game->players[playerShootMelee.playerID]);
						//--- Shoot � aux autres
					//	for (int i=0;i<MAX_PLAYER;++i)
					//	{
					//		if (game->players[i] && i != playerShootMelee.playerID)
					//		{
								bb_serverSend((char*)&playerShootMelee, sizeof(net_clsv_svcl_player_shoot_melee), NET_CLSV_SVCL_PLAYER_SHOOT_MELEE, 0);//game->players[i]->babonetID);
					//		}
					//	}
					}
				}
			}
			break;
		}
	case NET_SVCL_CONSOLE:
		{
			if (game)
			{
				CString adminCommand("%s", buffer); // R22: data, not a format
				for (int i=0;i<MAX_PLAYER;++i)
				{
					if (game->players[i])
					{
						if (game->players[i]->babonetID == bbnetID && game->players[i]->isAdmin)
						{
							CString log = "Executing command from ";
							log += game->players[i]->name;
							log += ", IP: ";
							log += game->players[i]->playerIP;
							console->add(log);
							console->sendCommand(adminCommand, true, bbnetID);
						}
					}
				}
			}
			break;
		}
	case NET_CLSV_ADMIN_REQUEST:
		{
			net_clsv_admin_request adminRequest;
			memcpy(&adminRequest, buffer, sizeof(net_clsv_admin_request));
			adminRequest.login[32] = '\0';
			adminRequest.password[32] = '\0';
			CString loginRecv("%s", adminRequest.login); // R23: data, not a format
			CString pwdRecv("%s", adminRequest.password);

			if (!gameVar.zsv_adminPass.isNull() &&
				!gameVar.zsv_adminUser.isNull())
			{
				if (loginRecv.isNull() || pwdRecv.isNull())
				{
					for (int i=0;i<MAX_PLAYER;++i)
					{
						if (game->players[i])
						{
							if (game->players[i]->babonetID == bbnetID)
							{
								game->players[i]->isAdmin = false;
								break;
							}
						}
					}
					break;
				}
				CMD5 login_(gameVar.zsv_adminUser.s);
				CString login("%s", login_.getMD5Digest());

				CMD5 pwd_(gameVar.zsv_adminPass.s);
				CString pwd("%s", pwd_.getMD5Digest());

				/*console->add(CString("\x9> L: %s", login.s));
				console->add(CString("\x9> P: %s", loginRecv.s));
				console->add(CString("\x9> L: %s", pwd.s));
				console->add(CString("\x9> P: %s", pwdRecv.s));*/

				if (loginRecv == login && pwdRecv == pwd)
				{
					//--- Admin accepted !!
					for (int i=0;i<MAX_PLAYER;++i)
					{
						if (game->players[i])
						{
							if (game->players[i]->babonetID == bbnetID)
							{
								bb_serverSend(0, 0, NET_SVCL_ADMIN_ACCEPTED, bbnetID);
								game->players[i]->isAdmin = true;
								CString logInName = "Admin name: ";
								CString logInIP = "Admin IP: ";
								logInName += game->players[i]->name;
								logInIP += game->players[i]->playerIP;
								console->add(logInName);
								console->add(logInIP);
								break;
							}
						}
					}
				}
				else
				{//Failed log in
					for (int i=0;i<MAX_PLAYER;++i)
					{
						if (game->players[i])
						{
							if (game->players[i]->babonetID == bbnetID)
							{
								CString logInName = "Admin log-in attempt by: ";
								CString logInIP = "Admin log-in attempt IP: ";
								logInName += game->players[i]->name;
								logInIP += game->players[i]->playerIP;
								console->add(logInName);
								console->add(logInIP);
								break;
							}
						}
					}
				}
			}
			break;
		}
	case NET_CLSV_GAMEVERSION_ACCEPTED:
		{
			net_clsv_gameversion_accepted gameVersionAccepted;
			memcpy(&gameVersionAccepted, buffer, sizeof(net_clsv_gameversion_accepted));
			gameVersionAccepted.password[15] = '\0';

			if (game->players[gameVersionAccepted.playerID])
			{
				if(gameVar.sv_password != "" && CString("%s", gameVersionAccepted.password) != gameVar.sv_password)
				{
					if (gameVar.c_netlog)
						console->add(CString("server> [net] join password mismatch for slot %i, disconnecting", (int)gameVersionAccepted.playerID), true);
					bb_serverDisconnectClient(game->players[gameVersionAccepted.playerID]->babonetID);
					break;
				}

				if (gameVar.c_netlog)
					console->add(CString("server> [net] handshake OK slot=%i -> SERVER_INFO map=\"%s\"", (int)gameVersionAccepted.playerID, game->mapName.s), true);
				// Handshake done: restart the ping watchdog so pre-accept time doesn't count.
				game->players[gameVersionAccepted.playerID]->waitForPong = false;
				game->players[gameVersionAccepted.playerID]->currentPingFrame = 0;

				// On envoi �CE player l'info sur la game
				net_svcl_server_info serverInfo;
				serverInfo.mapSeed = 0; // Pour l'instant on mettra rien (on va mettre le non dla map bientot)
				memcpy(serverInfo.mapName, game->mapName.s, game->mapName.len()+1);
				serverInfo.blueScore = game->blueScore;
				serverInfo.redScore = game->redScore;
				serverInfo.blueWin = game->blueWin;
				serverInfo.redWin = game->redWin;
				serverInfo.gameType = game->gameType;
				bb_serverSend((char*)&serverInfo, sizeof(net_svcl_server_info), NET_SVCL_SERVER_INFO, game->players[gameVersionAccepted.playerID]->babonetID);

				// On lui envoit l'info sur le round
				net_svcl_round_state roundState;
				roundState.newState = game->roundState;
				roundState.reInit = false;
				bb_serverSend((char*)&roundState, sizeof(net_svcl_round_state), NET_SVCL_GAME_STATE, game->players[gameVersionAccepted.playerID]->babonetID);

				// On lui envoit la valeur de ses variables sv_
				gameVar.sendSVVar(game->players[gameVersionAccepted.playerID]->babonetID);

				// On lui envoit l'info sur tout les autres player
				for (i=0;i<MAX_PLAYER;++i)
				{
					if (game->players[i] && i != gameVersionAccepted.playerID)
					{
						net_svcl_player_enum_state playerState;
						playerState.playerID = (char)i;
						memcpy(playerState.playerName, game->players[i]->name.s, game->players[i]->name.len() + 1);
						memcpy(playerState.playerIP, game->players[i]->playerIP, 16);
                        playerState.kills = (short)game->players[i]->kills;
						playerState.deaths = (short)game->players[i]->deaths;
						playerState.score = (short)game->players[i]->score;
						playerState.returns = (short)game->players[i]->returns;
						playerState.flagAttempts = (short)game->players[i]->flagAttempts;
						playerState.damage = (short)game->players[i]->damage;
						playerState.status = (char)game->players[i]->status;
						playerState.teamID = (char)game->players[i]->teamID;
						playerState.life = game->players[i]->life;
						playerState.dmg = game->players[i]->dmg;
						playerState.babonetID = game->players[i]->babonetID;
						memcpy(playerState.skin, game->players[i]->skin.s, (game->players[i]->skin.len() <= 6)?game->players[i]->skin.len()+1:7);
						playerState.blueDecal[0] = (unsigned char)(game->players[i]->blueDecal[0] * 255.0f);
						playerState.blueDecal[1] = (unsigned char)(game->players[i]->blueDecal[1] * 255.0f);
						playerState.blueDecal[2] = (unsigned char)(game->players[i]->blueDecal[2] * 255.0f);
						playerState.greenDecal[0] = (unsigned char)(game->players[i]->greenDecal[0] * 255.0f);
						playerState.greenDecal[1] = (unsigned char)(game->players[i]->greenDecal[1] * 255.0f);
						playerState.greenDecal[2] = (unsigned char)(game->players[i]->greenDecal[2] * 255.0f);
						playerState.redDecal[0] = (unsigned char)(game->players[i]->redDecal[0] * 255.0f);
						playerState.redDecal[1] = (unsigned char)(game->players[i]->redDecal[1] * 255.0f);
						playerState.redDecal[2] = (unsigned char)(game->players[i]->redDecal[2] * 255.0f);
						if (playerState.status == PLAYER_STATUS_ALIVE && game->players[i]->weapon)
						{
							playerState.weaponID = game->players[i]->weapon->weaponID;
						}
						else
						{
							playerState.weaponID = WEAPON_SMG;
						}
						bb_serverSend((char*)&playerState, sizeof(net_svcl_player_enum_state), NET_SVCL_PLAYER_ENUM_STATE, game->players[gameVersionAccepted.playerID]->babonetID);
					}
				}

				// Il faut lui envoyer tout les projectiles aussi!
				for (i=0;i<(int)game->projectiles.size();++i)
				{
					Projectile * projectile = game->projectiles[i];

					net_clsv_svcl_player_projectile playerProjectile;
					playerProjectile.nuzzleID = 0; // Ici on s'en caliss
					playerProjectile.playerID = projectile->fromID;
					playerProjectile.position[0] = (short)(projectile->currentCF.position[0] * 100);
					playerProjectile.position[1] = (short)(projectile->currentCF.position[1] * 100);
					playerProjectile.position[2] = (short)(projectile->currentCF.position[2] * 100);
					playerProjectile.projectileType = projectile->projectileType;
					playerProjectile.vel[0] = (char)(projectile->currentCF.vel[0] * 10);
					playerProjectile.vel[1] = (char)(projectile->currentCF.vel[1] * 10);
					playerProjectile.vel[2] = (char)(projectile->currentCF.vel[2] * 10);
					playerProjectile.uniqueID = projectile->uniqueID;
					switch (playerProjectile.projectileType)
					{
					case PROJECTILE_ROCKET:
						playerProjectile.weaponID = WEAPON_BAZOOKA;
						break;
					case PROJECTILE_GRENADE:
						playerProjectile.weaponID = WEAPON_GRENADE;
						break;
					case PROJECTILE_COCKTAIL_MOLOTOV:
						playerProjectile.weaponID = WEAPON_COCKTAIL_MOLOTOV;
						break;
					case PROJECTILE_FLAME:
						playerProjectile.weaponID = -2;
						break;
					case PROJECTILE_LIFE_PACK:
					case PROJECTILE_DROPED_WEAPON:
					case PROJECTILE_DROPED_GRENADE:
					default:
						playerProjectile.weaponID = -1;
						break;
					}
					bb_serverSend((char*)&playerProjectile, sizeof(net_clsv_svcl_player_projectile), NET_CLSV_SVCL_PLAYER_PROJECTILE, game->players[gameVersionAccepted.playerID]->babonetID);
				}

				// Hum les deux sont des entities, note �moi m�e : les consid�er comme la meme chose next time

				// Envoyer l'etat des flags
				if (game->gameType == GAME_TYPE_CTF)
				{
					net_svcl_flag_enum flagEnum;
					flagEnum.flagState[0] = game->map->flagState[0];
					flagEnum.positionBlue[0] = game->map->flagPos[0][0];
					flagEnum.positionBlue[1] = game->map->flagPos[0][1];
					flagEnum.positionBlue[2] = game->map->flagPos[0][2];
					flagEnum.flagState[1] = game->map->flagState[1];
					flagEnum.positionRed[0] = game->map->flagPos[1][0];
					flagEnum.positionRed[1] = game->map->flagPos[1][1];
					flagEnum.positionRed[2] = game->map->flagPos[1][2];
					bb_serverSend((char*)&flagEnum, sizeof(net_svcl_flag_enum), NET_SVCL_FLAG_ENUM, game->players[gameVersionAccepted.playerID]->babonetID);
				}
			}
			break;
		}
	case NET_CLSV_SVCL_PLAYER_INFO:
		{
			net_clsv_svcl_player_info playerInfo;
			memcpy(&playerInfo, buffer, sizeof(net_clsv_svcl_player_info));
			if (game->players[playerInfo.playerID])
			{
				playerInfo.playerName[31] = '\0';
				playerInfo.username[20] = '\0';
				playerInfo.macAddr[19] = '\0';
				game->players[playerInfo.playerID]->name = playerInfo.playerName;
				memcpy(playerInfo.playerIP,game->players[playerInfo.playerID]->playerIP, 16);
				bb_serverSend((char*)&playerInfo, sizeof(net_clsv_svcl_player_info), NET_CLSV_SVCL_PLAYER_INFO, 0);
#ifndef CONSOLE
				console->add(CString("\x3server> %s joined the game", playerInfo.playerName), true);
#else
				console->add(CString("server> %s joined the game id:%d", playerInfo.playerName, playerInfo.playerID), true);
#endif
				// broadcast the info at remote admins
				if( master ) master->RA_NewPlayer( textColorLess(CString("%s", playerInfo.playerName)).s, playerInfo.playerIP, (long)playerInfo.playerID );

				// Password is transfered sans null terminator, already MD5'd
				char pw[33];
				memcpy(pw, playerInfo.password, 32);
				pw[32] = '\0';

				// Prepare data string
				CUrlData data;
				data.add("action", "auth");
				data.add("username", CString("%s",playerInfo.username).s);
				data.add("password", CString("%s", pw).s);
				data.add("location", "in_game");
				data.add("server_name", gameVar.sv_gameName.s);
				data.add("server_ip", bb_getMyIP());
				data.add("server_port", gameVar.sv_port);

				console->add("[Auth] Sending request");

				// Send a new authorization request
				CCurl* request = new CCurl(gameVar.db_accountServer, data.get());
				authRequests.push_back(request);
				request->start(new int(playerInfo.playerID));
				if( gameVar.sv_gamePublic )
				{
					// test to see if we need to ask master if the guy is banned...legal issues
					bool canTest = true;
					if( strstr ( playerInfo.playerIP,"192.168."))
					{
						canTest = false;
					}

					if( strstr ( playerInfo.playerIP,"127.0.0.1"))
					{
						canTest = false;
					}

					if( canTest )
					{
						stCacheBanned banned;
						banned.ID	=	playerInfo.playerID;
						sprintf( banned.IP , "%s" , playerInfo.playerIP );
						sprintf( banned.MAC , "%s", playerInfo.macAddr );
	
	
						if ( master )
						{
							master->sendPacket( (char*)&banned , sizeof(stCacheBanned) , CACHE_BANNED );
						}
					}
				}


				//let's cache the player
				//see if his mac adress is already in the  list first, if so, overwrite
				for( int i = 0; i < 50; i++)
				{
					if( !stricmp( CachedPlayers[i].macAddr , playerInfo.macAddr ) && CachedPlayers[i].Valid )
					{
						memcpy( CachedPlayers[CachedIndex].IP , playerInfo.playerIP , 16 );
						memcpy( CachedPlayers[CachedIndex].NickName , playerInfo.playerName , 32 );
						memcpy( CachedPlayers[CachedIndex].macAddr , playerInfo.macAddr , 20 );
						return;
					}
				}
				memcpy( CachedPlayers[CachedIndex].IP , playerInfo.playerIP , 16 );
				memcpy( CachedPlayers[CachedIndex].NickName , playerInfo.playerName , 32 );
				memcpy( CachedPlayers[CachedIndex].macAddr , playerInfo.macAddr , 20 );
				CachedPlayers[CachedIndex].Valid = true;
				CachedIndex++;

				if( CachedIndex == 50 ) CachedIndex = 0;

			}
			break;
		}
	case NET_SVCL_PLAY_SOUND:
		{
			// Q-S7: a known sound inside the map, else nothing is relayed
			net_svcl_play_sound playSound;
			memcpy(&playSound, buffer, sizeof(net_svcl_play_sound));
			if (playSound.soundID < SOUND_GRENADE_REBOUND || playSound.soundID > SOUND_PHOTON_START ||
				!game->map || playSound.position[0] >= game->map->size[0] || playSound.position[1] >= game->map->size[1])
				return;
			//--- On l'envoit �toute les autres player
			for (int i=0;i<MAX_PLAYER;++i)
			{
				if (game->players[i])
				{
					if (game->players[i]->babonetID != bbnetID)
					{
						bb_serverSend(buffer, sizeof(net_svcl_play_sound), NET_SVCL_PLAY_SOUND, game->players[i]->babonetID);
					}
				}
			}
			break;
		}
	case NET_CLSV_SVCL_CHAT:
		{
			net_clsv_svcl_chat chat;
			memcpy(&chat, buffer, sizeof(net_clsv_svcl_chat));			
			chat.message[129] = '\0';
			// On print dans console
			console->add(CString("%s", chat.message), false, false); // R21: data, not a format

			// if everybody can see the messagem send it to everyone!
			if( chat.teamID == -2 )
			{			
				bb_serverSend((char*)&chat, sizeof(net_clsv_svcl_chat), NET_CLSV_SVCL_CHAT, 0);
			}
			else if( chat.teamID == PLAYER_TEAM_SPECTATOR )
			{
				for (int i=0;i<MAX_PLAYER;++i)
				{
					if (game->players[i])
					{
						if (game->players[i]->teamID == PLAYER_TEAM_SPECTATOR )
						{
							bb_serverSend((char*)&chat, sizeof(net_clsv_svcl_chat), NET_CLSV_SVCL_CHAT, game->players[i]->babonetID);
						}
					}
				}
			}
			else if( chat.teamID == PLAYER_TEAM_BLUE )
			{
				for (int i=0;i<MAX_PLAYER;++i)
				{
					if (game->players[i])
					{
						if (game->players[i]->teamID == PLAYER_TEAM_BLUE )
						{
							bb_serverSend((char*)&chat, sizeof(net_clsv_svcl_chat), NET_CLSV_SVCL_CHAT, game->players[i]->babonetID);
						}
					}
				}
			}
			else if( chat.teamID == PLAYER_TEAM_RED )
			{
				for (int i=0;i<MAX_PLAYER;++i)
				{
					if (game->players[i])
					{
						if (game->players[i]->teamID == PLAYER_TEAM_RED )
						{
							bb_serverSend((char*)&chat, sizeof(net_clsv_svcl_chat), NET_CLSV_SVCL_CHAT, game->players[i]->babonetID);
						}
					}
				}
			}

			// broadcast to potential remote admins
			CString chatString("%s", chat.message); // R21: data, not a format
			chatString = textColorLess( chatString );
			if( master )
			{
				// find the player id from bbnet id
				int playerid = -1;
				for (int i=0;i<MAX_PLAYER;++i)
				{
					if (game->players[i])
					{
						if (game->players[i]->babonetID == bbnetID )
						{
							playerid = i;
							break;
						}
					}
				}
				master->RA_Chat( chatString.s, playerid );
			}
		
			break;
		}
	case NET_CLSV_SVCL_TEAM_REQUEST:
		{
			net_clsv_svcl_team_request teamRequest;
			memcpy(&teamRequest, buffer, sizeof(net_clsv_svcl_team_request));
			if (teamRequest.teamRequested < PLAYER_TEAM_SPECTATOR || teamRequest.teamRequested > PLAYER_TEAM_AUTO_ASSIGN) return;

			// Est-ce que ce player existe
			if (game->players[teamRequest.playerID])
			{
				char oldTeam = game->players[teamRequest.playerID]->teamID;
				char newTeam = game->assignPlayerTeam(teamRequest.playerID, teamRequest.teamRequested);
				if (newTeam != oldTeam)
				{
					if ((oldTeam == PLAYER_TEAM_RED || oldTeam == PLAYER_TEAM_BLUE) &&
						(game->players[teamRequest.playerID]->timePlayedCurGame > EPSILON))
					{
						cacheStats(game->players[teamRequest.playerID], oldTeam);
						game->players[teamRequest.playerID]->reinit();
					}
					game->players[teamRequest.playerID]->waitForPong = false;
					game->players[teamRequest.playerID]->currentPingFrame = 0;
					teamRequest.teamRequested = newTeam;
					// On l'envoit �tout le monde, (si � chang�
					bb_serverSend((char*)&teamRequest, sizeof(net_clsv_svcl_team_request), NET_CLSV_SVCL_TEAM_REQUEST, 0);
				}
			}
			break;
		}
	case NET_CLSV_PONG:
		{
			net_clsv_pong pong;
			memcpy(&pong, buffer, sizeof(net_clsv_pong));
			const int pid = (unsigned char)pong.playerID;
			if (pid < MAX_PLAYER && game->players[pid])
			{
				if (game->players[pid]->waitForPong)
				{
					game->players[pid]->waitForPong = false;
					game->players[pid]->ping = game->players[pid]->currentPingFrame;
				}
			}
			break;
		}
	case NET_CLSV_SPAWN_REQUEST:
		{
			// Tout respawn sera refussi on a fini le round ou la game
			if (game->roundState == GAME_PLAYING)
			{
				net_clsv_spawn_request spawnRequest;
				memcpy(&spawnRequest, buffer, sizeof(net_clsv_spawn_request));
				spawnRequest.skin[6] = '\0';
				if (!validWeapon(spawnRequest.weaponID) || !validWeapon(spawnRequest.meleeID)) return; // Q-S2
				if (game->players[spawnRequest.playerID])
				{
					//--- Validate weapons, if the validation var is set
					if( gameVar.sv_validateWeapons )
					{
						if (!(spawnRequest.weaponID == WEAPON_SMG ||
							spawnRequest.weaponID == WEAPON_SHOTGUN ||
							spawnRequest.weaponID == WEAPON_SNIPER ||
							spawnRequest.weaponID == WEAPON_DUAL_MACHINE_GUN ||
							spawnRequest.weaponID == WEAPON_CHAIN_GUN ||
							spawnRequest.weaponID == WEAPON_BAZOOKA ||
							spawnRequest.weaponID == WEAPON_PHOTON_RIFLE ||
							spawnRequest.weaponID == WEAPON_FLAME_THROWER))
						{
							//--- Kick him so HARD!
							console->sendCommand(CString("sayall %s is trying to hack his primary weapon! Kicked him", game->players[spawnRequest.playerID]->name.s));
							scene->kick(spawnRequest.playerID);
							return;
						}

						if (!(spawnRequest.meleeID == WEAPON_KNIVES ||
							spawnRequest.meleeID == WEAPON_NUCLEAR ||
							spawnRequest.meleeID == WEAPON_SHIELD
								|| spawnRequest.meleeID == WEAPON_MINIBOT
							))
						{
							//--- Kick him so HARD!
							console->sendCommand(CString("sayall %s is trying to hack his secondary weapon! Kicked him", game->players[spawnRequest.playerID]->name.s));
							scene->kick(spawnRequest.playerID);
							return;
						}
					}

					// *** small checkup to make sure people can't choose minibot if sv_enableMinibot is false
					if( !gameVar.sv_enableMinibot && spawnRequest.meleeID == WEAPON_MINIBOT )
					{
						// force knives
						spawnRequest.meleeID = WEAPON_KNIVES;
					}

					game->players[spawnRequest.playerID]->nextSpawnWeapon = spawnRequest.weaponID;
					game->players[spawnRequest.playerID]->nextMeleeWeapon = spawnRequest.meleeID;

					// On lui trouve une place o le spawner
					// C'est impossible qu'il ne spawn pas (mais bon au cas)
					if (game->spawnPlayer(spawnRequest.playerID))
					{
						// On renvois � �tout le monde
						net_svcl_player_spawn playerSpawn;
						memcpy(playerSpawn.skin, spawnRequest.skin, 7);
						memcpy(playerSpawn.blueDecal, spawnRequest.blueDecal, 3);
						memcpy(playerSpawn.greenDecal, spawnRequest.greenDecal, 3);
						memcpy(playerSpawn.redDecal, spawnRequest.redDecal, 3);
						playerSpawn.weaponID = spawnRequest.weaponID;
						playerSpawn.meleeID = spawnRequest.meleeID;
						playerSpawn.playerID = spawnRequest.playerID;
						playerSpawn.position[0] = (short)(game->players[spawnRequest.playerID]->currentCF.position[0]*10);
						playerSpawn.position[1] = (short)(game->players[spawnRequest.playerID]->currentCF.position[1]*10);
						playerSpawn.position[2] = (short)(game->players[spawnRequest.playerID]->currentCF.position[2]*10);
						spawnRequest.skin[6] = '\0';
						game->players[spawnRequest.playerID]->skin = spawnRequest.skin;
						game->players[spawnRequest.playerID]->blueDecal.set(
							((float)spawnRequest.blueDecal[0]) / 255.0f,
							((float)spawnRequest.blueDecal[1]) / 255.0f,
							((float)spawnRequest.blueDecal[2]) / 255.0f);
						game->players[spawnRequest.playerID]->greenDecal.set(
							((float)spawnRequest.greenDecal[0]) / 255.0f,
							((float)spawnRequest.greenDecal[1]) / 255.0f,
							((float)spawnRequest.greenDecal[2]) / 255.0f);
						game->players[spawnRequest.playerID]->redDecal.set(
							((float)spawnRequest.redDecal[0]) / 255.0f,
							((float)spawnRequest.redDecal[1]) / 255.0f,
							((float)spawnRequest.redDecal[2]) / 255.0f);
						bb_serverSend((char*)&playerSpawn, sizeof(net_svcl_player_spawn), NET_SVCL_PLAYER_SPAWN, 0);

						// add console message of where this guy spawned with what weapon/secondary (to help modders)
						CString spawn = "Player ";
						spawn += game->players[spawnRequest.playerID]->name;
						spawn += " spawned; ID:";
                        spawn += playerSpawn.playerID;
						spawn += " WeaponID:";
						spawn += game->players[spawnRequest.playerID]->nextSpawnWeapon;
						spawn += " SecondaryID:";
						spawn += game->players[spawnRequest.playerID]->nextMeleeWeapon;
						spawn += " Position:";
						spawn += game->players[spawnRequest.playerID]->currentCF.position[0];
						spawn += ",";
						spawn += game->players[spawnRequest.playerID]->currentCF.position[1];
						spawn += " teamID:";
						spawn += game->players[spawnRequest.playerID]->teamID;

						console->add( spawn );
					}
					else
					{
						// Ici le joueur qui l'a request�est bais�
					}
				}
			}
			break;
		}
	case NET_CLSV_SVCL_PLAYER_COORD_FRAME:
		{
		//	console->add("Server recved playerPos");
			net_clsv_svcl_player_coord_frame playerCoordFrame;
			memcpy(&playerCoordFrame, buffer, sizeof(net_clsv_svcl_player_coord_frame));
			if (game->players[playerCoordFrame.playerID])
			{
				if (gameVar.sv_beGoodServer == false &&
					(game->players[playerCoordFrame.playerID]->teamID == PLAYER_TEAM_RED ||
					game->players[playerCoordFrame.playerID]->teamID == PLAYER_TEAM_BLUE) &&
					game->players[playerCoordFrame.playerID]->weapon &&
					game->players[playerCoordFrame.playerID]->weapon->weaponID != WEAPON_SNIPER &&
					playerCoordFrame.camPosZ >= 9) // default z pos is 7
				{
					addDelayedKick(game->players[playerCoordFrame.playerID]->babonetID,
						playerCoordFrame.playerID, 7);
				}
				//--- Is he alive? Else we ignore it
				if (game->players[playerCoordFrame.playerID]->status == PLAYER_STATUS_ALIVE)
				{
					if (game->players[playerCoordFrame.playerID]->babonetID == playerCoordFrame.babonetID)
					{
						game->players[playerCoordFrame.playerID]->timeIdle = 0.0f;
						//--- We compare the time between packet (important against cheating)
						if (game->players[playerCoordFrame.playerID]->lastFrame == 0)
						{
							game->players[playerCoordFrame.playerID]->lastFrame = playerCoordFrame.frameID;
						}
						game->players[playerCoordFrame.playerID]->currentFrame = playerCoordFrame.frameID;
						if (game->players[playerCoordFrame.playerID]->frameSinceLast >= 90)
						{
							// Check for acceleration hack
							CVector3f vel;
							vel[0] = (float)playerCoordFrame.vel[0] / 10.0f;
							vel[1] = (float)playerCoordFrame.vel[1] / 10.0f;
							vel[2] = (float)playerCoordFrame.vel[2] / 10.0f;

							//console->add( CString( "vel : %f  %f  %f" , vel[0] , vel[1] , vel[2] ), true );

							// Frame check: client frames elapsed vs server ticks in the same window.
							// Allow 5 frames + 10% for packet jitter and the client's catch-up ticks
							// after a stall; a real speed hack still drifts far beyond that.
							Player * shp = game->players[playerCoordFrame.playerID];
							long clientFrames = shp->currentFrame - shp->lastFrame;
							bool frameHack = clientFrames > shp->frameSinceLast + 5 + shp->frameSinceLast / 10;

							// Speed check: horizontal speed vs the client clamp (3.25) plus the
							// char*10 quantisation margin. Knockback from a hit legitimately
							// exceeds the clamp, so skip it for 2 s after the server registers one.
							float hSpeed = sqrtf(vel[0] * vel[0] + vel[1] * vel[1]);
							bool speedHack = shp->framesSinceKnockback > 60 && hSpeed > 3.25f + 0.15f;

							if (frameHack || speedHack)
							{
								//--- Hey, on a 10 frame de plus que le server.. hacking???
								game->players[playerCoordFrame.playerID]->speedHackCount++;

								//printf("hackcount++\n");

								//--- Apres 3 shot (9sec) BOUM ON LE KICK L'ENFANT DE PUTE
								if (game->players[playerCoordFrame.playerID]->speedHackCount >= 3)
								{
									// ?????? Save stats to cache
									//cacheStats(game->players[playerCoordFrame.playerID]);

									//printf("anti hack count = 3\n");
									//--- On envoit �tout le monde (y compris lui) quil essaye de speedhacking
									//sayall(CString("Disconnecting %s (%s): Speed Hack Detected", game->players[playerCoordFrame.playerID]->name.s, game->players[playerCoordFrame.playerID]->playerIP));
									if( master ) master->RA_DisconnectedPlayer( textColorLess(game->players[playerCoordFrame.playerID]->name).s, game->players[playerCoordFrame.playerID]->playerIP, (long)game->players[playerCoordFrame.playerID]->playerID);
									bb_serverDisconnectClient(game->players[playerCoordFrame.playerID]->babonetID);
									//console->add(CString("\x3server> POSSIBLE HACKER: %s (%s), Speed Hack", game->players[playerCoordFrame.playerID]->name.s, game->players[playerCoordFrame.playerID]->playerIP), true);
									ZEVEN_SAFE_DELETE(game->players[playerCoordFrame.playerID]);
									net_svcl_player_disconnect playerDisconnect;
									playerDisconnect.playerID = (char)playerCoordFrame.playerID;
									bb_serverSend((char*)&playerDisconnect,sizeof(net_svcl_player_disconnect),NET_SVCL_PLAYER_DISCONNECT,0);
									return;
								}
							}
							else
							{
								//--- Reset hacking count
								game->players[playerCoordFrame.playerID]->speedHackCount = 0;
							}

							game->players[playerCoordFrame.playerID]->frameSinceLast = 0;
							game->players[playerCoordFrame.playerID]->lastFrame = 0;
							game->players[playerCoordFrame.playerID]->currentFrame = 0;
						}
						game->players[playerCoordFrame.playerID]->setCoordFrame(playerCoordFrame);

						
						// Check for teleportation hack * Not anymore , too many problems
						//CVector3f vDiff;
						//vDiff = game->players[playerCoordFrame.playerID]->currentCF.position - game->players[playerCoordFrame.playerID]->lastCF.position;
						//
						//game->players[playerCoordFrame.playerID]->miNbCoord++;

						//#ifdef BV2_PLATFORM_WINDOWS
						//	game->players[playerCoordFrame.playerID]->mfCumulativeVel += max( fabsf(vDiff.x()) , fabsf(vDiff.y()) );
						//#else
						//	game->players[playerCoordFrame.playerID]->mfCumulativeVel += amax( fabsf(vDiff.x()) , fabsf(vDiff.y()) );
						//#endif

						// as it been 3 seconds ?
						// if so, check the average velocity
						//if( game->players[playerCoordFrame.playerID]->mfCFTimer > 3.0f )
						//{
						//	game->players[playerCoordFrame.playerID]->mfCFTimer = 0;

						//	float average =  game->players[playerCoordFrame.playerID]->mfCumulativeVel / 90.0f / dkcGetElapsedf();

						//	//console->add(CString("average vel : %f   el %f",average,dkcGetElapsedf()));
						//	// hax0rz
						//	if( average > 2.5f )
						//	{
						//		sayall(CString("%s forced to spectator : teleporting suspected", game->players[playerCoordFrame.playerID]->name.s, game->players[playerCoordFrame.playerID]->playerIP));
						//		net_clsv_svcl_team_request teamRequest;
						//		teamRequest.playerID = playerCoordFrame.playerID;
						//		teamRequest.teamRequested = PLAYER_TEAM_SPECTATOR;
						//		bb_serverSend((char*)&teamRequest, sizeof(net_clsv_svcl_team_request), NET_CLSV_SVCL_TEAM_REQUEST, 0);
						//		game->players[playerCoordFrame.playerID]->status = PLAYER_STATUS_DEAD;

						//	}

						//	game->players[playerCoordFrame.playerID]->miNbCoord = 0;
						//	game->players[playerCoordFrame.playerID]->mfCumulativeVel = 0.0f;
						//}


						//if( fabsf(vDiff.x()) > 5.0f || fabsf(vDiff.y()) > 5.0f )
						//{	
						//	// hax0rz
						//	bb_serverDisconnectClient(game->players[playerCoordFrame.playerID]->babonetID);
						//	ZEVEN_SAFE_DELETE(game->players[playerCoordFrame.playerID]);
						//	net_svcl_player_disconnect playerDisconnect;
						//	playerDisconnect.playerID = (char)playerCoordFrame.playerID;
						//	bb_serverSend((char*)&playerDisconnect,sizeof(net_svcl_player_disconnect),NET_SVCL_PLAYER_DISCONNECT,0);
						//	return;
						//}

						
					}
					// Sinon ce babonet ID est d�uet
				}
				else
				{
					game->players[playerCoordFrame.playerID]->speedHackCount = 0;
					game->players[playerCoordFrame.playerID]->frameSinceLast = 0;
					game->players[playerCoordFrame.playerID]->lastFrame = 0;
					game->players[playerCoordFrame.playerID]->currentFrame = 0;
				}
			}
			break;
		}
	case NET_CLSV_SVCL_PLAYER_CHANGE_NAME:
		{
			net_clsv_svcl_player_change_name playerChangeName;
			memcpy(&playerChangeName, buffer, sizeof(net_clsv_svcl_player_change_name));
			if (game->players[playerChangeName.playerID])
			{
				//what was this for?
				//if (game->players[playerChangeName.playerID]->status != PLAYER_STATUS_ALIVE)
				//	return;
				playerChangeName.playerName[31] = '\0';
				console->add(CString("\x3server> Player %s changed his name for %s", game->players[playerChangeName.playerID]->name.s, playerChangeName.playerName));
				game->players[playerChangeName.playerID]->name = playerChangeName.playerName;

				// On envoit maintenant � �tout les autres joueurs
				for (int i=0;i<MAX_PLAYER;++i)
				{
					if (game->players[i] && i!=playerChangeName.playerID)
					{
						bb_serverSend((char*)&playerChangeName, sizeof(net_clsv_svcl_player_change_name), NET_CLSV_SVCL_PLAYER_CHANGE_NAME, game->players[i]->babonetID);
					}
				}
			}
			break;
		}
	case NET_CLSV_PLAYER_SHOOT:
		{
			net_clsv_player_shoot playerShoot;
			memcpy(&playerShoot, buffer, sizeof(net_clsv_player_shoot));
			// Q-S2, R25: a known weapon, from a player who holds one (not one who died just now). Clients bound nuzzleID.
			if (!validWeapon(playerShoot.weaponID) || !game->players[playerShoot.playerID]->weapon) return;
			if (game->players[playerShoot.playerID])
			{
				//--- Is he alive? Else we ignore it
				if (game->players[playerShoot.playerID]->status == PLAYER_STATUS_ALIVE || ( game->players[playerShoot.playerID]->status == PLAYER_STATUS_DEAD && game->players[playerShoot.playerID]->timeDead < 0.2f))
				{

					//special case with the shotty and sniper that shots 2 and 5 bullets on the same frame :(
					if( playerShoot.weaponID == WEAPON_SHOTGUN || playerShoot.weaponID == WEAPON_SNIPER )
					{
						if (game->players[playerShoot.playerID]->weapon->weaponID == WEAPON_SNIPER)
						{
							if (game->players[playerShoot.playerID]->currentCF.camPosZ >= 10.0f)
								game->players[playerShoot.playerID]->weapon->nbShot = 3;
							else
								game->players[playerShoot.playerID]->weapon->nbShot = 2;
						}
						if( game->players[playerShoot.playerID]->mfElapsedSinceLastShot + 0.061f > gameVar.weapons[playerShoot.weaponID]->fireDelay )
						{
							//we are ok to shoot our first bullet
							game->players[playerShoot.playerID]->shotCount = 1;
							game->players[playerShoot.playerID]->mfElapsedSinceLastShot = 0.0f;
							
						}
						else
						{
							if(game->players[playerShoot.playerID]->shotCount < ( playerShoot.weaponID == WEAPON_SHOTGUN ? 5 : 3))

							{
								game->players[playerShoot.playerID]->shotCount += 1;
							}
							else
							{
								return;
							}
						}

						game->shootSV(playerShoot);
						game->players[playerShoot.playerID]->firedShowDelay = 2;

					}
					else // not using a shotty or sniper ( so its SMG / DMG / CG obviously )
					{
						if ( game->players[playerShoot.playerID]->mfElapsedSinceLastShot < gameVar.weapons[playerShoot.weaponID]->fireDelay + 0.05f)
						{//used to track the number of seconds an auto has been shooting for
							game->players[playerShoot.playerID]->secondsFired += gameVar.weapons[playerShoot.weaponID]->fireDelay;
						}
						else
						{
							game->players[playerShoot.playerID]->secondsFired = 0;
						}
						if( game->players[playerShoot.playerID]->mfElapsedSinceLastShot + ( playerShoot.weaponID == WEAPON_CHAIN_GUN ? 0.04f : 0.051f ) > gameVar.weapons[playerShoot.weaponID]->fireDelay )
						{
							//we are ok to shoot our first bullet
							//game->players[playerShoot.playerID]->shotCount = 1;
							game->players[playerShoot.playerID]->mfElapsedSinceLastShot = 0.0f;
						}
						else
						{
							//if(game->players[playerShoot.playerID]->shotCount < 2)
							//{
							//	game->players[playerShoot.playerID]->shotCount += 1;
							//}
							//else
							//{
								return;
							//}
						}
				
						game->shootSV(playerShoot);
						game->players[playerShoot.playerID]->firedShowDelay = 2;
					}
				}
			}
			break;
		}
	case NET_CLSV_SVCL_PLAYER_PROJECTILE:
		{
			net_clsv_svcl_player_projectile playerProjectile;
			memcpy(&playerProjectile, buffer, sizeof(net_clsv_svcl_player_projectile));
			// Q-S2, R25: a known weapon and projectile type, from a player who holds a weapon
			if (!validWeapon(playerProjectile.weaponID) || !game->players[playerProjectile.playerID]->weapon ||
				playerProjectile.projectileType < PROJECTILE_DIRECT || playerProjectile.projectileType > PROJECTILE_PHOTON)
				return;
			if (game->players[playerProjectile.playerID])
			{
				//--- Is he alive? Else we ignore it
				if (game->players[playerProjectile.playerID]->status == PLAYER_STATUS_ALIVE || ( game->players[playerProjectile.playerID]->status == PLAYER_STATUS_DEAD
					&& (playerProjectile.weaponID == WEAPON_GRENADE || playerProjectile.weaponID == WEAPON_COCKTAIL_MOLOTOV || playerProjectile.projectileType == PROJECTILE_ROCKET ) && game->players[playerProjectile.playerID]->timeDead < 0.2f ) 
					)
				{
					if (playerProjectile.projectileType == PROJECTILE_COCKTAIL_MOLOTOV && !gameVar.sv_enableMolotov)
					{
						return;
					}
					// if its a rocket we are launching, do we have a rocket launcher in hand ?
					if( playerProjectile.projectileType == PROJECTILE_ROCKET )
					{

						//no rocket launcher in hand, dont launch anything
						if( ! (game->players[playerProjectile.playerID]->weapon->weaponID == WEAPON_BAZOOKA) )
						{
							return;
						}
						if(gameVar.sv_zookaRemoteDet && gameVar.sv_serverType == 1)
						{
							if(game->players[playerProjectile.playerID]->rocketInAir && game->players[playerProjectile.playerID]->mfElapsedSinceLastShot > 0.25f)
							{
							//Rocket already in air?  Detonate and don't launch
								game->players[playerProjectile.playerID]->detonateRocket = true;
								return;
							}
						}
					}
					
					// Ignore nades and moltov since they are separate from other weapons
					if (playerProjectile.weaponID != WEAPON_GRENADE &&
						playerProjectile.weaponID != WEAPON_COCKTAIL_MOLOTOV ) 
					{
					
						// Check if shotsPerSecond is set by Player::Update
						//if(game->players[playerProjectile.playerID]->shotsPerSecond > 0)

						// enough time has elapsed for shotting
						if( game->players[playerProjectile.playerID]->mfElapsedSinceLastShot + 0.1f > gameVar.weapons[playerProjectile.weaponID]->fireDelay )
						{
							if (!(game->spawnProjectile(playerProjectile, true)))
							{
								return;
							}
						}
						else
						{
							return;
						}
						if(playerProjectile.projectileType == PROJECTILE_ROCKET)
						{	//We're launching the rocket so flag it as in the air
							game->players[playerProjectile.playerID]->rocketInAir = true;
						}
						game->players[playerProjectile.playerID]->mfElapsedSinceLastShot = 0.0f;
					}
					else
					{
						if (!(game->spawnProjectile(playerProjectile, true)))
						{
							return;
						}
					}

					

					// On l'envoit aux autres joueurs
					bb_serverSend((char*)&playerProjectile, sizeof(net_clsv_svcl_player_projectile), NET_CLSV_SVCL_PLAYER_PROJECTILE, 0);

				}
			}
			break;
		}
	case NET_CLSV_PICKUP_REQUEST:
		{
			net_clsv_pickup_request pickupRequest;
			memcpy(&pickupRequest, buffer, sizeof(net_clsv_pickup_request));
			if (game->players[pickupRequest.playerID])
			{
				//--- Is he alive? Else we ignore it
				if (game->players[pickupRequest.playerID]->status == PLAYER_STATUS_ALIVE)
				{
					// On check s'il n'y a pas un weapon �pickuper proche
					for (int i=0;i<(int)game->projectiles.size();++i)
					{
						Projectile * projectile = game->projectiles[i];
						if (projectile->projectileType == PROJECTILE_DROPED_WEAPON && !projectile->needToBeDeleted)
						{
							float dis = distanceSquared(
								CVector3f(projectile->currentCF.position[0], projectile->currentCF.position[1], 0),
								CVector3f(game->players[pickupRequest.playerID]->currentCF.position[0],
								game->players[pickupRequest.playerID]->currentCF.position[1],0));
							if (dis <= .5f*.5f)
							{
								// On drope l'autre
								net_clsv_svcl_player_projectile playerProjectile;
								playerProjectile.nuzzleID = 0;
								playerProjectile.playerID = game->players[pickupRequest.playerID]->playerID;
								playerProjectile.position[0] = (short)(game->players[pickupRequest.playerID]->currentCF.position[0] * 100);
								playerProjectile.position[1] = (short)(game->players[pickupRequest.playerID]->currentCF.position[1] * 100);
								playerProjectile.position[2] = (short)(game->players[pickupRequest.playerID]->currentCF.position[2] * 100);
								playerProjectile.vel[0] = (char)(game->players[pickupRequest.playerID]->currentCF.vel[0] * 10);
								playerProjectile.vel[1] = (char)(game->players[pickupRequest.playerID]->currentCF.vel[1] * 10);
								playerProjectile.vel[2] = (char)(game->players[pickupRequest.playerID]->currentCF.vel[2] * 10);
								playerProjectile.weaponID = game->players[pickupRequest.playerID]->weapon->weaponID;
								playerProjectile.projectileType = PROJECTILE_DROPED_WEAPON;
							//	playerProjectile.uniqueProjectileID = ++(game->uniqueProjectileID);
								game->spawnProjectile(playerProjectile, true);
								bb_serverSend((char*)&playerProjectile, sizeof(net_clsv_svcl_player_projectile), NET_CLSV_SVCL_PLAYER_PROJECTILE, 0);

								// On pickup � !!
								game->players[pickupRequest.playerID]->switchWeapon(projectile->fromID);
								net_svcl_pickup_item pickupItem;
								pickupItem.playerID = pickupRequest.playerID;
								pickupItem.itemType = ITEM_WEAPON;
								pickupItem.itemFlag = projectile->fromID;
								bb_serverSend((char*)&pickupItem, sizeof(net_svcl_pickup_item), NET_SVCL_PICKUP_ITEM, 0);
								projectile->needToBeDeleted = true;
								break;
							}
						}
					}
				}
			}
			break;
		}
	case NET_CLSV_MAP_LIST_REQUEST:
		{
			net_clsv_map_list_request maplRequest;
			memcpy(&maplRequest, buffer, sizeof(net_clsv_map_list_request));
			maplRequest.all = buffer[offsetof(net_clsv_map_list_request, all)] != 0;
			if (game->players[maplRequest.playerID])
			{
				std::vector<CString> maps = populateMapList(maplRequest.all);
				net_svcl_map_list mapl;
				for (int i = 0; i < (int)maps.size(); i++)
				{
					memset(mapl.mapName, 0, 16);
					memcpy(mapl.mapName, maps[i].s, min(maps[i].len(), 15)); // keep the '\0' in byte 16
					bb_serverSend((char*)&mapl, sizeof(net_svcl_map_list), NET_SVCL_MAP_LIST, game->players[maplRequest.playerID]->babonetID);
				}
			}
			break;
		}
	case NET_CLSV_SVCL_PLAYER_UPDATE_SKIN:
		{
			net_clsv_svcl_player_update_skin updateSkin;
			memcpy(&updateSkin, buffer, sizeof(net_clsv_svcl_player_update_skin));
			updateSkin.skin[6] = '\0';
			// Q-S7: one broadcast reaches everyone (it was sent once per connected player)
			bb_serverSend((char*)(&updateSkin), sizeof(net_clsv_svcl_player_update_skin), NET_CLSV_SVCL_PLAYER_UPDATE_SKIN);
			break;
		}

	}
}


