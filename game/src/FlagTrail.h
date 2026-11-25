// Which way a carried CTF flag points (ROBO-269): away from the carrier's movement, turning there smoothly. BV2 turned
// it with the carrier's aim every frame, which spun it. Plain C++: tests/test_flag_trail.cpp drives it.
#pragma once

#include <cmath>

namespace flagtrail
{
	const float MIN_SPEED = 0.5f; // map units per second; slower carriers keep the flag's direction
	const float TURN_RATE = 6.0f; // share of the remaining turn made per second
	const float DEGREES_PER_RADIAN = 57.2957795f; // TO_DEGREE in zeven's CVector.h; this header stays plain C++

	// a in [0, 360)
	inline float wrap(float a)
	{
		a = std::fmod(a, 360.0f);
		return a < 0 ? a + 360.0f : a;
	}

	// The flag's angle in degrees (the model's cloth points along +x at 0) after delay seconds of a carrier moving at
	// (vx, vy): it turns the short way toward the direction opposite the movement
	inline float angle(float current, float vx, float vy, float delay)
	{
		if (vx * vx + vy * vy < MIN_SPEED * MIN_SPEED)
			return current;
		float target = std::atan2(-vy, -vx) * DEGREES_PER_RADIAN;
		float diff = wrap(target - current + 180.0f) - 180.0f;
		float step = TURN_RATE * delay < 1.0f ? TURN_RATE * delay : 1.0f;
		return wrap(current + diff * step);
	}
}
