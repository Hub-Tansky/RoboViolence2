// Phase B step 10: an admin login is accepted only with HMACs of the admin user and password over the connection's
// challenge, and neither the user, the password nor their MD5 crosses the wire. Phase C step 6 (PNS-42): a password
// under 15 characters disables admin login (ADR 0038, was 12); the 5th failure on a connection disconnects it, and
// from the 5th failure from one IP that IP's admin logins are refused for a while, even on a new connection.
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

static std::vector<std::vector<char>> delivered;

static const uint8_t * nonceFor(UINT4 id)
{
	for (const harness::SentPacket & p : harness::sent())
		if (p.typeID == NET_SVCL_JOIN_CHALLENGE && p.destination == (INT4)id) return ((const net_svcl_join_challenge *)p.data.data())->nonce;
	return nullptr;
}

static bool accepted(UINT4 id)
{
	for (const harness::SentPacket & p : harness::sent())
		if (p.typeID == NET_SVCL_ADMIN_ACCEPTED && p.destination == (INT4)id) return true;
	return false;
}

static void login(UINT4 id, const char * user, const char * password, const uint8_t * nonce)
{
	net_clsv_admin_request request;
	credentials::prove(user, "admin user", nonce, request.login);
	credentials::prove(password, "admin password", nonce, request.password);
	const char * bytes = (const char *)&request;
	delivered.emplace_back(bytes, bytes + sizeof(request));
	harness::deliver(id, NET_CLSV_ADMIN_REQUEST, request);
}

static bool contains(const std::vector<char> & data, std::string needle)
{
	return std::search(data.begin(), data.end(), needle.begin(), needle.end()) != data.end();
}

int main(int, char ** argv)
{
	if (!harness::start(argv[0])) return 1;
	console->sendCommand("set zsv_adminUser boss");
	console->sendCommand("set zsv_adminPass fourteen-chars");
	UINT4 a = harness::join(), b = harness::join(), c = harness::join();
	login(a, "boss", "fourteen-chars", nonceFor(a));
	harness::tick();
	CHECK(!accepted(a)); // 14 characters: admin login is off
	console->sendCommand("set zsv_adminPass s3cret-long-enough");


	login(a, "boss", "wrong", nonceFor(a));
	CHECK(!accepted(a));
	login(b, "boss", "s3cret-long-enough", nonceFor(a)); // a's proof replayed on b's connection
	CHECK(!accepted(b));
	login(a, "boss", "s3cret-long-enough", nonceFor(a));
	CHECK(accepted(a) && scene->server->game->players[harness::slotOf(a)]->isAdmin);

	std::vector<std::vector<char>> traffic = delivered;
	for (const harness::SentPacket & p : harness::sent()) traffic.push_back(p.data);
	bool clean = true;
	for (const char * secret : {"boss", "s3cret-long-enough"})
	{
		std::string md5(CMD5(secret).getMD5Digest()), upper = md5;
		std::transform(upper.begin(), upper.end(), upper.begin(), [](unsigned char ch) { return (char)toupper(ch); });
		for (const std::vector<char> & data : traffic)
			clean &= !contains(data, secret) && !contains(data, md5) && !contains(data, upper);
	}
	CHECK(clean);

	// Five wrong guesses disconnect the connection; four don't
	for (int i = 0; i < 4; ++i) login(c, "boss", "guess", nonceFor(c));
	harness::tick(2);
	CHECK(harness::slotOf(c) >= 0);
	login(c, "boss", "guess", nonceFor(c));
	harness::tick(2);
	CHECK(harness::slotOf(c) < 0);

	// The same IP (every harness client is 127.0.0.1) reconnects: the right password is refused while it is locked
	UINT4 d = harness::join();
	login(d, "boss", "s3cret-long-enough", nonceFor(d));
	harness::tick();
	CHECK(!accepted(d));

	harness::stop();
	if (harness::failures() == 0) std::printf("ok: admin login\n");
	return harness::failures() ? 1 : 0;
}
