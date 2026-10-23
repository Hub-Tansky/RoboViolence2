#include "Retention.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>

namespace retention
{
	namespace
	{
		const long long DAY = 24 * 60 * 60;

		bool expired(const Ban & ban, long long now, int days)
		{
			return days > 0 && now - ban.since > days * DAY;
		}

		std::string oneLine(std::string text)
		{
			for (char & c : text)
				if (c == '\t' || c == '\n' || c == '\r') c = ' ';
			return text;
		}

		// Writes a temporary file and renames it over `path`: a crash or a full disk never leaves half a file
		bool replaceFile(const std::string & path, const std::string & content)
		{
			const std::string temporary = path + ".tmp";
			std::ofstream out(temporary, std::ios::trunc | std::ios::binary);
			out << content;
			out.close(); // the buffered data is written here: a full disk shows up now, not in the destructor
			std::error_code error;
			if (out) std::filesystem::rename(temporary, path, error);
			if (!out || error)
			{
				std::remove(temporary.c_str());
				return false;
			}
			return true;
		}

		std::vector<Ban> readTsv(std::ifstream & file)
		{
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

	void BanList::load(const std::string & file, const std::string & legacyPath, long long now, int days)
	{
		path = file;
		std::ifstream tsv(path, std::ios::binary);
		const bool legacy = !tsv && std::ifstream(legacyPath);
		bans = legacy ? readLegacy(legacyPath, now) : readTsv(tsv);
		const size_t before = bans.size();
		for (size_t i = bans.size(); i-- > 0;)
			if (expired(bans[i], now, days)) bans.erase(bans.begin() + i);
		if ((legacy || bans.size() != before) && !save()) return;
		std::remove(legacyPath.c_str()); // moved now, or left over beside banlist.tsv: either way never read again
	}

	void BanList::add(const std::string & name, const std::string & ip, long long now)
	{
		bans.push_back({name, ip, now});
		save();
	}

	void BanList::remove(size_t index)
	{
		bans.erase(bans.begin() + index);
		save();
	}

	const Ban * BanList::match(const std::string & ip, long long now, int days) const
	{
		for (const Ban & ban : bans)
			if (ban.ip == ip && !expired(ban, now, days)) return &ban;
		return nullptr;
	}

	bool BanList::save() const
	{
		std::ostringstream text;
		for (const Ban & ban : bans)
			text << ban.since << '\t' << oneLine(ban.ip) << '\t' << oneLine(ban.name) << '\n';
		return replaceFile(path, text.str());
	}

	std::string utcStamp(time_t when)
	{
		char stamp[32];
		strftime(stamp, sizeof(stamp), "%Y-%m-%dT%H:%M:%SZ", gmtime(&when));
		return stamp;
	}

	bool expireLog(const std::string & path, long long now, int days)
	{
		const std::string cutoff = utcStamp((time_t)(now - days * DAY));
		std::ifstream in(path, std::ios::binary);
		std::ostringstream kept;
		std::string line;
		bool dropped = false;
		while (std::getline(in, line))
		{
			if (line.compare(0, cutoff.size(), cutoff) < 0)
				dropped = true;
			else
				kept << line << '\n';
		}
		in.close();
		return dropped && replaceFile(path, kept.str());
	}
}
