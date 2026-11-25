// ROBO-269: a carried flag trails behind the carrier and turns there smoothly, instead of spinning with the aim.
#include "FlagTrail.h"

#include <cmath>
#include <cstdio>

static bool near(float a, float b)
{
	return std::fabs(a - b) < 0.01f;
}

int main()
{
	int failures = 0;
	auto check = [&](bool ok, const char* what)
	{ if (!ok) std::printf("FAIL %s\n", what), ++failures; };

	float a = 0;
	for (int i = 0; i < 60; ++i)
		a = flagtrail::angle(a, 0, 3, 1 / 30.0f);
	check(near(a, 270), "moving up (+y) the flag points down (270) after 2 s");
	check(near(flagtrail::angle(0, -3, 0, 1), 0), "moving left the flag points right (0) and stays");

	check(near(flagtrail::angle(90, 0, 0, 1 / 30.0f), 90), "standing still keeps the direction");
	check(near(flagtrail::angle(90, 0.3f, 0, 1 / 30.0f), 90), "drifting below the minimum speed keeps the direction");

	float step = flagtrail::angle(0, 0, 3, 1 / 30.0f);
	check(step > 270 && step < 360, "one frame turns only part of the way, the short way from 0 toward 270");
	float across = flagtrail::angle(350, -3, -0.5f, 1 / 30.0f);
	check(across > 350 || across < 10, "350 toward about 9.5 turns the short way across 0");
	check(near(flagtrail::angle(10, 3, 0, 1), 180), "a full second's step ends on the target");

	std::printf(failures ? "FAIL (%d)\n" : "OK\n", failures);
	return failures ? 1 : 0;
}
