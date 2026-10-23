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
#include "CMaster.h"
#include "SqlStatement.h"
#include "Console.h"
#include "GameVar.h"
#include "Scene.h"
#ifndef CONSOLE
#include "CLobby.h"
#endif
#include "md5class.h"

// Raw 16-byte MD5 digest (babonet's md5class yields hex).
static void MD5(const unsigned char* data, size_t len, unsigned char* out)
{
	std::string text((const char*)data, len);
	CMD5 md5(text.c_str());
	const char* hex = md5.getMD5Digest();
	for (int i = 0; i < 16; ++i)
	{
		unsigned int byte = 0;
		sscanf(hex + i * 2, "%2x", &byte);
		out[i] = (unsigned char)byte;
	}
}

CMaster* master = 0;
extern Scene* scene;


CMaster::CMaster()
{
	s1 = -1; s2 = -1; s3 = -1; s4 = -1;
	uniqueClientID = 0;
	m_isConnected = false;
	m_nbGameFound = 0;
	m_ping = 0;
	RunningServer = 0;

	GetMasterInfos();

	for( int i=0; i<64; i++)
	{
		BannedAnswers[i].ID = -1;
		BannedAnswers[i].Answer = -1;
	}

	clearRemoteCacheList();
}


CMaster::~CMaster()
{
	ZEVEN_SAFE_DELETE(m_ping);
	int i;
	ZEVEN_DELETE_VECTOR(messageStack, i);
	disconnectMaster();
	eraseGames();
	m_peers.clear();
}

void CMaster::UpdateDB()
{
	sqlite3 *DB=bv2::openClientDb();
	if (!DB) return;
	SqlStatement(DB, "Update LauncherSettings Set Value = ? Where Name = 'DBVersion';").bindInt(1, gameVar.db_version).run();
	SqlStatement(DB, "Update LauncherSettings Set Value = ? Where Name = 'AccountURL';").bindText(1, gameVar.db_accountServer.s).run();
	sqlite3_close(DB);
}


//
//--- Connecting the master
//
void CMaster::connectToMaster(const char* in_IP, short in_port)
{
	if (uniqueClientID)
		disconnectMaster();
	if (!in_IP || !in_IP[0])
		return; // no master configured (cl_masterServers / BV2_MASTER_SERVERS)
	uniqueClientID = bb_clientConnect(in_IP, in_port);
	m_isConnected = false;
}



//
//--- Kill it
//
void CMaster::disconnectMaster()
{
	if (uniqueClientID) bb_clientDisconnect(uniqueClientID);
	uniqueClientID = 0;
	m_isConnected = false;
	ZEVEN_SAFE_DELETE(m_ping);
}



//
//--- La row est valid� on a le ping pis toute
//
void CMaster::pingReceived(int ping)
{
	//--- We remove the ping object
	ZEVEN_SAFE_DELETE(m_ping);

	//--- Show that in console
	//if (gameVar.c_debug) console->add(CString("\x3> Ping received : %i", ping));

	if (m_games.size() > 0)
	{
		SBrowsableGame* browsableGame = new SBrowsableGame();
		//--- Add that BV2 to the GUIs
		browsableGame->bv2Row = new stBV2row();
		memcpy(browsableGame->bv2Row, m_games[0], sizeof(stBV2row));
		browsableGame->ping = ping;
		
#ifndef CONSOLE
		//--- Push it into the lobby
		if (lobby) lobby->pushGame(browsableGame);
#endif

		//--- Remove that row
		m_games.erase(m_games.begin());

		// Keep the master session open for the next refresh; reconnecting on
		// every listing caused connect/disconnect storms.
	}
}



