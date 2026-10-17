#include "cMasterServer.h"
#include "SqlStatement.h"


#ifdef BV2_PLATFORM_WINDOWS
	#include <windows.h>
	#include <time.h>


	void GetDate( char * dateOUT )
	{
		SYSTEMTIME st;
        	GetSystemTime(&st);

		sprintf( dateOUT , "%d/%d/%d", st.wDay , st.wMonth , st.wYear );
	}


#else
	#include <time.h>

	long GetDateLong()
	{
		time_t rawtime;
  		time ( &rawtime );

		return (long) rawtime;
	}

	void GetDateString( char * dateOUT , long dateLong)
	{

  		struct tm * timeinfo;
  		timeinfo = localtime ( &dateLong );
		
		sprintf( dateOUT , "%d/%d/%d", timeinfo->tm_mday , timeinfo->tm_mon , timeinfo->tm_year - 100 );
	}

	int GetDayDiff( time_t time1 , time_t time2 )
	{
		double diff = difftime ( time2 , time1 );
		
		return (int)( diff / 86400 );	
	}

#endif


#define DBPATH "master.db"
#define WEBDBPATH "web.db"

cMasterServer::cMasterServer()
{
	//Reset ClientList
	Clients =	0;
	NbGames	=	0;
	Games	=	0;

	//on va se connecter a la Master Database
 	int rc = sqlite3_open(DBPATH,&MasterDB);
 	
 	if(rc)
 	{
 		perror("Open Database");	
 	}
 	else
 	{
 		printf("Database opened successfully\n");
 	}


	// connect to web db
	rc = sqlite3_open(WEBDBPATH,&m_webDB);
 	if(rc)
 	{
 		perror("Open web Database");	
		m_webDB = 0;
 	}
 	else
 	{
		// clear game list
		SqlStatement(m_webDB, "Delete from Games;").run();
 		printf("Web Database opened successfully\n");
 	}

	TimeSinceBanCheck = 0.0f;
	
	//on indique la version courrante de bv2
	sprintf(Version,"2.09");
}

void cMasterServer::SendBanList( UINT4 fromID , char * filter )
{
	// The filter matches nicks containing it; it is bound, so '%' or '_' in it only widen the match
	SqlStatement bans(MasterDB, "Select Date, Duration, ID, IP, Nick From BanList Where Nick LIKE '%' || ? || '%' order by ID;");
	bans.bindText(1, filter);
	while (bans.row())
	{
		stCacheBan ban;
		memset(&ban, 0, sizeof(ban));
		long dateLong = (long)bans.integer(0);
#ifndef BV2_PLATFORM_WINDOWS
		GetDateString( ban.Date , dateLong );
#endif
		ban.Duration	= (char)bans.integer(1);
		ban.ID			= (short)bans.integer(2);
		snprintf( ban.IP , sizeof(ban.IP) , "%s" , bans.text(3).c_str() );
		snprintf( ban.Nick, sizeof(ban.Nick), "%s" , bans.text(4).c_str() );
		bb_serverSend( (char*)&ban, sizeof(stCacheBan) , CACHE_BAN , fromID );
	}
}

// The ID of a ban on this IP, -1 if none (bans are by IP until accounts exist, PNS-14)
long long cMasterServer::FindBan( const char * IP )
{
	SqlStatement ban(MasterDB, "Select ID From BanList where IP = ?;");
	ban.bindText(1, IP);
	return ban.row() ? ban.integer(0) : -1;
}

int cMasterServer::IsBanned( char * IP )
{
	return FindBan( IP ) >= 0 ? 1 : 0;
}

// Writes a game to the web database's Games table (names and maps are bound: stored as sent)
void cMasterServer::WriteWebGame( const stBV2row & game , bool insert )
{
	if( !m_webDB ) return;
	SqlStatement(m_webDB, insert
		? "Insert into Games(Map,MaxPlayers,Name,NbPlayers,Password,IP,Port) Values(?,?,?,?,?,?,?);"
		: "Update Games Set Map = ?, MaxPlayers = ?, Name = ?, NbPlayers = ?, Password = ? Where IP = ? and Port = ?;")
		.bindText(1, game.map).bindInt(2, game.maxPlayer).bindText(3, game.serverName).bindInt(4, game.nbPlayer)
		.bindInt(5, game.password[0] != '\0' ? 1 : 0).bindText(6, game.ip).bindInt(7, game.port).run();
}

