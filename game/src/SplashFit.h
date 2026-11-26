// Where the first-run splash goes in the 800 x 600 menu units (ROBO-275, ADR ROBO-275): the picture keeps its 16:9 shape
// and covers the window, cropping the sides of a narrower window or the top and bottom of a wider one. The texture is
// 2048 x 1024 (power of two), squeezed from the 16:9 picture, so the shape comes from SPLASH_ASPECT, not from the file.
// Plain C++: tests/test_splash_fit.cpp drives it.
#pragma once

namespace splashfit
{
	const float SPLASH_ASPECT = 16.0f / 9.0f;

	struct Rect
	{
		float x, y, w, h;
	};

	// The splash quad in units of an 800 x 600 ortho stretched over a winW x winH pixel window
	inline Rect cover(int winW, int winH)
	{
		if (winW <= 0 || winH <= 0)
			return {0, 0, 800, 600};
		const float window = (float)winW / (float)winH;
		if (window < SPLASH_ASPECT) // narrower: full height, sides cut
		{
			const float w = 800.0f * SPLASH_ASPECT / window;
			return {(800.0f - w) / 2, 0, w, 600};
		}
		const float h = 600.0f * window / SPLASH_ASPECT; // wider: full width, top and bottom cut
		return {0, (600.0f - h) / 2, 800, h};
	}
}
