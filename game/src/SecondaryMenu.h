// The in-game weapon menu's secondary column (ROBO-212): a secondary the server disables is hidden and the rest close up.
// Plain C++: tests/test_secondary_menu.cpp drives it without a window.
#pragma once

namespace secondaryMenu
{
	const int count = 4; // Knives, Nuke Bot, Shield, Mini Bot: WEAPON_KNIVES + index

	// row[i]: the menu row of secondary i, -1 when hidden. Returns how many are shown.
	inline int layout(const bool enabled[count], int row[count])
	{
		int shown = 0;
		for (int i = 0; i < count; ++i)
			row[i] = enabled[i] ? shown++ : -1;
		return shown;
	}

	// The secondary to use instead of `chosen` (an index) when it is disabled: the first enabled one, else 0 (Knives),
	// as SelectToAvailableMeleeWeapon in Game.cpp
	inline int fallback(const bool enabled[count], int chosen)
	{
		if (chosen >= 0 && chosen < count && enabled[chosen])
			return chosen;
		for (int i = 0; i < count; ++i)
			if (enabled[i])
				return i;
		return 0;
	}
}
