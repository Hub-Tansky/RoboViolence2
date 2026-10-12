#ifdef BV2_PLATFORM_WINDOWS
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "AdminApi.h"
#include "Console.h"
#include "Credentials.h"
#include "GameVar.h"
#include "Helper.h"
#include "Paths.h"
#include "Scene.h"
#include "Server.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <ctime>

extern Scene * scene;

namespace
{
#ifdef BV2_PLATFORM_WINDOWS
	typedef SOCKET Socket;
	const Socket NONE = INVALID_SOCKET;
	void closeSocket(Socket s) { closesocket(s); }
	void nonBlocking(Socket s) { u_long on = 1; ioctlsocket(s, FIONBIO, &on); }
	bool wouldBlock() { return WSAGetLastError() == WSAEWOULDBLOCK; }
#else
	typedef int Socket;
	const Socket NONE = -1;
	void closeSocket(Socket s) { ::close(s); }
	void nonBlocking(Socket s) { fcntl(s, F_SETFL, fcntl(s, F_GETFL, 0) | O_NONBLOCK); }
	bool wouldBlock() { return errno == EAGAIN || errno == EWOULDBLOCK; }
#endif

	const size_t MAX_REQUEST = 4096;
	const float TIMEOUT = 5;

	std::string header(const std::string & request, const char * name)
	{
		std::string lower = request;
		std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
		std::string key = std::string("\r\n") + name + ":";
		std::transform(key.begin(), key.end(), key.begin(), ::tolower);
		size_t at = lower.find(key);
		if (at == std::string::npos) return "";
		size_t start = request.find_first_not_of(' ', at + key.size()), end = request.find("\r\n", at + key.size());
		return start == std::string::npos || start > end ? "" : request.substr(start, end - start);
	}

	std::string reply(int code, const char * reason, const std::string & body)
	{
		char head[160];
		snprintf(head, sizeof(head), "HTTP/1.1 %d %s\r\nContent-Type: text/plain; charset=utf-8\r\nContent-Length: %zu\r\nConnection: close\r\n\r\n", code, reason, body.size());
		return head + body;
	}

	// The command as logged: a secret's value is masked as the console masks it
	std::string masked(const std::string & command)
	{
		char verb[16] = "", name[64] = "";
		if (sscanf(command.c_str(), "%15s %63s", verb, name) == 2 && std::string(verb) == "set" && dksvarIsSecret(name))
			return std::string("set ") + name + " ***";
		return command;
	}

	void audit(const std::string & source, const std::string & what, const std::string & result)
	{
		char stamp[32];
		time_t now = time(nullptr);
		strftime(stamp, sizeof(stamp), "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));
		if (FILE * f = fopen(bv2::userFile("admin-audit.log").c_str(), "a"))
		{
			fprintf(f, "%s %s %s %s\n", stamp, source.c_str(), result.c_str(), what.c_str());
			fclose(f);
		}
	}

	std::string status()
	{
		Game * game = scene->server->game;
		std::string out = std::string("map ") + game->mapName.s + "\ngame type " + std::to_string(game->gameType) +
			"\nscore blue " + std::to_string(game->blueScore) + " red " + std::to_string(game->redScore) + "\n";
		int players = 0;
		std::string list;
		for (int i = 0; i < MAX_PLAYER; ++i)
			if (Player * p = game->players[i])
			{
				++players;
				list += std::to_string(i) + " " + textColorLess(p->name).s + " team " + std::to_string(p->teamID) + " ping " + std::to_string(p->avgPing) + "\n";
			}
		return out + "players " + std::to_string(players) + "\n" + list;
	}
}

AdminApi::~AdminApi()
{
	close();
}

void AdminApi::close()
{
	for (Connection & c : connections) closeSocket((Socket)c.fd);
	connections.clear();
	if (listener != (long long)NONE) closeSocket((Socket)listener);
	listener = NONE;
	boundPort = 0;
}

void AdminApi::listen(int port, const std::string & address)
{
	close();
	boundPort = port;
	boundAddress = address;
	Socket s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == NONE) return;
	int on = 1;
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char *)&on, sizeof(on));
	sockaddr_in at = {};
	at.sin_family = AF_INET;
	at.sin_port = htons((unsigned short)port);
	at.sin_addr.s_addr = inet_addr(address.c_str());
	if (bind(s, (sockaddr *)&at, sizeof(at)) != 0 || ::listen(s, 8) != 0)
	{
		console->add(CString("\x4> Admin API: cannot listen on %s:%i", address.c_str(), port), true);
		closeSocket(s);
		return;
	}
	nonBlocking(s);
	listener = (long long)s;
	console->add(CString("> Admin API on %s:%i", address.c_str(), port), true);
}