//
//--- Updating the master client
//
void CMaster::update(float in_delay)
{
	//--- Update peer connection
	bool isNew = false; // bb_peerUpdate doesn't set it on every path (UBSan)
	int result = bb_peerUpdate(in_delay, isNew);
	if (result == -1)
	{
		if (gameVar.c_debug) console->add(CString("\x4> Error in peerUpdate : %s",  bb_peerGetLastError()));
	}
	else if (isNew)
	{
		// a new peer!
		if( result )
		{
			m_peers.push_back( SPeer(result) );
		}
	}
	else
	{
		//--- Ping result
		if (m_ping)
		{
			int ping = m_ping->pingResult(result);
			if (ping != -1)
			{
				pingReceived(ping);
			}
		}

		//remove this peer
		for( unsigned int i=0; i<m_peers.size(); i++ )
		{
			if( m_peers[i].peerId == result )
			{
				m_peers.erase( m_peers.begin() + i );
				bb_peerDelete( result, true );
				break;
			}
		}
	}

	//receive packets from peers (LAN browser broadcast)
	ReceivePeersPacket();

	//--- Update le ping
	if (m_ping) 
	{
		int ping = m_ping->update(in_delay);

		if (ping != -1)
		{
			pingReceived(ping);
		}
	}

	// On update le client
	if (uniqueClientID > 0)
	{
		int result = bb_clientUpdate(uniqueClientID, in_delay, UPDATE_SEND_RECV);

		if (result == 1)
		{
			// Une erreur !!! On arr�e tout !!!
			if (gameVar.c_debug) console->add(CString("\x4> Error connection to master : %s", bb_clientGetLastError(uniqueClientID)));
			bb_clientDisconnect(uniqueClientID);
			uniqueClientID = 0;
			m_isConnected = false;
			ZEVEN_SAFE_DELETE(m_ping);
		}
		else if (result == 2)
		{
			// Le server a foutu le CAMP !
			//if (gameVar.c_debug) console->add("\x9> Master server disconnected");
			bb_clientDisconnect(uniqueClientID);
			uniqueClientID = 0;
			m_isConnected = false;
			ZEVEN_SAFE_DELETE(m_ping);
		}
		else if (result == 3)
		{
			//if (gameVar.c_debug) console->add("\x3> Master server connected");

			//--- We are connected.
			m_isConnected = true;
		}
	}


	// check if we have gam einfos to send to the master
	if( m_isConnected && uniqueClientID && messageStack.size() > 0 )
	{
		for (std::vector<SMasterMessage*>::iterator it = messageStack.begin(); 
			it != messageStack.end(); ++it)
		{
			bb_clientSend(uniqueClientID, (*it)->buffer, (*it)->size, (*it)->typeID);
			//if (gameVar.c_debug) console->add("\x3> Packet sent to master");
		}

		//--- Delete the stack
		int i;
		ZEVEN_DELETE_VECTOR(messageStack, i);
	}


	if (uniqueClientID > 0)
	{
		// On recv les messages
		char * buffer;
		int messageID;
		while ((buffer = bb_clientReceive(uniqueClientID, &messageID)))
		{
			// On g�e les messages re�
			recvPacket(buffer, messageID);
		}
	}

	//--- We check if we have a new server to ping
	if (!m_ping && m_games.size() > 0)
	{
		stBV2row * bv2Row = m_games[0];

		m_ping = new CPing();
		if (m_ping->ping(CString("%s", bv2Row->ip), bv2Row->port) == -1)
		{
			// Can't UDP-ping this host (empty IP, ...): list it anyway.
			delete m_ping;
			m_ping = 0;
			pingReceived(1000);
		}
	}

	// peer timeouts
	for( unsigned int z=0; z<m_peers.size(); z++ )
	{
		m_peers[z].timeout -= in_delay;
		if( m_peers[z].timeout < 0.0f )
		{
			bb_peerDelete( m_peers[z].peerId, true );
			m_peers.erase( m_peers.begin() + z );
			z--;
		}
	}
}


