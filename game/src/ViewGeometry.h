// Where the 3D view sits in the window and how wide it is. Plain C++: tests/test_view_geometry.cpp drives it without a window.
#pragma once

namespace view
{
	// r_widescreen 2 ("Widescreen (Bars)") draws a 4:3 view centred between black bars; 0 and 1 use the whole window
	const int WIDESCREEN_BARS = 2;
	const float ASPECT_4_3 = 1.333f;

	// The 3D viewport inside a window of resW x resH: x is its left edge, w x h its size
	struct Rect { int x, w, h; };

	inline Rect rect(int resW, int resH, int widescreen)
	{
		if (widescreen >= WIDESCREEN_BARS)
		{
			int w = static_cast<int>(resH * ASPECT_4_3);
			return Rect{ (resW - w) / 2, w, resH };
		}
		return Rect{ 0, resW, resH };
	}

	// Width over height of that viewport, the aspect every 3D projection must use; 4:3 for an empty window
	inline float aspect(int resW, int resH, int widescreen)
	{
		Rect r = rect(resW, resH, widescreen);
		return (r.w > 0 && r.h > 0) ? static_cast<float>(r.w) / static_cast<float>(r.h) : ASPECT_4_3;
	}

	// How far the camera stays from the map's left and right edges so the void past them stays off screen:
	// 5 units at 4:3, wider with the view. A map narrower than both margins centres the camera.
	inline float clampCameraX(float x, float mapWidth, float viewAspect)
	{
		const float marginPerAspect = 5 / ASPECT_4_3;
		float margin = marginPerAspect * viewAspect;
		if (mapWidth <= 2 * margin) return mapWidth / 2;
		if (x < margin) return margin;
		if (x > mapWidth - margin) return mapWidth - margin;
		return x;
	}
}
