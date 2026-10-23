#include "Retention.h"

#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <sstream>

namespace retention
{
	namespace
	{
		const long long DAY = 24 * 60 * 60;

		std::string oneLine(std::string text)
		{
			for (char & c : text)
				if (c == '\t' || c == '\n' || c == '\r') c = ' ';
			return text;
		}

		std::vector<Ban> readLegacy(const std::string & path, long long now)
		{
			std::vector<Ban> bans;
			std::ifstream file(path, std::ios::binary);
			char record[48];
			while (file.read(record, sizeof(record)))
			{
				record[31] = record[47] = '\0';
				bans.push_back({record, record + 32, now});
			}
			return bans;
		}
	}

	std::vector<Ban> readBans(const std::string & path, const std::string & legacyPath, long long now)
	{
		std::ifstream file(path, std::ios::binary);
		if (!file) return readLegacy(legacyPath, now);
		std::vector<Ban> bans;
		std::string line;
		while (std::getline(file, line))
		{
			if (!line.empty() && line.back() == '\r') line.pop_back();
			const size_t a = line.find('\t'), b = a == std::string::npos ? a : line.find('\t', a + 1);
			if (b == std::string::npos) continue; // not a ban line
			bans.push_back({line.substr(b + 1), line.substr(a + 1, b - a - 1), std::atoll(line.c_str())});
		}
		return bans;
	}

	bool writeBans(const std::string & path, const std::vector<Ban> & bans)
	{
		std::ofstream file(path, std::ios::trunc | std::ios::binary);
		for (const Ban & ban : bans)
			file << ban.since << '\t' << oneLine(ban.ip) << '\t' << oneLine(ban.name) << '\n';
		return (bool)file;
	}

	bool expireBans(std::vector<Ban> & bans, long long now, int days)
	{
		if (days <= 0) return false;
		const size_t before = bans.size();
		for (size_t i = bans.size(); i-- > 0;)
			if (now - bans[i].since > days * DAY) bans.erase(bans.begin() + i);
		return bans.size() != before;
	}

	bool expireLog(const std::string & path, long long now, int days)
	{
		char cutoff[32];
		const time_t oldest = (time_t)(now - days * DAY);
		strftime(cutoff, sizeof(cutoff), "%Y-%m-%dT%H:%M:%SZ", gmtime(&oldest));
		std::ifstream in(path, std::ios::binary);
		if (!in) return false;
		std::ostringstream kept;
		std::string line;
		bool dropped = false;
		while (std::getline(in, line))
		{
			if (line.compare(0, std::strlen(cutoff), cutoff) < 0)
				dropped = true;
			else
				kept << line << '\n';
		}
		in.close();
		if (dropped) std::ofstream(path, std::ios::trunc | std::ios::binary) << kept.str();
		return dropped;
	}
}