// get the broadcast/remote admin data
void CMaster::ReceivePeersPacket()
{
	char *buf=0;
	INT4 peerID;
	int typeID;

	while(( buf = bb_peerReceive( &peerID, &typeID )))
	{		
		switch( typeID )
		{
			case NET_BROADCAST_GAME_INFO:
			{
				net_svcl_broadcast_game_info lanGame;

				memcpy( &lanGame , buf , sizeof(net_svcl_broadcast_game_info));
				
				if( !stricmp( lanGame.key , "3333113333" ) )
				{

					SBrowsableGame* browsableGame = new SBrowsableGame();
					//--- Add that BV2 to the GUIs
					browsableGame->bv2Row = new stBV2row();

					
					memcpy(browsableGame->bv2Row, &(lanGame.GameInfo) , sizeof(stBV2row));
					browsableGame->ping = 0;
			
					#ifndef CONSOLE
						//--- Push it into the lobby
						if (lobby) lobby->pushGame(browsableGame);
					#endif
				}

				break;
			}

			// we receive a lan udp broacast query
			case NET_BROADCAST_QUERY:
			{
				if( !RunningServer ) break;


				net_clsv_broadcast_query query;
				memcpy( &query , buf , sizeof(net_clsv_broadcast_query));

				if( !stricmp( query.key , "3333113333" ) )
				{
					//we receive a query, lets send a broadcast with our infos
					net_svcl_broadcast_game_info gameInfos;

					sprintf( gameInfos.key , "3333113333" );
					
					
					
					strncpy(gameInfos.GameInfo.map, RunningServer->game->mapName.s, 16);

					char srvName[63];
					sprintf( srvName , "{ LAN } %s", gameVar.sv_gameName.s );
					strncpy(gameInfos.GameInfo.serverName, srvName, 63);
					strcpy(gameInfos.GameInfo.password, gameVar.sv_password != "" ? "*" : ""); // a flag: never the password (Q-S5)
					sprintf( gameInfos.GameInfo.ip , "%s", bb_getMyIP() );
					gameInfos.GameInfo.port = (unsigned short)gameVar.sv_port;
					int nbPlayer = 0;
					for (int i=0;i<MAX_PLAYER;++i)
					{
						if (RunningServer->game->players[i])
						{
							nbPlayer++;
						}
					}
					gameInfos.GameInfo.nbPlayer = nbPlayer;
					gameInfos.GameInfo.maxPlayer = gameVar.sv_maxPlayer;
					gameInfos.GameInfo.flags = 0;
					gameInfos.GameInfo.gameType = RunningServer->game->gameType;
					gameInfos.GameInfo.ServerID = 0;
					gameInfos.GameInfo.Priority = 0;

					//send the packet
					bb_peerSend( NET_BROADCAST , (char*) &gameInfos , NET_BROADCAST_GAME_INFO , sizeof(net_svcl_broadcast_game_info) , false );

				}

				break;
			}
		}
	}

}


//
//--- On efface les games
//
void CMaster::eraseGames()
{
	for (int i=0;i<(int)m_games.size();++i)
	{
		delete m_games[i];
	}
	m_games.clear();
}

//
//--- To handle the incomming packets
//
void CMaster::recvPacket(const char * buffer, int typeID)
{
	switch (typeID)
	{
	case MASTER_INFO:
		{
#ifndef CONSOLE
			if (lobby) lobby->clearLobby();
#endif
			eraseGames();
			stMasterInfo masterInfo;
			memcpy(&masterInfo, buffer, sizeof(stMasterInfo));
			if (masterInfo.NbGames == 0)
			{
				m_nbGameFound = 0;
				console->add(CString("\x3> No games found"));
			}
			else if (masterInfo.NbGames == -1)
			{
				m_nbGameFound = 0;
				disconnectMaster();
				console->add(CString("\x4> Outdated version"));
			}
			else
			{
				m_nbGameFound = masterInfo.NbGames;
				console->add(CString("\x3> Games found : %i", m_nbGameFound));
			}
			break;
		}
	case BV2_ROW:
		{
			stBV2row* bv2Row = new stBV2row();
			memcpy(bv2Row, buffer, sizeof(stBV2row));
			m_games.push_back(bv2Row);

			//--- On check si on a pas fini
			//if ((int)m_games.size() >= m_nbGameFound)
			//{
			//	disconnectMaster();
			//}

			if (gameVar.c_debug) console->add(CString("\x9> Received game %i/%i", (int)m_games.size(), m_nbGameFound));
			break;
		}
	case CACHE_ANSWER:
		{	
			stCacheAnswer ca;
			memcpy(&ca,buffer,sizeof(stCacheAnswer));
	
			//add the answer to our list of cached answer for the server to check them later.
			for( int i=0; i<64; i++ )
			{
				if( BannedAnswers[i].ID == -1 )
				{
					BannedAnswers[i].ID			= ca.ID;
					BannedAnswers[i].Answer		= ca.Answer;
					return;
				}
			}

			break;
		}
	case CACHE_BAN:
		{
			stCacheBan cb;
			memcpy(&cb,buffer,sizeof(stCacheBan));

			console->add(CString("[%02i] %s - IP:%s  Date:%s  Duration:%i", cb.ID,
						cb.Nick,
						cb.IP,
						cb.Date,
						cb.Duration), true);

			break;
		}
	case CACHE_LIST_PLAYER:
		{
			stCachePlayer cp;
			memcpy(&cp,buffer,sizeof(stCachePlayer));

			if (cachePlayerRemoteServer(cp))
				console->add(CString("[%02i] %s \x8- %s", cp.ID, cp.NickName, cp.IP), true);

			break;
		}
	case CACHE_LIST_REMOTE:
		// Ignored: it asked for our recent players' nicks and IPs for another server's admin (ADR 0034)
		break;
	case ACCOUNT_URL:
		{
			unsigned short version=0;
			memcpy(&version,buffer,2);
			gameVar.db_version = version;
			gameVar.db_accountServer = CString((char*)buffer + 2);

			// update the db
			UpdateDB();
			
			break;
		}
	case CLIENT_HASH:
		{
			stHashSeed hs;
			memcpy(&hs,buffer,sizeof(stHashSeed));
			master->s1 = hs.s1;
			master->s2 = hs.s2;
			master->s3 = hs.s3;
			master->s4 = hs.s4;
			console->add(CString("hash seed: %d %d %d %d", hs.s1, hs.s2, hs.s3, hs.s4));
			return;
		}
	default:
		{
			console->add("Unhandled message by CMaster");
			break;
		}
	}
}



