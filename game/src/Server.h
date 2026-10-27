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

#ifndef SERVER_H
#define SERVER_H


#include "Game.h"
#include "AdminApi.h"
#include "Credentials.h"
#include "Retention.h"
#include "ServerValidator.h"
#include <map>
#include <string>

#ifndef BV2_PLATFORM_WINDOWS
	#include "platform.h"
#endif
#include <set>
#include <vector>

#define GAME_VERSION_SV 22100 // Phase A: fixed-width packed wire structs (step 4.2); Phase B step 10: password challenge

#define GAME_UPDATE_DELAY 20


class CCurl;

struct cachedPlayer
{
	bool Valid;
	char NickName[32];
	char IP[16];
};

class Server
{
public:
	// Le jeu controll�
	Game * game;

	// Cheat checks on received messages (step 8)
	ServerValidator validator;

	// Remote admin over HTTP behind a TLS proxy (step 12)
	AdminApi adminApi;

	// Per slot: the challenge sent in its JOIN_CHALLENGE, for the join and admin passwords (step 10)
	uint8_t joinNonce[MAX_PLAYER][credentials::NONCE_SIZE] = {};
	// Per slot: failed admin logins on this connection; the 5th disconnects it (PNS-42)
	int adminFails[MAX_PLAYER] = {};
	// Per IP, across reconnects: from the 5th failed admin login, that IP's admin requests are refused for 60 s,
	// doubling with each further failure up to an hour (rate limiting per NIST SP 800-63B-4, ADR 0038)
	struct AdminLock { int fails = 0; long long until = 0; };
	std::map<std::string, AdminLock> adminLocks;
	// Per slot: seconds since the TCP connect while the join handshake is pending; past JOIN_TIMEOUT it is dropped
	float joinWait[MAX_PLAYER] = {};
	static constexpr float JOIN_TIMEOUT = 10.0f;

#ifndef CONSOLE
	// La font
	unsigned int font;
#endif

	// Si le server run
	bool isRunning;

	// une erreur ou de quoi, on doit shutdowner
	bool needToShutDown;

	// Le temps restant avant le prochain ping
	float pingDelay;

	// Le delait avant de switcher de map
	float changeMapDelay;
	CString nextMap;

	// La liste des maps � jouer
	std::vector<CString> mapList;

	//A list of all maps + their sizes;
	std::vector<mapInfo> mapInfoList;

	// Ban list (banlist.tsv in the pref dir): name, IP and date; a ban lasts sv_banDays
	retention::BanList bans;

	// 50 Cached players in ram only , so we can ban them eventually
	int			 CachedIndex; // what index are we going to use for next client
	cachedPlayer CachedPlayers[50];

	// List of players downloading maps (playerID,map)
	struct SMapTransfer
	{
		unsigned long	uniqueClientID;
		CString	mapName;
		int		chunkNum;
	};
	std::vector<SMapTransfer> mapTransfers;

	// List of commands that can be used with vote
	std::vector<CString> voteList;

	// List of authorization requests
	std::vector<CCurl*>	authRequests;

	const float maxTimeOverMaxPing;

	//const float maxIdleTime;

	long frameID;

	float autoBalanceTimer;

	float infoSendDelay;

public:
	// Constructeur
	Server(Game * pGame);

	// Destructeur
	virtual ~Server();

	// Pour starter le server
	int host();

	// Pour l'updater
	void update(float delay);
	void updateNet(float delay, bool send);
	void updateCTF(float delay);
	void updateSnD(float delay);
	void sendServerInfo();

	// Pour changer la map
	void changeMap(CString & mapName);
	void addmap(CString & mapName);
	void removemap(CString & mapName);
	std::vector<CString> populateMapList(bool all = false);


	// Le voting validity
	bool validateVote(CString vote);

	void nukeAll();
	void nukePlayer(int);
	// Disconnects a slot's client and frees the slot at once (no disconnect event follows, and the ping watchdog
	// skips loading players); `reason` goes to the netlog
	void dropSlot(int i, const char * reason);
	// Drops the slots whose join handshake is still pending after JOIN_TIMEOUT
	void dropStalledJoins(float delay);
	// Pour chatter
	void sayall(CString message);
#ifndef CONSOLE
	// Pour le dessiner !?
	void render();
#endif

	// On a re�u un message y�� !
	void recvPacket(char * buffer, int size, int typeID, unsigned long bbnetID);
	// recvPacket's pre-dispatch check (Phase B step 4); returns the sender's slot or -1
	int checkPacket(const char * buffer, int size, int typeID, unsigned long bbnetID, std::vector<char> & out);
	// Connections whose join handshake (and password) passed
	std::set<unsigned long> joined;

	// Pour modifier une variable remotly
	void sendSVChange(CString varCom);

	// Pour aller chercher la prochaine map � loader
	CString queryNextMap();

	// Auto balance
	void autoBalance();

	bool filterMapFromRotation(const mapInfo & map);

	std::vector<std::string> reportUploadURLs;

	typedef std::multimap<int, PlayerStats*> StatsCache;
	typedef std::pair<int, PlayerStats*> StatsCachePair;

	StatsCache getCachedStats()
	{
		return statsCache;
	}

private:
	void cacheStats(const Player* player);

	void cacheStats(const Player* player, int teamid);

	PlayerStats* getStatsFromCache(int userID);

	void removeStatsFromCache(int userID);

	void clearStatsCache();

	void updateStatsCache();

	// List of stats of disconnected players, cleared at the end of round
	StatsCache statsCache;

	std::vector<CCurl*> reportUploads;

	struct delayedKickStruct
	{
		delayedKickStruct(unsigned long _babonetID, int _playerID, float _timeToKick)
		{
			babonetID = _babonetID;
			playerID = _playerID;
			timeToKick = _timeToKick;
		}

		UINT4 babonetID;
		int playerID;
		float timeToKick;
	};

	// babonetID->info
	typedef std::map<unsigned long, delayedKickStruct> DelayedKicksMap;
	typedef std::pair<unsigned long, delayedKickStruct> DelayedKicksPair;
	DelayedKicksMap delayedKicks;

	void addDelayedKick(unsigned long _babonetID, int _playerID, float _timeToKick);
};


#endif