void AdminApi::update(float delay)
{
	const int port = gameVar.zsv_adminApiPort;
	const std::string address = gameVar.zsv_adminApiBind.s;
	if (port != boundPort || address != boundAddress)
	{
		if (port > 0) listen(port, address);
		else close();
	}
	if (listener == (long long)NONE) return;

	for (auto & b : budget) b.second = std::min(5.0f, b.second + delay);

	sockaddr_in from;
	socklen_t size = sizeof(from);
	Socket s;
	while ((s = accept((Socket)listener, (sockaddr *)&from, &size)) != NONE)
	{
		nonBlocking(s);
		connections.push_back({(long long)s, inet_ntoa(from.sin_addr), "", 0});
		size = sizeof(from);
	}

	for (size_t i = 0; i < connections.size();)
	{
		Connection & c = connections[i];
		c.age += delay;
		char chunk[1024];
		int got;
		while ((got = (int)recv((Socket)c.fd, chunk, sizeof(chunk), 0)) > 0 && c.request.size() <= MAX_REQUEST)
			c.request.append(chunk, got);
		const bool closed = got == 0 || (got < 0 && !wouldBlock());
		const size_t headEnd = c.request.find("\r\n\r\n");
		const size_t length = headEnd == std::string::npos ? 0 : (size_t)atoi(header(c.request, "Content-Length").c_str());
		const bool complete = headEnd != std::string::npos && c.request.size() >= headEnd + 4 + length;
		if (!complete && !closed && c.age < TIMEOUT && c.request.size() <= MAX_REQUEST)
		{
			++i;
			continue;
		}
		if (complete || c.request.size() > MAX_REQUEST)
		{
			// Behind the proxy, the client is its X-Forwarded-For; the port is reachable from the proxy network only
			std::string forwarded = header(c.request, "X-Forwarded-For");
			std::string source = forwarded.empty() ? c.peer : forwarded.substr(0, forwarded.find(','));
			std::string command, result;
			std::string response = c.request.size() > MAX_REQUEST ? (result = "too large", reply(413, "Payload Too Large", "too large\n"))
				: serve(c.request, source, command, result);
			send((Socket)c.fd, response.data(), (int)response.size(), 0);
			audit(source, command.empty() ? "-" : masked(command), result);
		}
		closeSocket((Socket)c.fd);
		connections.erase(connections.begin() + i);
	}
}

std::string AdminApi::serve(const std::string & request, const std::string & source, std::string & command, std::string & result)
{
	const std::string line = request.substr(0, request.find("\r\n"));
	command = line.substr(0, line.rfind(' '));

	if (budget.find(source) == budget.end()) budget[source] = 5;
	if (budget[source] < 1)
	{
		result = "limited";
		return reply(429, "Too Many Requests", "too many requests\n");
	}
	budget[source] -= 1;

	// The admin password as a bearer token, compared in constant time (as proofs, so lengths don't leak)
	const std::string auth = header(request, "Authorization");
	const std::string token = auth.compare(0, 7, "Bearer ") == 0 ? auth.substr(7) : "";
	const uint8_t zero[credentials::NONCE_SIZE] = {};
	uint8_t proof[credentials::MAC_SIZE];
	credentials::prove(token.c_str(), "admin api", zero, proof);
	if (gameVar.zsv_adminPass.isNull() || token.empty() || !credentials::check(gameVar.zsv_adminPass.s, "admin api", zero, proof))
	{
		result = "denied";
		return reply(401, "Unauthorized", "unauthorized\n");
	}

	const std::string body = request.substr(request.find("\r\n\r\n") + 4);
	if (line.compare(0, 12, "GET /status ") == 0)
	{
		result = "ok";
		return reply(200, "OK", status());
	}
	if (line.compare(0, 14, "POST /command ") == 0)
	{
		command = body.substr(0, body.find_first_of("\r\n"));
		const size_t before = console->events().size();
		console->sendCommand(CString("%s", command.c_str()), true); // the command is data, not a format
		std::string output;
		for (size_t i = before; i < console->events().size(); ++i)
			output += std::string(textColorLess(console->events()[i]).s) + "\n";
		result = "ok";
		return reply(200, "OK", output);
	}
	result = "not found";
	return reply(404, "Not Found", "not found\n");
}