void cMasterServer::AddBan( char * NickName , char * IP , int Duration )
{
	//filter ip adress
	if( strstr( "192.168", IP ))
	{
		//canoot ban lan adress
		return;
	}

	if( strstr( "127.0.0.1", IP ))
	{
		//canoot ban local loopback adress
		return;
	}

	//get current time
	time_t theTime;
	theTime = time( NULL );

	long long existing = FindBan( IP );

	if( existing >= 0 )
	{
		// just update current row
		SqlStatement(MasterDB, "Update BanList Set Date = ?, Nick = ?, IP = ?, Duration = ? Where ID = ?;")
			.bindInt(1, (long long)theTime).bindText(2, NickName).bindText(3, IP).bindInt(4, Duration).bindInt(5, existing).run();
		printf("Updated ban with ID %lli\n", existing);
	}
	else
	{
		// add new ban
		SqlStatement(MasterDB, "Insert into BanList ( Date , Duration , IP , Nick ) values( ?, ?, ?, ? );")
			.bindInt(1, (long long)theTime).bindInt(2, Duration).bindText(3, IP).bindText(4, NickName).run();
		printf("Added ban : IP=%s  NickName=%s\n",IP,NickName);
	}
}

void cMasterServer::Unban( int ID )
{
	//let's delete the specified ID from the DB
	SqlStatement(MasterDB, "Delete From BanList Where ID = ?;").bindInt(1, ID).run();
	printf("Ban with ID %i has been removed\n", ID);
}

void cMasterServer::CheckBans()
{
	//get current time
	time_t theTime;
	theTime = time( NULL );

	SqlStatement bans(MasterDB, "Select Date, Duration, ID From BanList order by ID;");
	while (bans.row())
	{
		long dateLong = (long)bans.integer(0);
		int Duration = (int)bans.integer(1);

		// unlimited ban
		if( !Duration )
		{
			continue;
		}

#ifndef BV2_PLATFORM_WINDOWS
		int nbDays = GetDayDiff( dateLong , theTime );
#else
		int nbDays = 0;
		(void)dateLong;
#endif

		// we can remove the ban
		if( nbDays >= Duration )
			SqlStatement(MasterDB, "Delete From BanList Where ID = ?;").bindInt(1, bans.integer(2)).run();
	}
}

int cMasterServer::Update(float elapsed)
{
	TimeSinceBanCheck += elapsed;

	char	IP[16] = { 0 };
	bool	isNew=false;
	long r = bb_serverUpdate(elapsed,UPDATE_SEND_RECV,IP);

	//un nouveau client s'est connecter
	if(r > 0)
	{
		//printf("new client ID %li\n",r);
		AddClient(r,IP);	//adding a client
		printf("new client\n");
	}
	else if(r==BBNET_ERROR) //une erreur c produite
	{
		printf("error on bb_serverUpdate\n");
		return 1;
	}
	else if(r<0)
	{
		printf("client ID %li disconnected\n",r*-1);
		RemoveClient(r*-1);
	}

	//on va updater les query des clients
	for(MasterClient *C=Clients;C;C=C->Next)
	{
		if(C->Update( elapsed ))
		{
			//client timed out
			bb_serverDisconnectClient( C->BabonetID );
			RemoveClient(C->BabonetID);
		
			break;
		}
	}

	for(cBV2game *G=Games;G;G=G->Next)
	{
		if(G->Update( elapsed ))
		{
			//game timed out, remove it form the list
			
			if(G->Next)	G->Next->Previous = G->Previous;
			if(G->Previous)	G->Previous->Next = G->Next;

			if(G == Games) Games = G->Next;

			// update web db if any
			if( m_webDB )
			{
				SqlStatement(m_webDB, "Delete from Games Where IP = ? and Port = ?;").bindText(1, G->GameInfos.ip).bindInt(2, G->GameInfos.port).run();
			}

			delete G;
			NbGames--;
			break;
		}
	}


	// we check 2 times a day
	if(TimeSinceBanCheck >= 43200)
	{
		CheckBans();
		TimeSinceBanCheck = 0;
	}

	return 0;
}

