// How long a server keeps personal data on disk (website/privacy.html, ADR 0034): a ban (nick and IP) lasts
// sv_banDays, an admin-audit.log line (with the requester's IP) AUDIT_DAYS.
#pragma once
#include <ctime>
#include <string>
#include <vector>

namespace retention
{
	const int AUDIT_DAYS = 90;

	struct Ban
	{
		std::string name, ip;
		long long since; // Unix time of the ban
	};

	// The server's bans in banlist.tsv, one per line: since<TAB>ip<TAB>name. Every change is saved at once.
	class BanList
	{
	public:
		// Reads `path`, or without it the binary `legacyPath` of 0.0.1 (name and IP NUL-padded to 32 and 16 bytes,
		// dated `now`, deleted once moved; deleted unread when `path` exists). Drops the bans older than `days`
		// (0: bans never expire).
		void load(const std::string & path, const std::string & legacyPath, long long now, int days);
		void add(const std::string & name, const std::string & ip, long long now);
		void remove(size_t index);
		// The ban of `ip` that is younger than `days`, or null. Never changes the list, so indexes stay put.
		const Ban * match(const std::string & ip, long long now, int days) const;
		const std::vector<Ban> & entries() const { return bans; }

	private:
		std::string path;
		std::vector<Ban> bans;
		bool save() const;
	};

	// The stamp each admin-audit.log line starts with: ISO 8601 UTC (2026-10-23T11:00:00Z), so text order is time order
	std::string utcStamp(time_t when);
	// Rewrites the log at `path` without its lines older than `days`. True when a line was dropped.
	bool expireLog(const std::string & path, long long now, int days);
}
