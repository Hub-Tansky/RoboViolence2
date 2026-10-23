// How long a server keeps personal data on disk (website/privacy.html): a ban (nick and IP) expires after sv_banDays,
// an admin-audit.log line (with the requester's IP) after AUDIT_DAYS.
#pragma once
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

	// The bans in `path`, one per line: since<TAB>ip<TAB>name. Without that file, the bans in the binary `legacyPath`
	// (before 0.0.2: name and IP NUL-padded to 32 and 16 bytes), dated `now`.
	std::vector<Ban> readBans(const std::string & path, const std::string & legacyPath, long long now);
	// Replaces `path` with every ban; a tab or line break in a name becomes a space
	bool writeBans(const std::string & path, const std::vector<Ban> & bans);
	// Drops the bans older than `days` (0: bans never expire). True when one was dropped.
	bool expireBans(std::vector<Ban> & bans, long long now, int days);
	// Rewrites the log at `path` without its lines older than `days`. Each line starts with an ISO 8601 UTC stamp
	// (2026-10-23T11:00:00Z), so text order is time order. True when a line was dropped.
	bool expireLog(const std::string & path, long long now, int days);
}