void cMasterServer::AddClient(long baboNetID,char *IP)
{

	//on va se rendre a la fin des clients presentement connecter
	MasterClient *C	=	Clients;

	if(C)
	{
		while(C->Next)
		{
			C = C->Next;
		}

		C->Next			=	new MasterClient((UINT4)baboNetID,IP);
		C->Next->Previous	=	C;
		//C			=	C->Next;
	}
	else
	{
		Clients =	new MasterClient((UINT4)baboNetID,IP);
		//C	=	Clients;
	}
}

int cMasterServer::GetBV2List(MasterClient *client)
{

	if(!client) return 0;

	// Reset any in-progress queue (refresh on the same TCP session).
	client->nbGames = 0;
	client->CurrentGame = 0;

	//on va sneder les info du master au player
	stMasterInfo info;
	info.NbGames	=	NbGames;
	
	bb_serverSend((char*)&info,sizeof(stMasterInfo),MASTER_INFO,client->BabonetID);
	
	// Send every BV2_ROW now instead of one per tick (MasterClient::Update), so a
	// quick refresh or disconnect can't lose rows.
	if(NbGames)
	{
		int i=0;
		for(cBV2game *G=Games;G;G=G->Next)
		{
			bb_serverSend((char*)&(G->GameInfos), sizeof(stBV2row), BV2_ROW, client->BabonetID);

			i++;
			if(i>=100) break;
		}
	}
	
	

	return 0;
}

bool cMasterServer::IsPrivateAddress(const char *ip)
{
	int a = -1, b = -1;
	if (sscanf(ip, "%d.%d", &a, &b) != 2) return false;
	return a == 10 || a == 127 || (a == 192 && b == 168) || (a == 172 && b >= 16 && b <= 31);
}

int cMasterServer::UpdateGame(stBV2row *row,UINT4 fromID)
{
	// check for racial slurs in the server name
	if( (strstr( row->serverName, "Nigger" )) || (strstr( row->serverName, "nigger" )) || (strstr( row->serverName, "Niger" )) || (strstr( row->serverName, "niger" )))
	{
		return 0;
	}

	//on va creater la game, le id est retourner peu importe si elle existe ou pas

	MasterClient *c = GetClientByID(fromID);
	if(!c) return 0;


	c->isServer = true;
	c->Timeout = 0;

	// let's check if the database version is good, if not, we'll send the new version/url
	unsigned short version = (unsigned short)atoi(SqlStatement::value(MasterDB, "Select Value From Settings Where Name = 'DBVersion';").c_str());

	if( row->DBVersion != version )
	{
		// send url! (DB value is data, never a format string)
		std::string url = SqlStatement::value(MasterDB, "Select Value From Settings Where Name = 'AccountURL';");
		if (!url.empty())
		{
			char *buf = new char[512];
			memset(buf, 0, 512);
			memcpy(buf,&version,2);
			snprintf(buf + 2, 510, "%s", url.c_str());
			bb_serverSend( buf,512,ACCOUNT_URL,fromID);
			delete [] buf;
		}
	}

	
	// A server behind NAT or Docker advertises its public address; trusted only from a private or loopback source
	// (PNS-44, ADR 0022)
	row->ip[sizeof(row->ip) - 1] = '\0';
	row->Priority = 0; // master use only: a client-set priority would bypass the address rule
	char * ip = (row->ip[0] && IsPrivateAddress(c->IP)) ? row->ip : c->IP;
	int gameID = CreateGame(new cBV2game(ip,*row,fromID));
	return gameID;
}

