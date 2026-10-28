// Where the 3D view sits in the window and how wide it is (ADR 0039: Stretch shows more map sideways, Bars stays 4:3).
// Plain C++: tests/test_view_geometry.cpp drives it without a window.
#pragma once

namespace view
{
	// r_widescreen 2 ("Widescreen (Bars)") draws a 4:3 view centred between black bars; 0 and 1 use the whole window
	const int WIDESCREEN_BARS = 2;
	const float ASPECT_4_3 = 1.333f;

	// How far the camera stays from the map's edges so the void past them stays off screen: 4 units top and bottom,
	// 5 left and right at 4:3, wider with the view
	const float CAMERA_MARGIN_X_4_3 = 5;
	const float CAMERA_MARGIN_Y = 4;

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

	// This frame's viewport, from the window resolution and r_widescreen (client only, defined in IntroScreen.cpp)
	Rect current();

	// Width over height of a viewport, the aspect the gameplay, scope and intro projections use; 4:3 for an empty window
	inline float aspect(const Rect & r)
	{
		return (r.w > 0 && r.h > 0) ? static_cast<float>(r.w) / static_cast<float>(r.h) : ASPECT_4_3;
	}

	// Keeps v at least margin inside [0, extent]; an extent narrower than both margins centres it
	inline float clampAxis(float v, float extent, float margin)
	{
		if (extent <= 2 * margin) return extent / 2;
		if (v < margin) return margin;
		if (v > extent - margin) return extent - margin;
		return v;
	}

	inline float clampCameraX(float x, float mapWidth, float viewAspect)
	{
		return clampAxis(x, mapWidth, CAMERA_MARGIN_X_4_3 / ASPECT_4_3 * viewAspect);
	}

	inline float clampCameraY(float y, float mapHeight)
	{
		return clampAxis(y, mapHeight, CAMERA_MARGIN_Y);
	}
}
