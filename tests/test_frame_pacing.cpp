// ROBO-197: client frame pacing decisions (game/src/FramePacing.h), without a window.
#include "FramePacing.h"

#include <cstdio>

using namespace roboviolence2::framePacing;

int main()
{
	int failures = 0;
	auto check = [&](bool ok, const char* what)
	{ if (!ok) std::printf("FAIL %s\n", what), ++failures; };
	const Clock::duration zero = Clock::duration::zero();
	const Clock::duration thirtyFps = std::chrono::milliseconds(33);

	Frame playing = decide(true, true, true);
	check(playing.render && playing.minInterval == zero, "focused game renders, paced by vsync only");
	check(!playing.screenSaverAllowed, "focused game keeps the display awake");

	Frame menu = decide(true, true, false);
	check(menu.render && menu.minInterval == zero, "focused menu renders, paced by vsync only");
	check(menu.screenSaverAllowed, "menu lets the display sleep");

	Frame background = decide(true, false, true);
	check(background.render && background.minInterval == thirtyFps, "unfocused window renders at about 30 fps");
	check(background.screenSaverAllowed, "unfocused game lets the display sleep");

	Frame hidden = decide(false, true, true);
	check(!hidden.render, "hidden window doesn't render");
	check(hidden.minInterval == thirtyFps, "hidden window ticks at the 30 Hz fixed step");

	Clock::time_point t0 = Clock::time_point() + std::chrono::seconds(100);
	Clock::time_point next = t0 - std::chrono::seconds(10); // stale: focus was regained long ago
	check(schedule(next, t0, thirtyFps) == t0, "stale schedule starts now, no burst");
	check(next == t0 + thirtyFps, "next frame one interval later");
	check(schedule(next, t0 + std::chrono::milliseconds(5), thirtyFps) == t0 + thirtyFps, "early frame waits for its slot");
	check(next == t0 + 2 * thirtyFps, "schedule keeps a steady rate");

	if (failures == 0)
		std::printf("frame pacing: PASS\n");
	return failures == 0 ? 0 : 1;
}
