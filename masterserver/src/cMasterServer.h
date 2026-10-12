#ifndef _MASTERSERVER_H
#define	_MASTERSERVER_H

#ifdef BV2_PLATFORM_WINDOWS
	#include "stdlib.h"
	#include "string.h"
#else
	#include "platform.h"
#endif

#include "cMSstruct.h"
#include "baboNet.h"
#include "MasterClient.h"
#include "cBV2game.h"
#include "sqlite3.h"






// struct stServerLink
// {
// 	CServer *Next;
// 	CServer *Previous;
// 	CServer *Current;
// };
// 
// struct stPlayerLink
// {
// 	cPlayer *Next;
// 	cPlayer *Previous;
// 	cPlayer *Current;
// };


class cMasterServer
{
private:
public:


	char			Version[5];		//garde la version de BaboViolent 2 ici
	
	int			NbGames;		//number of games currently being played
	cBV2game		*Games;			//list of current games in lobby

	MasterClient			*Clients;		//list of currently connected clients(Players or Servers)
	//stPlayerLink		*Players;		//connected players that are in *Clients
	//stServerLink		*Servers;		//connected servers that are in *Clients

	sqlite3			*MasterDB;		//pointeur sur notre master database

	sqlite3			* m_webDB;		//pointer on the database we update games to for web query purposes

	
	
	cMasterServer();
	~cMasterServer();


	


	int		Update(float elapsed);
	void		AddClient(long baboNetID,char *ip);
	void		RemoveClient(long baboNetID);

	MasterClient*	GetClientByID(UINT4 babonetID);
	cBV2game*	GetGameByIPport(char *ip,unsigned short port);								//va retourner la game qui est sur ce ip/port
	UINT4	GetGameBaboIDByIPport(char *ip,unsigned short port);
	int		CreateGame(cBV2game* newGame);										//permet de creer une nouvelle game de bv2
	void		UpdateStats(const stPlayerResult & result);


	float		TimeSinceBanCheck;

	void		CheckBans();												// to see if a ban must stop
	void		AddBan( char * NickName , char * IP , int Duration );					// to add or update a ban
	void		Unban( int ID );											// unban someone by ID
	void		SendBanList( UINT4 fromID , char * filter );							//function to send a ban list to a client	
	int		IsBanned( char * IP );									// return 0 if the guy is not banned, return 1 if he is
	long long	FindBan( const char * IP );								// the ban's ID, -1 if none
	void		WriteWebGame( const stBV2row & game , bool insert );						// a game's row in the web database

	int		RegisterServer(const stServerRegister & reg,char *ip);							//authentification of a server, return 0 on failure, return the ServerID on success
	int		RegisterPlayer(const stPlayerRegister & reg);								//tries to register a player
	int		GetBV2List(MasterClient *client);										//va pogner la liste des games de bv2, et la donner au player qui l'attend
	int		UpdateGame(stBV2row *row,UINT4 fromID);								//met a jour un enregistrement dla DB
	void		GetLastBV2ID();												//get the last gameID
};


#endif
