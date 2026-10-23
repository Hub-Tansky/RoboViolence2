// Personal data on a server's disk expires (website/privacy.html): bans after sv_banDays, with 0.0.1's binary banlist
// read once, and admin-audit.log lines after AUDIT_DAYS.
#include "Retention.h"

#include <cstdio>
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

int main(int argc, char ** argv)
{
	const std::string dir = argc > 1 ? argv[1] : ".";
	const std::string tsv = dir + "/retention-banlist.tsv", legacy = dir + "/retention-banlist", log = dir + "/retention-audit.log";
	std::remove(tsv.c_str());
	const long long now = 1792000000, day = 86400; // 2026-10-14T17:46:40Z

	// 0.0.1's binary records (name and IP NUL-padded to 32 and 16 bytes) are read when there is no banlist.tsv, dated now
	{
		std::ofstream out(legacy, std::ios::binary | std::ios::trunc);
		char record[48] = {};
		std::snprintf(record, 32, "%s", "Griefer");
		std::snprintf(record + 32, 16, "%s", "203.0.113.7");
		out.write(record, sizeof(record));
	}
	std::vector<retention::Ban> bans = retention::readBans(tsv, legacy, now);
	CHECK(bans.size() == 1 && bans[0].name == "Griefer" && bans[0].ip == "203.0.113.7" && bans[0].since == now);

	// A name can't break the line format, and banlist.tsv wins over the binary file once written
	bans.push_back({"Tab\tName\nX", "198.51.100.2", now - 400 * day});
	CHECK(retention::writeBans(tsv, bans));
	CHECK(slurp(tsv) == "1792000000\t203.0.113.7\tGriefer\n1757440000\t198.51.100.2\tTab Name X\n");
	bans = retention::readBans(tsv, legacy, now + 1);
	CHECK(bans.size() == 2 && bans[0].since == now && bans[1].name == "Tab Name X" && bans[1].ip == "198.51.100.2");

	// Bans older than the limit go; 0 keeps them all
	CHECK(!retention::expireBans(bans, now, 0) && bans.size() == 2);
	CHECK(retention::expireBans(bans, now, 365) && bans.size() == 1 && bans[0].name == "Griefer");
	CHECK(!retention::expireBans(bans, now + 365 * day, 365)); // exactly a year old: still banned
	CHECK(retention::expireBans(bans, now + 365 * day + 1, 365) && bans.empty());

	// Audit lines older than the limit go, newer ones stay in order
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
	std::remove(legacy.c_str());
	std::remove(log.c_str());
	std::printf(failures ? "retention: %d failures\n" : "retention: ok\n", failures);
	return failures;
}
