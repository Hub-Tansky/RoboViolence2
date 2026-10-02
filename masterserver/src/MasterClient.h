#ifndef MASTER_CLIENT_H
#define MASTER_CLIENT_H

#include "platform.h"

// A connection to the master server: a player browsing the lobby or a game server.
//
// The original class was missing from the source release (the shipped sources contained a copy of
// babonet's cClient under this name, and cMasterServer used members it lacks). This is rebuilt
// from how cMasterServer.cpp uses it; the timeout policy is an assumption, see MASTER_SERVER_TIMEOUT.
#define MASTER_SERVER_TIMEOUT 60.0f // seconds without a game update before a registered server is dropped

class MasterClient
{
public:
	UINT4	BabonetID;		// babonet connection handle
	char			IP[16];			// dotted IPv4 of the peer
	bool			isServer;		// true once it has sent a game update
	float			Timeout;		// seconds since the last game update (reset to 0 by cMasterServer::UpdateGame)

	int				nbGames;		// BV2 rows queued for this client (reset by GetBV2List)
	int				CurrentGame;	// index of the next row to send

	MasterClient	*Next;
	MasterClient	*Previous;

	MasterClient(UINT4 babonetID, const char *ip);

	// Advances the timeout. Returns true when the client must be dropped.
	bool Update(float elapsed);
};

#endif
