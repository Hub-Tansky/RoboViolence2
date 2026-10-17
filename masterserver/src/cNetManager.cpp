#include "cNetManager.h"
#include "SqlStatement.h"

#include <cstdlib>
#include <cstring>

// Bans and unbans need BV2_MASTER_BAN_PASS (at most 7 characters: the game's wire field); unset refuses them (PNS-43)
static bool banPasswordMatches(const char * pass)
{
	const char * secret = std::getenv("BV2_MASTER_BAN_PASS");
	return secret && secret[0] && std::strncmp(pass, secret, 7) == 0 && std::strlen(secret) <= 7;
}


cNetManager::cNetManager()
{
	Server			=	0;
}


void cNetManager::Init()
{
	if(bb_init())
	{
		printf("Error Loading BaboNet");
		return;
	}	

	printf("baboNet.dll - OK\n");
	printf("IP Adress : %s\n",bb_getMyIP());

	//on va starter le master server
	SpawnServer(10207);
}

void cNetManager::SpawnServer(unsigned short listenPort)
{
	
	//doit checker si on avait un serveur existant
	if(Server)
	{
		bb_serverShutdown();
		delete Server;
	}

	
	bb_serverCreate(false,1020,listenPort);

	
	Server = new cMasterServer();
	printf("-----------Master Server Up and Running----------\n");

}


bool cNetManager::Update(float elapsed)
{
	//si on a un serveur present on va l'updater
	if(Server)
	{
		int r = Server->Update(elapsed);
		
		if(r==1)
		{
			//printf("%s",bb_serverGetLastError());
			SpawnServer(10207);
			return 1;
		}
		
		//on va pogner le data nouvellement disponible
		RetreiveData();	
	}

	return 1;
}



void cNetManager::RetreiveData()
{
	char*			buffer=0;
	int			typeID=0;
	UINT4		fromID=0;
	
	while((buffer=bb_serverReceive(fromID,typeID)))
	{
		ReceiveServerPacket(buffer,typeID,fromID);
	}
}

