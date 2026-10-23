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
#include "Retention.h"
#include "Scene.h"
#include "Server.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <ctime>

extern Scene * scene;

namespace
{
#ifdef BV2_PLATFORM_WINDOWS
	typedef SOCKET Socket;
	const Socket NONE = INVALID_SOCKET;
	void closeSocket(Socket s) { closesocket(s); }
	void setBlocking(Socket s, bool blocking) { u_long on = blocking ? 0 : 1; ioctlsocket(s, FIONBIO, &on); }
	bool wouldBlock() { return WSAGetLastError() == WSAEWOULDBLOCK; }
	const int EXCLUSIVE_BIND = SO_EXCLUSIVEADDRUSE; // SO_REUSEADDR on Windows would let another process take the port
#else
	typedef int Socket;
	const Socket NONE = -1;
	void closeSocket(Socket s) { ::close(s); }
	void setBlocking(Socket s, bool blocking)
	{
		const int flags = fcntl(s, F_GETFL, 0);
		fcntl(s, F_SETFL, blocking ? flags & ~O_NONBLOCK : flags | O_NONBLOCK);
	}
	bool wouldBlock() { return errno == EAGAIN || errno == EWOULDBLOCK; }
	const int EXCLUSIVE_BIND = SO_REUSEADDR; // restarts don't wait for TIME_WAIT
#endif
	Socket sock(std::uintptr_t s) { return (Socket)s; }

	const size_t MAX_REQUEST = 4096;
	const size_t MAX_CONNECTIONS = 8; // more are closed at once: the API never holds many sockets
	const float TIMEOUT = 1;
	const float BURST = 5;

	// Commands that delete the scene or the server under the request (they run on the game thread)
	const char * const REFUSED[] = {"restart", "disconnect", "host", "connect"};

	// A header's value, from the head only (up to the blank line)
	std::string header(const std::string & head, const char * name)
	{
		std::string lower = head, key = std::string("\r\n") + name + ":";
		std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
		std::transform(key.begin(), key.end(), key.begin(), ::tolower);
		const size_t at = lower.find(key);
		if (at == std::string::npos) return "";
		const size_t start = head.find_first_not_of(' ', at + key.size()), end = head.find("\r\n", at + key.size());
		return start == std::string::npos || start >= end ? "" : head.substr(start, end - start);
	}

	std::string reply(int code, const char * reason, const std::string & body)
	{
		char head[160];
		snprintf(head, sizeof(head), "HTTP/1.1 %d %s\r\nContent-Type: text/plain; charset=utf-8\r\nContent-Length: %zu\r\nConnection: close\r\n\r\n", code, reason, body.size());
		return head + body;
	}

	// Log fields can't break the line format: control characters become '?'
	std::string clean(std::string text)
	{
		for (char & c : text)
			if ((unsigned char)c < 0x20 || c == 0x7f) c = '?';
		return text.empty() ? "-" : text;
	}

	// The command as logged: a secret's value is masked as the console masks it
	std::string masked(const std::string & command)
	{
		char verb[16] = "", name[64] = "";
		if (sscanf(command.c_str(), "%15s %63s", verb, name) == 2 && std::string(verb) == "set" && dksvarIsSecret(name))
			return std::string("set ") + name + " ***";
		return command;
	}

	void audit(const std::string & source, const std::string & result, const std::string & what)
	{
		if (FILE * f = fopen(bv2::userFile("admin-audit.log").c_str(), "a"))
		{
			fprintf(f, "%s %s %s %s\n", retention::utcStamp(time(nullptr)).c_str(), clean(source).c_str(), result.c_str(), clean(masked(what)).c_str());
			fclose(f);
		}
	}

	// The proxy's address, so only it may name the client in X-Forwarded-For
	std::string proxyAddress()
	{
		if (gameVar.zsv_adminApiProxy.isNull()) return "";
		hostent * host = gethostbyname(gameVar.zsv_adminApiProxy.s);
		return host && host->h_addr_list[0] ? inet_ntoa(*(in_addr *)host->h_addr_list[0]) : "";
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
	for (Connection & c : connections) closeSocket(sock(c.socket));
	connections.clear();
	if (listening) closeSocket(sock(listener));
	listening = false;
}

void AdminApi::listen(int port, const std::string & address)
{
	Socket s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == NONE) return;
	int on = 1;
	setsockopt(s, SOL_SOCKET, EXCLUSIVE_BIND, (const char *)&on, sizeof(on));
	sockaddr_in at = {};
	at.sin_family = AF_INET;
	at.sin_port = htons((unsigned short)port);
	at.sin_addr.s_addr = inet_addr(address.c_str());
	if (bind(s, (sockaddr *)&at, sizeof(at)) != 0 || ::listen(s, 8) != 0)
	{
		// Not retried until the port or address changes
		console->add(CString("\x4> Admin API: cannot listen on %s:%i", address.c_str(), port), true);
		closeSocket(s);
		return;
	}
	setBlocking(s, false);
	listener = (std::uintptr_t)s;
	listening = true;
	console->add(CString("> Admin API on %s:%i", address.c_str(), port), true);
}

