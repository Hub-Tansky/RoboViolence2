// Remote admin over HTTP (Phase B step 12, ADR 0015): `POST /command` runs a console command and returns its output,
// `GET /status` returns the game's state. Listens on zsv_adminApiBind:zsv_adminApiPort (port 0: off), only behind a
// TLS proxy (zsv_adminApiProxy: the only peer whose X-Forwarded-For is believed). A request carries
// `Authorization: Bearer <zsv_adminPass>`; each source has a request budget, and every request leaves a line in
// admin-audit.log in the pref dir.
#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>

class AdminApi
{
public:
	~AdminApi();
	// Every server frame, on the game thread: follows the port and bind variables, serves complete requests
	void update(float delay);

private:
	struct Connection
	{
		std::uintptr_t socket; // a SOCKET or file descriptor (no winsock in this header)
		std::string peer, request;
		float age = 0;
	};
	struct Served
	{
		std::string response, command, result; // what to send, and what to audit
	};
	std::uintptr_t listener;
	bool listening = false;
	int wantedPort = 0;
	std::string wantedAddress;
	std::vector<Connection> connections;
	std::map<std::string, float> budget; // per source: requests left (burst 5, 1 per second); absent = full

	void listen(int port, const std::string & address);
	void close();
	Served serve(const std::string & request, const std::string & source);
};
