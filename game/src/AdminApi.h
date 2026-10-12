// Remote admin over HTTP (Phase B step 12, ADR 0015): `POST /command` runs a console command and returns its output,
// `GET /status` returns the game's state. Listens on zsv_adminApiBind:zsv_adminApiPort (port 0: off), only behind a
// TLS proxy. A request carries `Authorization: Bearer <zsv_adminPass>`; each source has a request budget, and every
// request leaves a line in admin-audit.log in the pref dir.
#pragma once
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
		long long fd;
		std::string peer, request;
		float age = 0;
	};
	long long listener = -1;
	int boundPort = 0;
	std::string boundAddress;
	std::vector<Connection> connections;
	std::map<std::string, float> budget; // per source: requests left (burst 5, 1 per second)

	void listen(int port, const std::string & address);
	void close();
	// The response to a complete request; `audit` gets the logged command and result
	std::string serve(const std::string & request, const std::string & source, std::string & command, std::string & result);
};