//
//--- Shooter l'info de la game
//
void CMaster::sendGameInfo(Server* server)
{
	if (server->game)
	{
		// Replace only the previous BV2_ROW heartbeat; other queued master packets
		// may still be waiting for the TCP session to come up.
		for (std::vector<SMasterMessage*>::iterator it = messageStack.begin(); it != messageStack.end(); )
		{
			if ((*it)->typeID == BV2_ROW)
			{
				delete *it;
				it = messageStack.erase(it);
			}
			else
				++it;
		}
		int i;

		stBV2row bv2Row;

		strncpy(bv2Row.map, server->game->mapName.s, 16);
		strncpy(bv2Row.serverName, gameVar.sv_gameName.s, 63);
		strcpy(bv2Row.password, gameVar.sv_password != "" ? "*" : ""); // a flag: never the password (Q-S5)
		snprintf(bv2Row.ip, sizeof(bv2Row.ip), "%s", gameVar.sv_publicAddress.s); // the master trusts it only from a private source
		bv2Row.port = (unsigned short)gameVar.sv_port;
		int nbPlayer = 0;
		for (i=0;i<MAX_PLAYER;++i)
		{
			if (server->game->players[i])
			{
				nbPlayer++;
			}
		}
		bv2Row.nbPlayer = nbPlayer;
		bv2Row.maxPlayer = gameVar.sv_maxPlayer;
		bv2Row.flags = 0;
		bv2Row.gameType = server->game->gameType;
		bv2Row.ServerID = 0;
		bv2Row.Priority = 0;
		bv2Row.DBVersion = (unsigned short)gameVar.db_version;

		sendPacket((char*)(&bv2Row), sizeof(stBV2row), BV2_ROW);
	}
}




//
//--- Requester la game
//
void CMaster::requestGames()
{
	#ifndef CONSOLE
		//clear lobby
		if( lobby ) lobby->clearLobby();
	#endif

	//clear the game stack (and a ping still pending from the last refresh)
	ZEVEN_SAFE_DELETE(m_ping);
	eraseGames();
	m_nbGameFound = 0;

	stBV2list bv2List;
	strcpy(bv2List.Version, m_CurrentVersion );
	sendPacket((char*)(&bv2List), sizeof(stBV2list), BV2_LIST);

	//send a udp broadcast
	net_clsv_broadcast_query query;
	sprintf( query.key , "3333113333");
	bb_peerSend( NET_BROADCAST , (char*) &query , NET_BROADCAST_QUERY , sizeof( net_clsv_broadcast_query ) , false );


}

