// ROBO-274: hits spray black oil with about one coolant fleck in six, never red; the minibot leaks coolant only
#include "HitSpray.h"

#include <cstdio>

static int g_fail = 0;

static void check(bool ok, const char* what)
{
	if (!ok)
	{
		std::printf("FAIL: %s\n", what);
		g_fail = 1;
	}
}

static bool isCoolant(const hitspray::Color& c)
{
	return c.r == hitspray::COOLANT[0] && c.g == hitspray::COOLANT[1] && c.b == hitspray::COOLANT[2];
}

int main()
{
	int coolant = 0;
	for (int roll = 0; roll < hitspray::COOLANT_ONE_IN; ++roll)
	{
		for (float shade : {hitspray::OIL_SHADE_MIN, hitspray::OIL_SHADE_MAX})
		{
			hitspray::Color c = hitspray::player(roll, shade, .7f);
			check(c.a == .7f, "alpha passes through");
			check(!(c.r > c.g && c.r > c.b), "no red tint");
			if (isCoolant(c))
				++coolant;
			else
				check(c.r == shade && c.g == shade && c.b == shade && shade <= .1f, "oil is a dark grey");
		}
	}
	check(coolant == 2, "exactly one roll in COOLANT_ONE_IN is coolant (2 shades)");
	check(hitspray::COOLANT_ONE_IN == 6, "one in six");
	// #39FF8E
	check(hitspray::COOLANT[0] > .2f && hitspray::COOLANT[0] < .23f && hitspray::COOLANT[1] == 1.0f &&
			  hitspray::COOLANT[2] > .55f && hitspray::COOLANT[2] < .57f,
		  "coolant is #39FF8E");
	for (float glow : {.3f, 1.0f})
	{
		hitspray::Color c = hitspray::minibot(glow, 1);
		check(c.g > c.r && c.g > c.b, "minibot spray is green");
	}
	if (!g_fail)
		std::printf("ok: hit spray\n");
	return g_fail;
}
