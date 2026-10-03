#ifndef _NET_MANAGER_H
#define _NET_MANAGER_H

//le net manager va gerer notre objet Server, ou notre client, ou les 2
//et communiquer le data necessaire aux bonnes entites presente

#include <map>
#include "cMSstruct.h"
#include "cMasterServer.h"
#include "string.h"


class cNetManager
{
private:

	class RemoteCacheListReq
	{
	public:
		RemoteCacheListReq(UINT4 _fromID, short _reqNum)
		{
			fromID = _fromID;
			reqNum = _reqNum;
		}

		UINT4 fromID;
		short reqNum;
	};

	typedef std::map<UINT4, RemoteCacheListReq> RemoteCacheListReqMap;
	typedef std::pair<UINT4, RemoteCacheListReq> RemoteCacheListReqPair;

	RemoteCacheListReqMap cacheRequests;

public:
	

	cMasterServer	*Server;			//notre serveur

	cNetManager();
	~cNetManager();


	void	Init();																					//initialise la babonet

	void	SpawnServer(unsigned short listenPort=11114);							//permet de spawner le master server
	//void	Connect(char* ip,unsigned short port);

	void	RetreiveData();
	void	ReceiveServerPacket(char *data,int typeID,UINT4 fromID);

	bool	Update(float elapsed);	//permet de donner du temps a la network

};

#endif