//
//--- Get Master Server IP / Port and our current game version from the database
//
void CMaster::GetMasterInfos()
{
	// Defaults when bv2.db or its rows are missing: no master, version 2.11.
	m_IP[0] = '\0';
	m_Port = 0;
	snprintf(m_CurrentVersion, sizeof(m_CurrentVersion), "%s", "2.11");

	// Master servers come from the config (cl_masterServers / BV2_MASTER_SERVERS); none means no master list
	std::vector<bv2::HostPort> masters = bv2::parseHostPorts(gameVar.cl_masterServers.s);
	if (!masters.empty())
	{
		snprintf(m_IP, sizeof(m_IP), "%s", masters[0].host.c_str());
		m_Port = (short)masters[0].port;
	}

	sqlite3 *db = bv2::openClientDb();
	if (!db)
	{
		console->add("Could not open the user database");
		return;
	}
	// get our current game version
	std::string version = SqlStatement::value(db, "Select Value From LauncherSettings Where Name = 'Version';");
	if (!version.empty())
		snprintf(m_CurrentVersion, sizeof(m_CurrentVersion), "%s", version.c_str());
	sqlite3_close( db );
}


//
//--- Send message to master (If not connected, it will try open a connection)
//
void CMaster::sendPacket(const char* in_data, int in_size, int in_ID, bool disconnectAfter)
{

	//--- Are we connected?
	if (!uniqueClientID)
	{
		//--- Connect us first
		connectToMaster( m_IP , m_Port );
		console->add(" master needed to connect ");
	}

	// add the message to the queue
	if( messageStack.size() < 1 || (in_ID >= 1010 && in_ID <= 1013) || in_ID == 1016 || in_ID == 1017)
	{
		messageStack.push_back(new SMasterMessage(in_data, in_size, in_ID));
	}
}

bool CMaster::cachePlayerRemoteServer(const stCachePlayer& player)
{
	if (player.ID < 0 || player.ID >= 50 || player.ReqNum != cacheListReqNum)
		return false;

	CachedPlayersRemote[player.ID].Valid = true;
	sprintf( CachedPlayersRemote[player.ID].IP , "%s", player.IP );
	sprintf( CachedPlayersRemote[player.ID].NickName , "%s", player.NickName );
	return true;
}

void CMaster::clearRemoteCacheList()
{
	for( int i = 0; i < 50; i++ )
	{
		CachedPlayersRemote[i].Valid = false;
		CachedPlayersRemote[i].NickName[0] = '\0';
		CachedPlayersRemote[i].IP[0] = '\0';
	}
}

void CMaster::requestRemoteCacheList(CString& strFilter, CString& serverIP, CString& serverPort, unsigned long baboid)
{
	if (serverIP.len() > 15)
	{
		console->add("\x3> Invalid IP address");
		return;
	}
	if (serverPort.len() > 5)
	{
		console->add("\x3> Invalid port");
		return;
	}
	cacheListReqNum++;
	clearRemoteCacheList();
	stCacheListRemote clr;
	memset(&clr, 0, sizeof(stCacheListRemote));
	clr.ReqNum = cacheListReqNum;
	clr.FromID = baboid;
	sprintf( clr.Filter , "%s", strFilter.s );
	sprintf( clr.ServerIP , "%s", serverIP.s );
	sprintf( clr.ServerPort , "%s", serverPort.s );
	
	sendPacket( (char*)&clr , sizeof(stCacheListRemote) , CACHE_LIST_REMOTE );
}

bool CMaster::hasValidHashSeed()
{
	if (s1 == -1 || s2 == -1 || s3 == -1 || s4 == -1)
		return false;
	else
		return true;
}

void CMaster::HashPass(char* pass)
{
	int* tmp = (int*)pass;
	tmp[0] = tmp[0] ^ (int)s1;
	tmp[1] = tmp[1] ^ (int)s2;
	tmp[2] = tmp[2] ^ (int)s3;
	tmp[3] = tmp[3] ^ (int)s4;
}

bool AccountManagerClient::createAccount(char* login, char* password, char* nick, char* email)
{
	if ((short)strlen(login) > loginMaxLength || (short)strlen(password) > passMaxLength ||
		(short)strlen(nick) > nickMaxLength || (short)strlen(email) > emailMaxLength || !master->hasValidHashSeed())
		return false;

	unsigned char output[16];
	MD5((unsigned char*)password, strlen(password), output);
	master->HashPass((char*)output);

	stAccount ac;
	memset(&ac, 0, sizeof(stAccount));
	sprintf( ac.Login , "%s", login );
	strncpy( ac.HashedPassMD5, (char*)output, 16 );
	sprintf( ac.Nick , "%s", nick );
	sprintf( ac.Email , "%s", email );
	master->sendPacket( (char*)&ac , sizeof(stAccount) , CREATE_ACCOUNT );

	return true;
}

