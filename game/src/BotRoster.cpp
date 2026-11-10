#include "BotRoster.h"

#include <algorithm>
#include <fstream>

namespace bots
{
	Teams wanted(int humansBlue, int humansRed, int humans, int target, int maxPlayers)
	{
		const int capacity = std::max(0, std::min(target - humansBlue - humansRed, maxPlayers - 1 - humans));
		Teams t;
		while (t.blue + t.red < capacity)
		{
			if (humansBlue + t.blue <= humansRed + t.red)
				++t.blue;
			else
				++t.red;
		}
		return t;
	}

	std::vector<std::string> loadNames(const std::string& path)
	{
		std::vector<std::string> names;
		std::ifstream in(path);
		std::string line;
		while (std::getline(in, line))
		{
			while (!line.empty() && (line.back() == '\r' || line.back() == ' '))
				line.pop_back();
			if (!line.empty() && line[0] != '#')
				names.push_back(line);
		}
		return names;
	}

	std::string name(const std::vector<std::string>& names, int index)
	{
		if (names.empty())
			return "Bot " + std::to_string(index + 1) + " [bot]";
		return names[index % names.size()] + " [bot]";
	}
}
