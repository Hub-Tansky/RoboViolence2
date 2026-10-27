// Personal data on a server's disk expires (https://roboviolence.com/privacy.html, ADR 0034): bans after sv_banDays, with 0.0.1's
// binary banlist moved once, and admin-audit.log lines after AUDIT_DAYS.
#include "Retention.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

static int failures = 0;
#define CHECK(cond)                                                              \
	do                                                                           \
	{                                                                            \
		if (!(cond))                                                             \
		{                                                                        \
			std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);           \
			++failures;                                                          \
		}                                                                        \
	} while (0)

static std::string slurp(const std::string & path)
{
	std::ifstream in(path, std::ios::binary);
	std::ostringstream text;
	text << in.rdbuf();
	return text.str();
}

static bool exists(const std::string & path)
{
	return (bool)std::ifstream(path);
}

int main(int argc, char ** argv)
{
	const std::string dir = argc > 1 ? argv[1] : ".";
	const std::string tsv = dir + "/retention-banlist.tsv", legacy = dir + "/retention-banlist", log = dir + "/retention-audit.log";
	std::remove(tsv.c_str());
	const long long now = 1792000000, day = 86400; // 2026-10-14T17:46:40Z

	// 0.0.1's binary records (name and IP NUL-padded to 32 and 16 bytes) move to banlist.tsv, dated now
	{
		std::ofstream out(legacy, std::ios::binary | std::ios::trunc);
		char record[48] = {};
		std::snprintf(record, 32, "%s", "Griefer");
		std::snprintf(record + 32, 16, "%s", "203.0.113.7");
		out.write(record, sizeof(record));
	}
	retention::BanList bans;
	bans.load(tsv, legacy, now, 365);
	CHECK(bans.entries().size() == 1 && bans.entries()[0].name == "Griefer" && bans.entries()[0].ip == "203.0.113.7");
	CHECK(slurp(tsv) == "1792000000\t203.0.113.7\tGriefer\n" && !exists(legacy) && !exists(tsv + ".tmp"));

	// Every change is saved at once; a name can't break the line format
	bans.add("Tab\tName\nX", "198.51.100.2", now - 400 * day);
	bans.add("Third", "192.0.2.9", now);
	CHECK(slurp(tsv) == "1792000000\t203.0.113.7\tGriefer\n1757440000\t198.51.100.2\tTab Name X\n1792000000\t192.0.2.9\tThird\n");
	bans.remove(2);
	CHECK(bans.entries().size() == 2 && slurp(tsv).find("Third") == std::string::npos);

	// Matching ignores an expired ban but never drops it (the console's unban indexes stay put); 0 days: never expires
	CHECK(bans.match("198.51.100.2", now, 365) == nullptr && bans.entries().size() == 2);
	CHECK(bans.match("198.51.100.2", now, 0) != nullptr);
	CHECK(bans.match("203.0.113.7", now + 365 * day, 365) != nullptr); // exactly a year old: still banned
	CHECK(bans.match("203.0.113.7", now + 365 * day + 1, 365) == nullptr);
	CHECK(bans.match("192.0.2.1", now, 365) == nullptr);

	// Loading drops the expired bans from the file; with nothing expired the file is left alone
	retention::BanList reloaded;
	reloaded.load(tsv, legacy, now, 365);
	CHECK(reloaded.entries().size() == 1 && reloaded.entries()[0].name == "Griefer");
	CHECK(slurp(tsv) == "1792000000\t203.0.113.7\tGriefer\n");
	reloaded.load(tsv, legacy, now, 365);
	CHECK(reloaded.entries().size() == 1);

	// A binary banlist left beside banlist.tsv is deleted unread
	std::ofstream(legacy, std::ios::binary) << "stale";
	reloaded.load(tsv, legacy, now, 365);
	CHECK(!exists(legacy) && reloaded.entries().size() == 1);

	// A save whose rename fails (here: a non-empty folder in the way) leaves no temporary file with nicks and IPs
	const std::string blocked = dir + "/retention-blocked.tsv";
	std::filesystem::create_directories(blocked + "/x");
	retention::BanList unwritable;
	unwritable.load(blocked, legacy, now, 365);
	unwritable.add("Griefer", "203.0.113.7", now);
	CHECK(!exists(blocked + ".tmp"));
	std::filesystem::remove_all(blocked);

	// Audit lines older than the limit go, newer ones stay in order; the writer's stamp is the one compared
	CHECK(retention::utcStamp((time_t)now) == "2026-10-14T17:46:40Z");
	{
		std::ofstream out(log, std::ios::binary | std::ios::trunc);
		out << "2026-07-01T10:00:00Z 192.0.2.1 ok status\n"
		    << "2026-07-16T17:46:39Z 192.0.2.2 ok status\n"
		    << "2026-07-16T17:46:40Z 192.0.2.3 ok status\n"
		    << "2026-10-14T09:00:00Z 192.0.2.4 denied -\n";
	}
	CHECK(retention::expireLog(log, now, 90)); // cutoff 2026-07-16T17:46:40Z, kept
	CHECK(slurp(log) == "2026-07-16T17:46:40Z 192.0.2.3 ok status\n2026-10-14T09:00:00Z 192.0.2.4 denied -\n");
	CHECK(!retention::expireLog(log, now, 90));
	CHECK(!retention::expireLog(dir + "/retention-missing.log", now, 90));

	std::remove(tsv.c_str());
	std::remove(log.c_str());
	std::printf(failures ? "retention: %d failures\n" : "retention: ok\n", failures);
	return failures;
}