void AdminApi::update(float delay)
{
	const int port = gameVar.zsv_adminApiPort;
	const std::string address = gameVar.zsv_adminApiBind.s;
	if (port != wantedPort || address != wantedAddress)
	{
		wantedPort = port;
		wantedAddress = address;
		close();
		if (port > 0) listen(port, address);
	}
	// The audit log names requesters' IPs: hourly, its lines past retention go
	const time_t now = time(nullptr);
	if (now - auditPruned >= 3600)
	{
		retention::expireLog(bv2::userFile("admin-audit.log"), now, retention::AUDIT_DAYS);
		auditPruned = now;
	}
	if (!listening) return;

	for (auto it = budget.begin(); it != budget.end();)
	{
		it->second += delay;
		it = it->second >= BURST ? budget.erase(it) : std::next(it);
	}

	sockaddr_in from;
	socklen_t size = sizeof(from);
	Socket s;
	while ((s = accept(sock(listener), (sockaddr *)&from, &size)) != NONE)
	{
		if (connections.size() >= MAX_CONNECTIONS) closeSocket(s);
		else
		{
			setBlocking(s, false);
			connections.push_back({(std::uintptr_t)s, inet_ntoa(from.sin_addr), "", 0});
		}
		size = sizeof(from);
	}

	for (size_t i = 0; i < connections.size();)
	{
		Connection & c = connections[i];
		c.age += delay;
		char chunk[1024];
		int got;
		while ((got = (int)recv(sock(c.socket), chunk, sizeof(chunk), 0)) > 0 && c.request.size() <= MAX_REQUEST)
			c.request.append(chunk, got);
		const bool closed = got == 0 || (got < 0 && !wouldBlock());
		const size_t headEnd = c.request.find("\r\n\r\n");
		const std::string head = headEnd == std::string::npos ? "" : c.request.substr(0, headEnd + 2);
		const long length = std::strtol(header(head, "Content-Length").c_str(), nullptr, 10);
		const bool tooLarge = c.request.size() > MAX_REQUEST || length < 0 || length > (long)MAX_REQUEST;
		const bool complete = headEnd != std::string::npos && (tooLarge || c.request.size() >= headEnd + 4 + (size_t)length);
		if (!complete && !closed && c.age < TIMEOUT && !tooLarge)
		{
			++i;
			continue;
		}
		// The client is the TCP peer; behind the proxy, the last address the proxy added to X-Forwarded-For
		std::string source = c.peer;
		const std::string forwarded = header(head, "X-Forwarded-For");
		if (!forwarded.empty() && c.peer == proxyAddress())
		{
			source = forwarded.substr(forwarded.rfind(',') + 1);
			source.erase(0, source.find_first_not_of(' '));
		}
		if (complete || tooLarge)
		{
			const Served served = tooLarge ? Served{reply(413, "Payload Too Large", "too large\n"), "", "too large"} : serve(c.request.substr(0, headEnd + 4 + (size_t)length), source);
			if (tooLarge) audit(source, served.result, "");
			setBlocking(sock(c.socket), true);
			for (size_t sent = 0; sent < served.response.size();)
			{
				const int n = (int)send(sock(c.socket), served.response.data() + sent, (int)(served.response.size() - sent), 0);
				if (n <= 0) break;
				sent += (size_t)n;
			}
		}
		else
			audit(source, "timeout", "");
		closeSocket(sock(c.socket));
		connections.erase(connections.begin() + i);
	}
}

AdminApi::Served AdminApi::serve(const std::string & request, const std::string & source)
{
	const std::string line = request.substr(0, request.find_first_of("\r\n"));
	const std::string target = line.substr(0, line.rfind(' ')); // "POST /command"
	const size_t headEnd = request.find("\r\n\r\n");
	const std::string head = request.substr(0, headEnd + 2);

	auto answer = [&](int code, const char * reason, const std::string & body, const char * result, const std::string & what) {
		audit(source, result, what);
		return Served{reply(code, reason, body), what, result};
	};

	float & left = budget.emplace(source, BURST).first->second;
	if (left < 1) return answer(429, "Too Many Requests", "too many requests\n", "limited", target);
	left -= 1;

	// The admin password as a bearer token, compared in constant time (as proofs, so lengths don't leak)
	const std::string auth = header(head, "Authorization");
	const std::string token = auth.compare(0, 7, "Bearer ") == 0 ? auth.substr(7) : "";
	const uint8_t zero[credentials::NONCE_SIZE] = {};
	uint8_t proof[credentials::MAC_SIZE];
	credentials::prove(token.c_str(), "admin api", zero, proof);
	if (gameVar.zsv_adminPass.isNull() || token.empty() || !credentials::check(gameVar.zsv_adminPass.s, "admin api", zero, proof))
		return answer(401, "Unauthorized", "unauthorized\n", "denied", target);

	if (!header(head, "Transfer-Encoding").empty())
		return answer(411, "Length Required", "send Content-Length\n", "refused", target);
	if (target == "GET /status") return answer(200, "OK", status(), "ok", target);
	if (target != "POST /command") return answer(404, "Not Found", "not found\n", "not found", target);

	const std::string body = request.substr(headEnd + 4);
	const std::string command = body.substr(0, body.find_first_of("\r\n"));
	char verb[32] = "";
	sscanf(command.c_str(), "%31s", verb);
	for (const char * refused : REFUSED)
		if (std::string(verb) == refused)
			return answer(403, "Forbidden", std::string(refused) + " is not allowed over the admin API\n", "refused", command);

	audit(source, "ok", command); // before it runs: a command that ends the server is still logged
	const size_t before = console->events().size();
	console->sendCommand(CString("%s", command.c_str()), true); // the command is data, not a format
	std::string output;
	for (size_t i = before; i < console->events().size(); ++i)
		output += std::string(textColorLess(console->events()[i]).s) + "\n";
	return Served{reply(200, "OK", output), command, "ok"};
}
