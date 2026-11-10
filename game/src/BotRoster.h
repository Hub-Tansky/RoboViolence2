// Bot roster rules (ROBO-123, ADR 0049): how many bots each team gets, and their names. No game state here, so the
// rules are unit-tested on their own (tests/test_bots.cpp).
#pragma once
#include <string>
#include <vector>

namespace bots
{
	// Humans (by team) and bots on the server
	struct Count
	{
		int humans = 0, bots = 0;
		int total() const { return humans + bots; }
	};

	struct Teams
	{
		int blue = 0, red = 0;
	};

	// sv_bots is a target population: humans on a team + bots = target, so each human who joins blue or red replaces
	// one bot. A human still connecting or spectating replaces none (no bot churn while they load) but takes a slot:
	// one slot below maxPlayers always stays free, so a human can join. Bots fill the smaller team first.
	Teams wanted(int humansBlue, int humansRed, int humans, int target, int maxPlayers);

	// Names from a text file, one per line; blank lines and lines starting with # are skipped
	std::vector<std::string> loadNames(const std::string& path);
	// Longest name taken from the file: with " [bot]" it fits the 31-character wire name (playerName[32])
	const size_t MAX_NAME = 25;
	// A bot's display name: names[index] (cycled, cut to MAX_NAME) with " [bot]", or "Bot <index+1> [bot]" without names.
	// The server passes pickName's result, or the roster index when no name is free
	std::string name(const std::vector<std::string>& names, int index);
	// A new bot's name, so every name in the list gets used: the roll-th (mod the count) name whose display name no
	// bot in `taken` has, or -1 when every name is taken or there are none
	int pickName(const std::vector<std::string>& names, const std::vector<std::string>& taken, int roll);
}