bool AccountManagerClient::deleteAccount(int userID, char* password)
{
	if ((short)strlen(password) > passMaxLength || !master->hasValidHashSeed())
		return false;

	unsigned char output[16];
	MD5((unsigned char*)password, strlen(password), output);
	master->HashPass((char*)output);

	stDeleteAccount da;
	memset(&da, 0, sizeof(stDeleteAccount));
	da.UserID = userID;
	strncpy( da.HashedPassMD5, (char*)output, 16 );
	master->sendPacket( (char*)&da , sizeof(stDeleteAccount) , DELETE_ACCOUNT );
	return true;
}

bool AccountManagerClient::updateAccount(char* login, char* password, char* nick, char* email)
{
	if ((short)strlen(login) > loginMaxLength || (short)strlen(password) > passMaxLength ||
		(short)strlen(nick) > nickMaxLength || (short)strlen(email) > emailMaxLength || !master->hasValidHashSeed())
		return false;

	unsigned char output[16];
	MD5((unsigned char*)password, strlen(password), output);
	master->HashPass((char*)output);

	stAccount ac;
	memset(&ac, 0, sizeof(stAccount));
	sprintf( ac.Login , "%s", login );
	strncpy( ac.HashedPassMD5, (char*)output, 16 );
	sprintf( ac.Nick , "%s", nick );
	sprintf( ac.Email , "%s", email );
	master->sendPacket( (char*)&ac , sizeof(stAccount) , UPDATE_ACCOUNT );
	return true;
}

bool AccountManagerClient::changePassword(int userID, char* oldPass, char* newPass)
{
	if ((short)strlen(oldPass) > passMaxLength || (short)strlen(newPass) > passMaxLength || !master->hasValidHashSeed())
		return false;

	stChangePassword cp;
	memset(&cp, 0, sizeof(stChangePassword));

	unsigned char outputOld[32];
	MD5((unsigned char*)oldPass, strlen(oldPass), outputOld);
	master->HashPass((char*)outputOld);
	unsigned char outputNew[32];
	MD5((unsigned char*)newPass, strlen(newPass), outputNew);
	master->HashPass((char*)outputNew);

	cp.UserID = userID;
	memcpy( cp.HashedOldPassMD5, outputOld, 16 ); // the first 16 hash characters, no '\0' (as strncpy did)
	memcpy( cp.HashedNewPassMD5, outputNew, 16 );
	master->sendPacket( (char*)&cp , sizeof(stChangePassword) , CHANGE_PASSWORD );
	return true;
}

bool AccountManagerClient::recoverPassword(char* login)
{
	if ((short)strlen(login) > loginMaxLength)
		return false;

	stRecoverPassword rp;
	memset(&rp, 0, sizeof(stRecoverPassword));
	sprintf( rp.Login , "%s", login );
	master->sendPacket( (char*)&rp , sizeof(stRecoverPassword) , RECOVER_PASSWORD );
	return true;
}

bool AccountManagerClient::loginAccount(char* login, char* password)
{
	if ((short)strlen(login) > loginMaxLength || (short)strlen(password) > passMaxLength || !master->hasValidHashSeed())
		return false;

	unsigned char output[16];
	MD5((unsigned char*)password, strlen(password), output);
	master->HashPass((char*)output);

	stLoginLogout da;
	memset(&da, 0, sizeof(stLoginLogout));
	sprintf( da.Login , "%s", login );
	strncpy( da.HashedPassMD5, (char*)output, 16 );
	master->sendPacket( (char*)&da , sizeof(stLoginLogout) , LOGIN_ACCOUNT );
	return true;
}

bool AccountManagerClient::logoutAccount(char* login, char* password)
{
	if ((short)strlen(login) > loginMaxLength || (short)strlen(password) > passMaxLength || !master->hasValidHashSeed())
		return false;

	unsigned char output[16];
	MD5((unsigned char*)password, strlen(password), output);
	master->HashPass((char*)output);

	stLoginLogout da;
	memset(&da, 0, sizeof(stLoginLogout));
	sprintf( da.Login , "%s", login );
	strncpy( da.HashedPassMD5, (char*)output, 16 );
	master->sendPacket( (char*)&da , sizeof(stLoginLogout) , LOGOUT_ACCOUNT );
	return true;
}

