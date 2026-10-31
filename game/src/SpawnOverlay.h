// The editor's spawn overlay: lines from the DM spawn farthest from a team's spawns to each of them.
// Plain C++: tests/test_spawn_overlay.cpp drives it without a window.
#pragma once

#include <vector>

namespace spawns
{
	// Index of the candidate whose nearest point in `others` is farthest away (the first one on ties or when `others`
	// is empty), or -1 when there is no candidate. V is any point with operator[] for x, y, z (CVector3f).
	template <class V>
	int farthestFrom(const std::vector<V> & candidates, const std::vector<V> & others)
	{
		int best = -1;
		float bestDist = -1;
		for (int i = 0; i < (int)candidates.size(); ++i)
		{
			float nearest = 1000000;
			for (const V & o : others)
			{
				float dx = candidates[i][0] - o[0], dy = candidates[i][1] - o[1], dz = candidates[i][2] - o[2];
				float d = dx * dx + dy * dy + dz * dz;
				if (d < nearest)
					nearest = d;
			}
			if (nearest > bestDist)
			{
				bestDist = nearest;
				best = i;
			}
		}
		return best;
	}
}
