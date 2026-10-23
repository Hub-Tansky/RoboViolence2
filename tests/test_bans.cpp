// The server's bans (ADR 0034): 0.0.1's binary banlist moves to banlist.tsv on start and still refuses that IP
// (`legacy`); a ban older than sv_banDays lets the IP in again (`expired`). Needs BV2_PREF_DIR, one mode per run.
#include "server_harness.h"

#include "Console.h"

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <string>

int main(int argc, char ** argv)
{
	const char * pref = std::getenv("BV2_PREF_DIR");
	const std::string mode = argc > 1 ? argv[1] : "", tsv = std::string(pref ? pref : ".") + "/banlist.tsv",
					  legacy = std::string(pref ? pref : ".") + "/banlist";
	if (pref) std::filesystem::create_directories(pref);
	std::remove(tsv.c_str());
	std::remove(legacy.c_str());
	if (mode == "legacy")
	{
		char record[48] = {};
		std::snprintf(record, 32, "%s", "Griefer");
		std::snprintf(record + 32, 16, "%s", "127.0.0.1"); // the harness clients' IP
		std::ofstream(legacy, std::ios::binary).write(record, sizeof(record));
	}
	else if (mode == "expired")
		std::ofstream(tsv, std::ios::binary) << (long long)time(nullptr) - 2 * 86400 << "\t127.0.0.1\tGriefer\n";
	else
	{
		std::printf("usage: test_bans legacy|expired\n");
		return 1;
	}
	if (!harness::start(argv[0])) return 1;

	if (mode == "legacy")
	{
		CHECK(std::ifstream(tsv) && !std::ifstream(legacy));
		UINT4 id = harness::connect();
		CHECK(harness::slotOf(id) < 0); // refused
	}
	else
	{
		console->sendCommand("set sv_banDays 1"); // loaded under 365 days, now past its single day
		UINT4 id = harness::connect();
		CHECK(harness::slotOf(id) >= 0);
		std::ifstream file(tsv);
		std::string line;
		CHECK(std::getline(file, line) && line.find("Griefer") != std::string::npos); // kept: matching never drops a ban
	}
	harness::stop();
	std::printf("%s: %d failures\n", mode.c_str(), harness::failures());
	return harness::failures();
}
