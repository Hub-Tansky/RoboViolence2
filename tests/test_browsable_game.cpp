// A server browser row is a copy of a lobby game. CLobby::getNext() frees the game it returned before (and its
// stBV2row), so the copy needs its own row: a shared one left the browser joining freed memory (an empty IP).
#include "CMaster.h"

#include <cstdio>
#include <cstring>

int main()
{
	SBrowsableGame * lobbyGame = new SBrowsableGame();
	lobbyGame->bv2Row = new stBV2row();
	snprintf(lobbyGame->bv2Row->ip, sizeof(lobbyGame->bv2Row->ip), "%s", "192.0.2.1");
	lobbyGame->bv2Row->port = 3333;
	lobbyGame->ping = 42;

	SBrowsableGame * row = new SBrowsableGame(*lobbyGame);
	delete lobbyGame; // as the lobby's next getNext()

	const bool ok = row->bv2Row && strcmp(row->bv2Row->ip, "192.0.2.1") == 0 && row->bv2Row->port == 3333 && row->ping == 42;
	delete row;
	if (!ok)
	{
		std::printf("FAIL the browser row lost its game when the lobby freed it\n");
		return 1;
	}
	std::printf("ok: browser rows own their game\n");
	return 0;
}
