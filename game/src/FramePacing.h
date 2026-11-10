// Client frame pacing (ROBO-197, ADR 0050): what the main loop does with a frame, from the window's state.
#ifndef FRAMEPACING_H
#define FRAMEPACING_H

#include <chrono>

namespace roboviolence2
{
	namespace framePacing
	{
		typedef std::chrono::steady_clock Clock;

		struct Frame
		{
			bool render; // false while nothing drawn would be seen
			Clock::duration minInterval; // zero: vsync paces the frame
			bool screenSaverAllowed; // the display may sleep
		};

		// Hidden (minimized, hidden, occluded): no render, paint at the 30 Hz fixed step so the game keeps ticking.
		// Unfocused: about 30 fps. The display stays awake only for a game in the focused window.
		inline Frame decide(bool visible, bool focused, bool inGame)
		{
			Frame frame;
			frame.render = visible;
			frame.minInterval = (visible && focused) ? Clock::duration::zero() : Clock::duration(std::chrono::milliseconds(33));
			frame.screenSaverAllowed = !(focused && inGame);
			return frame;
		}

		// When to start the next frame: `next` if still ahead, else now (a stale schedule doesn't burst).
		// Moves `next` one interval past the returned time.
		inline Clock::time_point schedule(Clock::time_point& next, Clock::time_point now, Clock::duration interval)
		{
			Clock::time_point wake = next > now ? next : now;
			next = wake + interval;
			return wake;
		}
	}
}

#endif
