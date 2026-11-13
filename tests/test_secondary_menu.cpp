// ROBO-212: the weapon menu hides a secondary the server disables (Nuke Bot, Mini Bot on the shipped servers), the rest
// close up, and a saved pick of a hidden one falls back to an enabled one (game/src/SecondaryMenu.h, Game.cpp).
#include "SecondaryMenu.h"

#include <cstdio>

int main()
{
	int failures = 0;
	auto check = [&](bool ok, const char* what)
	{ if (!ok) std::printf("FAIL %s\n", what), ++failures; };
	int row[secondaryMenu::count];

	const bool all[secondaryMenu::count] = {true, true, true, true};
	check(secondaryMenu::layout(all, row) == 4 && row[0] == 0 && row[1] == 1 && row[2] == 2 && row[3] == 3, "all on: BV2's four rows");

	const bool shipped[secondaryMenu::count] = {true, false, true, false}; // sv_enableNuclear, sv_enableMinibot false
	check(secondaryMenu::layout(shipped, row) == 2, "shipped servers: two shown");
	check(row[0] == 0 && row[1] == -1 && row[2] == 1 && row[3] == -1, "Nuke Bot and Mini Bot hidden, Shield moves up");
	check(secondaryMenu::fallback(shipped, 1) == 0, "a saved Nuke Bot pick becomes Knives");
	check(secondaryMenu::fallback(shipped, 3) == 0, "a saved Mini Bot pick becomes Knives");
	check(secondaryMenu::fallback(shipped, 2) == 2, "an enabled pick stays");

	const bool noKnives[secondaryMenu::count] = {false, false, true, false};
	check(secondaryMenu::fallback(noKnives, 1) == 2, "no Knives: the first enabled one");
	const bool none[secondaryMenu::count] = {false, false, false, false};
	check(secondaryMenu::layout(none, row) == 0 && secondaryMenu::fallback(none, 3) == 0, "none on: nothing shown, Knives");

	if (!failures)
		std::printf("ok: secondary_menu\n");
	return failures ? 1 : 0;
}
