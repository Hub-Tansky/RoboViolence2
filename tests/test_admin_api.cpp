// Phase B step 12: the admin HTTP API refuses a wrong token, limits a burst, runs a command and returns its output,
// returns the status, refuses commands that rebuild the server, handles split, oversized and silent requests, trusts
// X-Forwarded-For only from the proxy, and logs every request with secrets masked. Real TCP to the in-process server.
#include "server_harness.h"

#include "Console.h"
#include "Game.h"
#include "Paths.h"
#include "Scene.h"
#include "Server.h"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <string>
#include <vector>

extern Scene * scene;

static const int PORT = 10209;

// Opens a connection and sends raw bytes, without waiting
static int send(const std::string & text)
{
	int s = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in at = {};
	at.sin_family = AF_INET;
	at.sin_port = htons(PORT);
	at.sin_addr.s_addr = inet_addr("127.0.0.1");
	if (connect(s, (sockaddr *)&at, sizeof(at)) != 0)
	{
		close(s);
		return -1;
	}
	::send(s, text.data(), text.size(), 0);
	fcntl(s, F_SETFL, fcntl(s, F_GETFL, 0) | O_NONBLOCK);
	return s;
}

static std::string text(const std::string & method, const std::string & path, const std::string & token,
	const std::string & body = "", const std::string & extra = "")
{
	std::string t = method + " " + path + " HTTP/1.1\r\nHost: test\r\n" + extra;
	if (!token.empty()) t += "Authorization: Bearer " + token + "\r\n";
	return t + "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body;
}

// Opens a connection and sends one request, without waiting
static int start(const std::string & method, const std::string & path, const std::string & token,
	const std::string & body = "")
{
	return send(text(method, path, token, body));
}

// The server runs frames until every connection is answered and closed
static std::vector<std::string> finish(std::vector<int> sockets)
{
	std::vector<std::string> responses(sockets.size());
	std::vector<bool> done(sockets.size(), false);
	char chunk[1024];
	for (int frame = 0; frame < 90 && std::count(done.begin(), done.end(), false); ++frame)
	{
		harness::tick();
		for (size_t i = 0; i < sockets.size(); ++i)
		{
			int got;
			while (!done[i] && (got = (int)recv(sockets[i], chunk, sizeof(chunk), 0)) > 0) responses[i].append(chunk, got);
			if (!done[i] && got == 0) done[i] = true, close(sockets[i]);
		}
	}
	return responses;
}

static std::string request(const std::string & method, const std::string & path, const std::string & token,
	const std::string & body = "")
{
	return finish({start(method, path, token, body)})[0];
}

static int code(const std::string & response) { return response.size() > 12 ? std::atoi(response.c_str() + 9) : 0; }

int main(int, char ** argv)
{
	if (!harness::start(argv[0])) return 1;
	std::remove(bv2::userFile("admin-audit.log").c_str()); // only this run's lines
	console->sendCommand("set zsv_adminPass s3cret");
	console->sendCommand(CString("set zsv_adminApiPort %i", PORT));
	harness::tick();

	CHECK(code(request("GET", "/status", "")) == 401);
	CHECK(code(request("GET", "/status", "wrong")) == 401);
	std::string status = request("GET", "/status", "s3cret");
	CHECK(code(status) == 200 && status.find("map ") != std::string::npos && status.find("players ") != std::string::npos);
	std::string info = request("POST", "/command", "s3cret", "info");
	CHECK(code(info) == 200 && info.find("[Server Info]") != std::string::npos);
	CHECK(code(request("POST", "/command", "s3cret", "set sv_password hunter2")) == 200);
	CHECK(gameVar.sv_password == "hunter2");

	// A burst of 8 at once: the budget is 5 requests, refilled by 1 a second
	harness::tick(150);
	std::vector<int> burst;
	for (int i = 0; i < 8; ++i) burst.push_back(start("GET", "/status", "s3cret"));
	int limited = 0;
	for (const std::string & r : finish(burst)) limited += code(r) == 429;
	CHECK(limited >= 2);

	// Refused: commands that rebuild the server (they used to free it under the request). The server goes on.
	harness::tick(150);
	CHECK(code(request("POST", "/command", "s3cret", "restart")) == 403);
	CHECK(code(request("POST", "/command", "s3cret", "disconnect")) == 403);
	CHECK(code(request("GET", "/status", "s3cret")) == 200);
	CHECK(code(request("GET", "/nope", "s3cret")) == 404);
	CHECK(code(finish({send(text("POST", "/command", "s3cret", std::string(5000, 'x')))})[0]) == 413);

	// A request split over frames is served once complete; a silent one times out
	harness::tick(150);
	const std::string whole = text("POST", "/command", "s3cret", "info");
	int split = send(whole.substr(0, 20));
	harness::tick(3);
	::send(split, whole.data() + 20, whole.size() - 20, 0);
	CHECK(code(finish({split})[0]) == 200);
	CHECK(finish({send("GET /status HTTP/1.1\r\n")})[0].empty());

	// X-Forwarded-For counts only from the proxy (its rightmost address); a newline can't forge an audit line
	harness::tick(150);
	request("GET", "/status", "s3cret", "");
	finish({send(text("GET", "/status", "s3cret", "", "X-Forwarded-For: 203.0.113.9\r\n"))});
	console->sendCommand("set zsv_adminApiProxy 127.0.0.1");
	finish({send(text("GET", "/status", "s3cret", "", "X-Forwarded-For: 198.51.100.1, 203.0.113.7\r\n"))});
	finish({send("GET /x\n2099-01-01T00:00:00Z 192.0.2.1 ok forged HTTP/1.1\r\nContent-Length: 0\r\n\r\n")});
	finish({send(text("GET", "/status", "s3cret", "", "X-Forwarded-For: 203.0.113.8\n2099-01-01T00:00:00Z forged\r\n"))});

	// Every request is audited; the password set above is masked
	std::ifstream file(bv2::userFile("admin-audit.log"));
	std::stringstream log;
	log << file.rdbuf();
	const std::string audit = log.str();
	CHECK(audit.find(" denied GET /status") != std::string::npos);
	CHECK(audit.find(" ok info") != std::string::npos);
	CHECK(audit.find(" ok set sv_password ***") != std::string::npos && audit.find("hunter2") == std::string::npos);
	CHECK(audit.find(" limited GET /status") != std::string::npos);
	CHECK(audit.find(" refused restart") != std::string::npos && audit.find(" too large") != std::string::npos);
	CHECK(audit.find(" timeout") != std::string::npos && audit.find(" not found GET /nope") != std::string::npos);
	CHECK(audit.find("203.0.113.9") == std::string::npos && audit.find(" 203.0.113.7 ok GET /status") != std::string::npos);
	CHECK(audit.find("\n2099-") == std::string::npos && audit.find("203.0.113.8?2099") != std::string::npos);

	harness::stop();
	if (harness::failures() == 0) std::printf("ok: admin API\n");
	return harness::failures() ? 1 : 0;
}
