// Widescreen (Stretch) used to draw a 4:3 projection into the whole 16:10 window, so the world was 20% too wide.
// The 3D projection now takes its aspect from the viewport view::rect gives.
#include "ViewGeometry.h"

#include <cmath>
#include <cstdio>

static bool near(float a, float b) { return std::fabs(a - b) < 0.01f; }

int main()
{
	int failures = 0;
	auto check = [&](bool ok, const char * what) { if (!ok) std::printf("FAIL %s\n", what), ++failures; };

	auto aspect = [](int w, int h, int mode) { return view::aspect(view::rect(w, h, mode)); };
	check(near(aspect(1920, 1200, 1), 1.6f), "stretch at 1920x1200 is 16:10");
	check(near(aspect(960, 600, 1), 1.6f), "stretch at 960x600 is 16:10");
	check(near(aspect(1920, 1080, 1), 1920.f / 1080.f), "stretch at 1920x1080 is 16:9");
	check(near(aspect(1920, 1200, 2), 1.333f), "bars stay 4:3");
	check(near(aspect(800, 600, 0), 800.f / 600.f), "standard at 800x600 is 4:3");
	check(near(aspect(800, 0, 1), 1.333f), "empty window falls back to 4:3");

	view::Rect bars = view::rect(1920, 1200, 2);
	check(bars.w == 1599 && bars.h == 1200 && bars.x == 160, "bars viewport is 4:3 and centred");
	view::Rect full = view::rect(1920, 1200, 1);
	check(full.x == 0 && full.w == 1920 && full.h == 1200, "stretch viewport is the whole window");

	check(near(view::clampCameraX(0, 64, 1.333f), 5), "4:3 margin is 5");
	check(near(view::clampCameraX(0, 64, 1.6f), 6), "16:10 margin is 6");
	check(near(view::clampCameraX(64, 64, 1.6f), 58), "right margin");
	check(near(view::clampCameraX(30, 64, 1.6f), 30), "inside the margins it moves freely");
	check(near(view::clampCameraX(1, 10, 1.6f), 5), "narrow map centres the camera");
	check(near(view::clampCameraY(0, 64), 4) && near(view::clampCameraY(64, 64), 60), "top and bottom margin is 4");
	check(near(view::clampCameraY(1, 6), 3), "short map centres the camera");

	if (!failures) std::printf("ok: view_geometry\n");
	return failures ? 1 : 0;
}
