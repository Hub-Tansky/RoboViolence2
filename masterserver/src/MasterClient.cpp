#include "MasterClient.h"

#include <string.h>

MasterClient::MasterClient(UINT4 babonetID, const char *ip)
{
	BabonetID = babonetID;
	memset(IP, 0, sizeof(IP));
	if (ip)
		strncpy(IP, ip, sizeof(IP) - 1);
	isServer = false;
	Timeout = 0;
	nbGames = 0;
	CurrentGame = 0;
	Next = 0;
	Previous = 0;
}

bool MasterClient::Update(float elapsed)
{
	// Only registered servers are expected to keep talking; players may idle on the lobby list.
	if (!isServer)
		return false;
	Timeout += elapsed;
	return Timeout > MASTER_SERVER_TIMEOUT;
}
