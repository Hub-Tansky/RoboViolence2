// ROBO-275: the first-run splash keeps its 16:9 shape and covers the window (SplashFit.h)
#include "SplashFit.h"

#include <cmath>
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

static bool near(float a, float b)
{
	return std::fabs(a - b) < 0.01f;
}

// The picture's shape on screen: quad units scaled to pixels
static float shownAspect(const splashfit::Rect& r, int winW, int winH)
{
	return (r.w * winW / 800.0f) / (r.h * winH / 600.0f);
}

int main()
{
	const int windows[][2] = {{1920, 1080}, {1024, 768}, {2560, 1080}, {1280, 800}, {800, 600}, {3840, 1600}};
	for (const auto& win : windows)
	{
		splashfit::Rect r = splashfit::cover(win[0], win[1]);
		check(near(shownAspect(r, win[0], win[1]), splashfit::SPLASH_ASPECT), "the picture keeps 16:9");
		check(r.x <= 0.01f && r.y <= 0.01f && r.x + r.w >= 799.99f && r.y + r.h >= 599.99f, "the picture covers the window");
		check(near(r.x + r.w / 2, 400) && near(r.y + r.h / 2, 300), "the picture is centred");
	}
	splashfit::Rect full = splashfit::cover(1920, 1080);
	check(near(full.x, 0) && near(full.y, 0) && near(full.w, 800) && near(full.h, 600), "a 16:9 window shows the whole picture");
	splashfit::Rect none = splashfit::cover(0, 0);
	check(near(none.w, 800) && near(none.h, 600), "no window size falls back to the whole screen");
	if (!g_fail)
		std::printf("ok: splash fit\n");
	return g_fail;
}