bool AccountManagerClient::userStatusUpdate(int userID, char* password, char* serverName, char* ip, short port)
{
	if ((short)strlen(password) > passMaxLength || (short)strlen(serverName) > 64 || strlen(ip) > 16 || !master->hasValidHashSeed())
		return false;

	stUserStatusUpdate su;
	memset(&su, 0, sizeof(stUserStatusUpdate));

	unsigned char output[16];
	MD5((unsigned char*)password, strlen(password), output);
	master->HashPass((char*)output);

	su.UserID = userID;
	strncpy( su.HashedPassMD5, (char*)output, 16 );
	sprintf( su.ServerName , "%s", serverName );
	sprintf( su.IP , "%s", ip );
	su.Port = port;
	master->sendPacket( (char*)&su , sizeof(stUserStatusUpdate) , USER_STATUS_UPDATE );
	return true;
}

bool AccountManagerClient::registerClan(int userID, char* password, char* name, char* tag, char* email, char* website)
{
	if ((short)strlen(password) > passMaxLength || (short)strlen(name) > clanNameMaxLength ||
		(short)strlen(tag) > clanTagMaxLength || (short)strlen(website) > websiteMaxLength || !master->hasValidHashSeed())
		return false;

	stRegisterClan data;
	memset(&data, 0, sizeof(stRegisterClan));

	unsigned char output[16];
	MD5((unsigned char*)password, strlen(password), output);
	master->HashPass((char*)output);

	data.UserID = userID;
	strncpy( data.HashedPassMD5, (char*)output, 16 );
	sprintf( data.Name , "%s", name );
	sprintf( data.Tag , "%s", tag );
	sprintf( data.Email , "%s", email );
	sprintf( data.Website , "%s", website );

	master->sendPacket( (char*)&data , sizeof(stRegisterClan) , REGISTER_CLAN );
	return true;
}

bool AccountManagerClient::removeClan(int userID, char* password)
{
	if ((short)strlen(password) > passMaxLength || !master->hasValidHashSeed())
		return false;

	stRemoveClan data;
	memset(&data, 0, sizeof(stRemoveClan));

	unsigned char output[16];
	MD5((unsigned char*)password, strlen(password), output);
	master->HashPass((char*)output);

	data.UserID = userID;
	strncpy( data.HashedPassMD5, (char*)output, 16 );
	master->sendPacket( (char*)&data , sizeof(stRemoveClan) , REMOVE_CLAN );
	return true;
}

bool AccountManagerClient::changeClanPermissions(int userID, char* password, int userIDChanging, char permissions)
{
	if ((short)strlen(password) > passMaxLength || !master->hasValidHashSeed())
		return false;

	stChangeClanPermissions data;
	memset(&data, 0, sizeof(stChangeClanPermissions));

	unsigned char output[16];
	MD5((unsigned char*)password, strlen(password), output);
	master->HashPass((char*)output);

	data.UserID = userID;
	strncpy( data.HashedPassMD5, (char*)output, 16 );
	data.DestUserID = userIDChanging;
	data.Permissions = permissions;
	master->sendPacket( (char*)&data , sizeof(stChangeClanPermissions) , CHANGE_CLAN_PERMISSIONS );
	return true;
}

bool AccountManagerClient::joinClanRequest(int userID, char* password, int userIDDest)
{
	if ((short)strlen(password) > passMaxLength || !master->hasValidHashSeed())
		return false;

	stJoinClanRequest data;
	memset(&data, 0, sizeof(stJoinClanRequest));

	unsigned char output[16];
	MD5((unsigned char*)password, strlen(password), output);
	master->HashPass((char*)output);

	data.UserID = userID;
	strncpy( data.HashedPassMD5, (char*)output, 16 );
	data.DestUserID = userIDDest;
	master->sendPacket( (char*)&data , sizeof(stJoinClanRequest) , JOIN_CLAN_REQUEST );
	return true;
}