void cNetManager::ReceiveServerPacket(char *data,int typeID,UINT4 fromID)
{

	//Console->AddText("Receiving packet with TypeID : %i",typeID);

	switch(typeID)
	{
		case BV2_LIST:	//requete de la liste des games en cours
		{
			//on check la version que le player roule
			stBV2list query;
			memcpy(&query,data,sizeof(stBV2list));



			///// REMOVED CLIENT VERSION CHECK BEFORE SENDING GAME LIST, NOT VERY USEFULL

			//ici la version est bonne
			//if(!stricmp(query.Version,Server->Version))
			//{
				Server->GetBV2List(Server->GetClientByID(fromID));
			//}
			//ici la version est pas bonne
			/* else
			{
				stMasterInfo info;
				info.NbGames	=	-1;
	
				//MasterClient *client = Server->GetClientByID(fromID);
				//p->nRow = 0;
				//p->CurrentRow = 0;
			
				bb_serverSend((char*)&info,sizeof(stMasterInfo),MASTER_INFO,fromID,NET_TCP);
			} */
			

			return;
		}

		case BV2_ROW:	//un update d'une game de bv2, ou une nouvelle carement
		{
			stBV2row	row;
			memcpy(&row,data,sizeof(stBV2row));
			
			if(Server->UpdateGame(&row,(UINT4)fromID))
			{
				printf("Problem with Update from player %i",fromID);
			}
			else
			{
				printf("Update from player %i- OK\n",fromID);
			}
			// we dont disconnect servers anymore
			//bb_serverDisconnectClient(fromID);
			//Server->RemoveClient(fromID);

			return;
		}

// 		case PLAYER_RESULT:
// 		{
// 			//un serveur nous envoie les stats d'un player
// 			stPlayerResult res;
// 			memcpy(&res,data,sizeof(stPlayerResult));
// 			
// 			Server->UpdateStats(res);
// 
// 			return;
// 		}

// 		case FINISH_RESULT: //pas sur
// 		{
// 			bb_peerDelete(fromID,true);
// 			return;
// 		}

// 		case PLAYER_REGISTER:
// 		{
// 			stPlayerRegister reg;
// 			memcpy(&reg,data,sizeof(stPlayerRegister));
// 
// 			stRegisterResult	res;//the actual query result
// 
// 			res.Result = Server->RegisterPlayer(reg);
// 			
// 			bb_serverSend((char*)&res,sizeof(stRegisterResult),REGISTER_RESULT,fromID,NET_TCP);
// 
// 			return;
// 		}

/*		case SERVER_REGISTER:
		{
			stServerRegister reg;
			memcpy(&reg,data,sizeof(stServerRegister));

			stRegisterResult	res;//the actual query result

			char ip[16];
			unsigned short port;

			bb_peerGetIPport(fromID,ip,&port);
			res.Result = (char)Server->RegisterServer(reg,ip);
			
			bb_peerSend(fromID,(char*)&res,REGISTER_RESULT,sizeof(stRegisterResult),true);
			bb_peerDelete(fromID,false);

			return;
		}*/

		case KILL_SERV:
		{
			//on va deleter un serveur
			stKillServ kill;
			memcpy(&kill,data,sizeof(stKillServ));
			
			cBV2game	*game = Server->GetGameByConnection((UINT4)fromID,kill.Port);
			if(game)	game->LastCheck	=	9999; //timeout la game
			
			bb_serverDisconnectClient(fromID);
			Server->RemoveClient(fromID);
			
			return;
		}

		//bans
		case CACHE_BAN_LIST:
		{
			stCacheList cl;
			memcpy(&cl,data,sizeof(stCacheList));
			cl.Filter[sizeof(cl.Filter) - 1] = '\0'; // wire text becomes SQL parameters: terminate it

			Server->SendBanList( fromID , cl.Filter );

			return;
		}

		case CACHE_BAN:
		{
			stCacheBan cb;
			memcpy(&cb,data,sizeof(stCacheBan));
			cb.Nick[sizeof(cb.Nick) - 1] = cb.IP[sizeof(cb.IP) - 1] = cb.Pass[sizeof(cb.Pass) - 1] = '\0';

			if (!banPasswordMatches(cb.Pass)) return;

			Server->AddBan( cb.Nick , cb.IP , cb.Duration );
			return;
		}

		case CACHE_UNBAN:
		{
			stCacheUnban cb;
			memcpy(&cb,data,sizeof(stCacheUnban));

			if (!banPasswordMatches(cb.Pass)) return;

			Server->Unban( cb.ID );
			return;
		}

		case CACHE_BANNED:
		{
			stCacheBanned cb;
			memcpy(&cb,data,sizeof(stCacheBanned));
			cb.IP[sizeof(cb.IP) - 1] = '\0';

			stCacheAnswer ca;
			ca.Answer = Server->IsBanned( cb.IP );
			ca.ID = cb.ID;
			
			bb_serverSend( (char*)&ca, sizeof(stCacheAnswer) , CACHE_ANSWER , fromID );

			printf("Client Is Banned ? ...%s\n", ca.Answer ? "Yes" : "No");

			return;
		}

		case CACHE_LIST_REMOTE:
		{
			stCacheListRemote clr;
			memcpy(&clr,data,sizeof(stCacheListRemote));

			// remove old request from this client, remember new one and send it to server
			RemoteCacheListReqMap::iterator it = cacheRequests.find(clr.FromID);
			if (it != cacheRequests.end())
				cacheRequests.erase(clr.FromID);

			UINT4 baboid = Server->GetGameBaboIDByIPport(clr.ServerIP, atoi(clr.ServerPort));
			if (baboid != -1)
			{
				cacheRequests.insert(RemoteCacheListReqPair(clr.FromID, RemoteCacheListReq(clr.FromID, clr.ReqNum)));
				// send request to server
				bb_serverSend((char*)&clr, sizeof(stCacheListRemote), CACHE_LIST_REMOTE , baboid);
			}

			return;
		}

		case CACHE_LIST_PLAYER:
		{
			stCachePlayer cp;
			memcpy(&cp,data,sizeof(stCachePlayer));

			RemoteCacheListReqMap::iterator it = cacheRequests.find(cp.FromID);
			if (it != cacheRequests.end())
				bb_serverSend((char*)&cp, sizeof(stCachePlayer), CACHE_LIST_PLAYER , cp.FromID);

			break;
		}

		case CACHE_LIST_PLAYER_END:
		{
			stCacheListEnd cle;
			memcpy(&cle,data,sizeof(stCacheListEnd));

			RemoteCacheListReqMap::iterator it = cacheRequests.find(cle.requestFromID);
			if (it != cacheRequests.end())
				cacheRequests.erase(cle.requestFromID);

			return;
		}

	}
}


cNetManager::~cNetManager()
{	
	if(Server) delete Server;

	bb_shutdown();
}
