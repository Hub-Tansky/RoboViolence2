// The colour of hit spray and floor splats (ROBO-274): black oil with a few glowing coolant flecks, never red blood
// (style guide 01 and 02). The art pack's blood textures are white shapes and these colours tint them. Plain C++:
// tests/test_hit_spray.cpp drives it.
#pragma once

namespace hitspray
{
	struct Color
	{
		float r, g, b, a;
	};

	const float OIL_SHADE_MIN = .02f; // style token oil #0B0B0C, with some spread
	const float OIL_SHADE_MAX = .1f;
	const float COOLANT[3] = {.22f, 1.0f, .56f}; // style token coolant #39FF8E
	const int COOLANT_ONE_IN = 6; // one hit particle in six is coolant

	// A player's spray: coolant when roll (0 .. COOLANT_ONE_IN-1) is 0, else oil of the given shade
	// (OIL_SHADE_MIN .. OIL_SHADE_MAX)
	inline Color player(int roll, float shade, float alpha)
	{
		if (roll == 0)
			return {COOLANT[0], COOLANT[1], COOLANT[2], alpha};
		return {shade, shade, shade, alpha};
	}

	// The minibot's spray is additive, where oil would not show: coolant only, glow (0.3 .. 1) scales it
	inline Color minibot(float glow, float alpha)
	{
		return {COOLANT[0] * glow, COOLANT[1] * glow, COOLANT[2] * glow, alpha};
	}
}
