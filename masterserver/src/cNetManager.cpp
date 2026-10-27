#include "cNetManager.h"
#include "SqlStatement.h"

#include <sodium.h>

#include <cstdlib>
#include <cstring>
#include <ctime>

// Bans and unbans carry an HMAC keyed by BV2_MASTER_BAN_PASS (15 characters or more; unset refuses them), so the
// password never crosses the wire (PNS-43, ADR 0038). A proof must be within BAN_PROOF_WINDOW of the master's clock.
static const size_t BAN_PASS_MIN = 15;
static const long long BAN_PROOF_WINDOW = 300; // seconds of clock skew and replay accepted

template <class Packet> static bool banProofMatches(Packet packet)
{
	const char * secret = std::getenv("BV2_MASTER_BAN_PASS");
	const long long now = (long long)std::time(nullptr);
	if (!secret || std::strlen(secret) < BAN_PASS_MIN || std::llabs(now - (long long)packet.Time) > BAN_PROOF_WINDOW ||
		sodium_init() < 0)
		return false;

	uint8_t proof[sizeof(packet.Proof)], expected[crypto_auth_hmacsha256_BYTES];
	std::memcpy(proof, packet.Proof, sizeof(proof));
	std::memset(packet.Proof, 0, sizeof(packet.Proof));
	crypto_auth_hmacsha256_state state;
	crypto_auth_hmacsha256_init(&state, reinterpret_cast<const unsigned char *>(secret), std::strlen(secret));
	crypto_auth_hmacsha256_update(&state, reinterpret_cast<const unsigned char *>(&packet), sizeof(packet));
	crypto_auth_hmacsha256_final(&state, expected);
	return sodium_memcmp(expected, proof, sizeof(proof)) == 0;
}

// The wire size of each packet the master reads: anything else is refused before its memcpy (an older game's ban
// with the password in clear included, ADR 0038)
static int packetSize(int typeID)
{
	switch (typeID)
	{
		case BV2_LIST:					return sizeof(stBV2list);
		case BV2_ROW:					return sizeof(stBV2row);
		case KILL_SERV:					return sizeof(stKillServ);
		case CACHE_BAN_LIST:			return sizeof(stCacheList);
		case CACHE_BAN:					return sizeof(stCacheBan);
		case CACHE_UNBAN:				return sizeof(stCacheUnban);
		case CACHE_BANNED:				return sizeof(stCacheBanned);
		case CACHE_LIST_REMOTE:			return sizeof(stCacheListRemote);
		case CACHE_LIST_PLAYER:			return sizeof(stCachePlayer);
		case CACHE_LIST_PLAYER_END:		return sizeof(stCacheListEnd);
		default:						return -1;
	}
}


cNetManager::cNetManager()
{
	Server			=	0;
}


void cNetManager::Init()
{
	const char * banPass = std::getenv("BV2_MASTER_BAN_PASS");
	if (!banPass || !banPass[0]) printf("BV2_MASTER_BAN_PASS is unset: bans and unbans are refused\n");
	else if (std::strlen(banPass) < BAN_PASS_MIN) printf("BV2_MASTER_BAN_PASS is shorter than %zu characters: bans and unbans are refused\n", BAN_PASS_MIN);
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
	
	int			size=0;
	
	while((buffer=bb_serverReceive(fromID,typeID,&size)))
	{
		ReceiveServerPacket(buffer,typeID,fromID,size);
	}
}

void cNetManager::ReceiveServerPacket(char *data,int typeID,UINT4 fromID,int size)
{

	//Console->AddText("Receiving packet with TypeID : %i",typeID);
	if (size != packetSize(typeID)) return;

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
			if (!banProofMatches(cb)) return;
			cb.Nick[sizeof(cb.Nick) - 1] = cb.IP[sizeof(cb.IP) - 1] = '\0';

			Server->AddBan( cb.Nick , cb.IP , cb.Duration );
			return;
		}

		case CACHE_UNBAN:
		{
			stCacheUnban cb;
			memcpy(&cb,data,sizeof(stCacheUnban));
			if (!banProofMatches(cb)) return;

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
