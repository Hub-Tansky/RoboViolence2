// The editor crashed (EXC_BAD_ACCESS at 0x0 in Map::renderGround) once a map had a team spawn but no DM spawn: the
// overlay indexed dm_spawns with no candidate. spawns::farthestFrom now says "none" with -1.
#include "SpawnOverlay.h"

#include <cstdio>
#include <vector>

struct P
{
	float v[3];
	float operator[](int i) const { return v[i]; }
};

int main()
{
	int failures = 0;
	auto check = [&](bool ok, const char * what) { if (!ok) std::printf("FAIL %s\n", what), ++failures; };
	const std::vector<P> none;
	const std::vector<P> blue = {{{0, 0, 0}}};

	check(spawns::farthestFrom(none, blue) == -1, "no DM spawn: no line origin");
	check(spawns::farthestFrom(none, none) == -1, "empty map: no line origin");
	check(spawns::farthestFrom(std::vector<P>{{{5, 5, 0}}}, none) == 0, "one DM spawn, no team spawns: that one");
	check(spawns::farthestFrom(std::vector<P>{{{1, 0, 0}}, {{9, 0, 0}}, {{4, 0, 0}}}, blue) == 1, "the farthest of three");
	check(spawns::farthestFrom(std::vector<P>{{{0, 0, 0}}, {{0, 0, 0}}}, blue) == 0, "coincident spawns: the first");
	check(spawns::farthestFrom(std::vector<P>{{{3, 0, 0}}, {{0, 3, 0}}}, std::vector<P>{{{0, 0, 0}}, {{3, 1, 0}}}) == 1,
	      "distance to the nearest team spawn, not the first");

	if (!failures) std::printf("ok: spawn_overlay\n");
	return failures ? 1 : 0;
}