bool AccountManagerClient::joinClanAccept(int userID, char* password, int userIDFrom)
{
	if ((short)strlen(password) > passMaxLength || !master->hasValidHashSeed())
		return false;

	stJoinClanAccept data;
	memset(&data, 0, sizeof(stJoinClanAccept));

	unsigned char output[16];
	MD5((unsigned char*)password, strlen(password), output);
	master->HashPass((char*)output);

	data.UserID = userID;
	strncpy( data.HashedPassMD5, (char*)output, 16 );
	data.UserIDFrom = userIDFrom;
	master->sendPacket( (char*)&data , sizeof(stJoinClanAccept) , JOIN_CLAN_ACCEPT );
	return true;
}

bool AccountManagerClient::leaveClan(int userID, char* password)
{
	if ((short)strlen(password) > passMaxLength || !master->hasValidHashSeed())
		return false;

	stLeaveClan data;
	memset(&data, 0, sizeof(stLeaveClan));

	unsigned char output[16];
	MD5((unsigned char*)password, strlen(password), output);
	master->HashPass((char*)output);

	data.UserID = userID;
	strncpy( data.HashedPassMD5, (char*)output, 16 );
	master->sendPacket( (char*)&data , sizeof(stLeaveClan) , LEAVE_CLAN );
	return true;
}

bool AccountManagerClient::requestFriend(int userID, char* password, int userIDDest, char* groupName)
{
	if ((short)strlen(password) > passMaxLength || !master->hasValidHashSeed())
		return false;

	stFriendOp data;
	memset(&data, 0, sizeof(stFriendOp));

	unsigned char output[16];
	MD5((unsigned char*)password, strlen(password), output);
	master->HashPass((char*)output);

	data.UserID = userID;
	strncpy( data.HashedPassMD5, (char*)output, 16 );
	data.DestUserID = userIDDest;
	strncpy( data.GroupName, groupName, 49 );
	master->sendPacket( (char*)&data , sizeof(stFriendOp) , REQUEST_FRIEND );
	return true;
}

bool AccountManagerClient::acceptFriend(int userID, char* password, int userIDDest, char* groupName)
{
	if ((short)strlen(password) > passMaxLength || !master->hasValidHashSeed())
		return false;

	stFriendOp data;
	memset(&data, 0, sizeof(stFriendOp));

	unsigned char output[16];
	MD5((unsigned char*)password, strlen(password), output);
	master->HashPass((char*)output);

	data.UserID = userID;
	strncpy( data.HashedPassMD5, (char*)output, 16 );
	data.DestUserID = userIDDest;
	strncpy( data.GroupName, groupName, 49 );
	master->sendPacket( (char*)&data , sizeof(stFriendOp) , ACCEPT_FRIEND );
	return true;
}

bool AccountManagerClient::removeFriend(int userID, char* password, int userIDDest)
{
	if ((short)strlen(password) > passMaxLength || !master->hasValidHashSeed())
		return false;

	stFriendOp data;
	memset(&data, 0, sizeof(stFriendOp));

	unsigned char output[16];
	MD5((unsigned char*)password, strlen(password), output);
	master->HashPass((char*)output);

	data.UserID = userID;
	strncpy( data.HashedPassMD5, (char*)output, 16 );
	data.DestUserID = userIDDest;
	master->sendPacket( (char*)&data , sizeof(stFriendOp) , REMOVE_FRIEND );
	return true;
}

bool AccountManagerClient::moveFriend(int userID, char* password, int userIDDest, char* groupName)
{
	if ((short)strlen(password) > passMaxLength || !master->hasValidHashSeed())
		return false;

	stFriendOp data;
	memset(&data, 0, sizeof(stFriendOp));

	unsigned char output[16];
	MD5((unsigned char*)password, strlen(password), output);
	master->HashPass((char*)output);

	data.UserID = userID;
	strncpy( data.HashedPassMD5, (char*)output, 16 );
	data.DestUserID = userIDDest;
	strncpy( data.GroupName, groupName, 49 );
	master->sendPacket( (char*)&data , sizeof(stFriendOp) , MOVE_FRIEND );
	return true;
}

bool AccountManagerClient::statsUpdate()
{
	stStatsUpdate data;
	memset(&data, 0, sizeof(stStatsUpdate));
	master->sendPacket( (char*)&data , sizeof(stStatsUpdate) , STATS_UPDATE );
	return true;
}

