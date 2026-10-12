// Phase B step 12: the admin HTTP API refuses a wrong token, limits a burst, runs a command and returns its output,
// returns the status, and logs every request with secrets masked. Real TCP to the in-process server (POSIX).
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

// Opens a connection and sends one request, without waiting
static int start(const std::string & method, const std::string & path, const std::string & token,
	const std::string & body = "")
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
	std::string text = method + " " + path + " HTTP/1.1\r\nHost: test\r\n";
	if (!token.empty()) text += "Authorization: Bearer " + token + "\r\n";
	text += "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body;
	send(s, text.data(), text.size(), 0);
	fcntl(s, F_SETFL, fcntl(s, F_GETFL, 0) | O_NONBLOCK);
	return s;
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

	// Every request is audited; the password set above is masked
	std::ifstream file(bv2::userFile("admin-audit.log"));
	std::stringstream log;
	log << file.rdbuf();
	const std::string audit = log.str();
	CHECK(audit.find(" denied GET /status") != std::string::npos);
	CHECK(audit.find(" ok info") != std::string::npos);
	CHECK(audit.find(" ok set sv_password ***") != std::string::npos && audit.find("hunter2") == std::string::npos);
	CHECK(audit.find(" limited GET /status") != std::string::npos);

	harness::stop();
	if (harness::failures() == 0) std::printf("ok: admin API\n");
	return harness::failures() ? 1 : 0;
}
