// Phase B step 10: with sv_password set, a client joins only with an HMAC of the password over its own connection's
// challenge, and neither direction of the handshake carries the password or its MD5. The server relays a player's
// info without the account login.
#include "server_harness.h"

#include "Console.h"
#include "Credentials.h"
#include "Game.h"
#include "Scene.h"
#include "Server.h"
#include "md5class.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

extern Scene * scene;

static std::vector<std::vector<char>> delivered; // what the clients sent

static const net_svcl_gameversion * versionFor(UINT4 id)
{
	for (const harness::SentPacket & p : harness::sent())
		if (p.typeID == NET_SVCL_GAMEVERSION && p.destination == (INT4)id) return (const net_svcl_gameversion *)p.data.data();
	return nullptr;
}

static bool welcomed(UINT4 id)
{
	for (const harness::SentPacket & p : harness::sent())
		if (p.typeID == NET_SVCL_SERVER_INFO && p.destination == (INT4)id) return true;
	return false;
}

// A new client answers its GAMEVERSION with `proof`
static void answer(UINT4 id, const uint8_t proof[credentials::MAC_SIZE])
{
	net_clsv_gameversion_accepted accepted = {};
	memcpy(accepted.proof, proof, credentials::MAC_SIZE);
	const char * bytes = (const char *)&accepted;
	delivered.emplace_back(bytes, bytes + sizeof(accepted));
	harness::deliver(id, NET_CLSV_GAMEVERSION_ACCEPTED, accepted);
	harness::tick();
}

static bool contains(const std::vector<char> & data, const std::string & needle)
{
	return std::search(data.begin(), data.end(), needle.begin(), needle.end()) != data.end();
}

int main(int, char ** argv)
{
	if (!harness::start(argv[0])) return 1;
	console->sendCommand("set sv_password hunter2");
	uint8_t proof[credentials::MAC_SIZE];

	// The right password over this connection's challenge joins
	UINT4 a = harness::connect();
	const net_svcl_gameversion * version = versionFor(a);
	CHECK(version && version->gameVersion == GAME_VERSION_SV);
	credentials::prove("hunter2", "join", version->nonce, proof);
	answer(a, proof);
	CHECK(welcomed(a) && harness::slotOf(a) >= 0);

	// The player info relayed to everyone carries no account login (its username and MD5 password)
	net_clsv_svcl_player_info info = {};
	strcpy(info.playerName, "Tester");
	strcpy(info.username, "acct");
	memset(info.password, 'f', sizeof(info.password));
	harness::deliver(a, NET_CLSV_SVCL_PLAYER_INFO, info);
	bool relayed = false, blank = true;
	for (const harness::SentPacket & p : harness::sent())
		if (p.typeID == NET_CLSV_SVCL_PLAYER_INFO)
		{
			const net_clsv_svcl_player_info * r = (const net_clsv_svcl_player_info *)p.data.data();
			relayed = true;
			blank &= r->username[0] == 0 && r->password[0] == 0 && r->password[31] == 0;
		}
	CHECK(relayed && blank);

	// A replay of that proof on another connection (another challenge), and a wrong password, are disconnected
	UINT4 b = harness::connect();
	CHECK(versionFor(b) && memcmp(versionFor(b)->nonce, version->nonce, credentials::NONCE_SIZE) != 0);
	answer(b, proof);
	CHECK(!welcomed(b) && harness::slotOf(b) == -1);
	UINT4 c = harness::connect();
	credentials::prove("hunter3", "join", versionFor(c)->nonce, proof);
	answer(c, proof);
	CHECK(!welcomed(c) && harness::slotOf(c) == -1);

	// No password or MD5 of it on the wire, either way
	std::string md5(CMD5("hunter2").getMD5Digest()), MD5 = md5;
	std::transform(MD5.begin(), MD5.end(), MD5.begin(), [](unsigned char ch) { return (char)toupper(ch); });
	std::vector<std::vector<char>> traffic = delivered;
	for (const harness::SentPacket & p : harness::sent()) traffic.push_back(p.data);
	bool clean = true;
	for (const std::vector<char> & data : traffic)
		clean &= !contains(data, "hunter2") && !contains(data, md5) && !contains(data, MD5);
	CHECK(clean);

	harness::stop();
	if (harness::failures() == 0) std::printf("ok: join password\n");
	return harness::failures() ? 1 : 0;
}