int cMasterServer::CreateGame(cBV2game* newGame)
{
	cBV2game *lastHighPriority=0; //keep the last high priority game encountered


	if(Games)
	{
		//on va a la fin dla liste
		cBV2game *G = 0;
		for(G = Games;G->Next;G=G->Next)
		{
			if(G->GameInfos.Priority == 0) lastHighPriority = G;

			//on check si la game existe deja
			if(!stricmp(G->GameInfos.ip,newGame->GameInfos.ip))
			{
				if(G->GameInfos.port==newGame->GameInfos.port)
				{
					//printf("UPDATED Game with ID: %i IP : %s port : %i",G->ID,newGame->IP,newGame->Port);
					G->LastCheck = 0;
					//la game existe deja on update,delete et on sort
					memcpy(&(G->GameInfos),&(newGame->GameInfos),sizeof(stBV2row));
					G->BaboID = newGame->BaboID; // the connection that sent this heartbeat (KILL_SERV)

					WriteWebGame(G->GameInfos, false);

					delete newGame;
					return 0;
				}
			}
		}
		//on check si la game existe deja
		if(!stricmp(G->GameInfos.ip,newGame->GameInfos.ip))
		{
			if(G->GameInfos.Priority == 0) lastHighPriority = G;

			if(G->GameInfos.port==newGame->GameInfos.port)
			{
				//printf("UPDATED Game with ID: %i IP : %s port : %i",G->ID,newGame->IP,newGame->Port);

				//la game existe deja on update, delete et on sort
				G->LastCheck = 0;
				memcpy(&(G->GameInfos),&(newGame->GameInfos),sizeof(stBV2row));
				G->BaboID = newGame->BaboID; // the connection that sent this heartbeat (KILL_SERV)

				WriteWebGame(G->GameInfos, false);

				delete newGame;
				return 0;
			}
		}

		if(newGame->GameInfos.Priority == 0 && lastHighPriority)
		{
			if(lastHighPriority->Next)	lastHighPriority->Next->Previous = newGame;
			newGame->Next = lastHighPriority->Next;
			newGame->Previous = lastHighPriority;
			lastHighPriority->Next = newGame;
		}
		else
		{
			G->Next = newGame;
			newGame->Previous = G;
		}

		//on va attribuer un nouveau ID
		//newGame->ID = ++LastBV2id;

	}
	else
	{
		Games	= newGame;
		//newGame->ID = ++LastBV2id;
	}

	//printf("CREATED Game with IP : %s port : %i\n",newGame->GameInfos.ip ,newGame->GameInfos.port);
	//*exist = false;

	WriteWebGame(newGame->GameInfos, true);

	NbGames++;

	return 0;
}

cBV2game* cMasterServer::GetGameByConnection(UINT4 baboID,unsigned short port)
{
	for(cBV2game* G = Games; G; G=G->Next)
		if(G->BaboID == baboID && G->GameInfos.port == port) return G;
	return 0;
}


UINT4	cMasterServer::GetGameBaboIDByIPport(char *ip,unsigned short port)
{
	for(cBV2game* G = Games; G; G=G->Next)
	{
		if(!stricmp(G->GameInfos.ip,ip))
		{
			if(G->GameInfos.port == port)
			{
				return G->BaboID;
			}
		}
	}
	return (UINT4)-1;
}

void cMasterServer::RemoveClient(long babonetID)
{
	for(MasterClient *C=Clients;C;C=C->Next)
	{
		if(C->BabonetID	==	babonetID)
		{
			if(C->Next)		C->Next->Previous	=	C->Previous;
			if(C->Previous)		C->Previous->Next	=	C->Next;

				
			if(C==Clients) Clients = Clients->Next;
			delete C;
			return;
		}
	}
}	

MasterClient* cMasterServer::GetClientByID(UINT4 babonetID)
{	
	for(MasterClient *c=Clients;c;c=c->Next)
	{
		if(c->BabonetID == babonetID) return c;
	}

	return 0;
}

cMasterServer::~cMasterServer()
{
	//sqlite3_close(MasterDB);
	if( m_webDB )
	{
		sqlite3_close(m_webDB);
	}

	//on va delter toute les players
	MasterClient *toKill=0;
	for(MasterClient *C = Clients;C;delete toKill)
	{
		toKill 	= 	C;
		C 	= 	C->Next;
	}

	//on va deleter les games de bv2
	cBV2game *toK = 0;
	for(cBV2game *G = Games;G;delete toK)
	{
		toK	=	G;
		G	=	G->Next;
	}
	
}
