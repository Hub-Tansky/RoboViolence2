// Port of BaboViolent/babo-dedicated-server server/src/game/bots.rs (GPL-3, commit 7c7d05c); see Bot.h
#include "Bot.h"

#include "Game.h"
#include "GameVar.h"
#include "Map.h"
#include "Server.h"
#include "netPacket.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <deque>
#include <vector>

namespace bots
{
	const Tuning tuning;

	namespace
	{
		const float PLAYER_RADIUS = 0.25f;
		const float PI_F = 3.14159265f;

		float len(float x, float y)
		{
			return std::sqrt(x * x + y * y);
		}
		float dist(V2 a, V2 b)
		{
			return len(a.x - b.x, a.y - b.y);
		}
		float dist(V2 a, const CVector3f& b)
		{
			return len(a.x - b[0], a.y - b[1]);
		}
		float dist(const CVector3f& a, const CVector3f& b)
		{
			return len(a[0] - b[0], a[1] - b[1]);
		}
		V2 xy(const CVector3f& p)
		{
			return {p[0], p[1]};
		}
		CVector3f at(V2 p)
		{
			return CVector3f(p.x, p.y, 0.25f);
		}
		float clampf(float v, float lo, float hi)
		{
			return std::max(lo, std::min(hi, v));
		}
		int ownFlag(int team)
		{
			return team == PLAYER_TEAM_BLUE ? 0 : 1;
		}

		// --- Map queries (bots.rs map/mod.rs) ---

		bool inside(const Map& map, float x, float y)
		{
			return x >= 0 && y >= 0 && x < map.size[0] && y < map.size[1];
		}

		// Where a segment at player height first meets a wall (the end point when it meets none)
		CVector3f rayEnd(Map& map, CVector3f p1, CVector3f p2)
		{
			CVector3f normal;
			map.rayTest(p1, p2, normal);
			return p2;
		}

		float segmentHitT(const CVector3f& p1, const CVector3f& p2, const CVector3f& hit)
		{
			const float dx = p2[0] - p1[0], dy = p2[1] - p1[1];
			const float l2 = dx * dx + dy * dy;
			if (l2 < 1e-8f)
				return 1;
			return clampf(((hit[0] - p1[0]) * dx + (hit[1] - p1[1]) * dy) / l2, 0, 1);
		}

		// Where on p1->p2 (0..1) the segment enters a circle, if it does
		bool segmentCircleT(const CVector3f& p1, const CVector3f& p2, const CVector3f& c, float r, float& t)
		{
			const float dx = p2[0] - p1[0], dy = p2[1] - p1[1];
			const float fx = p1[0] - c[0], fy = p1[1] - c[1];
			const float a = dx * dx + dy * dy, b = 2 * (fx * dx + fy * dy), cc = fx * fx + fy * fy - r * r;
			if (a < 1e-8f)
				return false;
			const float disc = b * b - 4 * a * cc;
			if (disc < 0)
				return false;
			const float s = std::sqrt(disc);
			const float t1 = (-b - s) / (2 * a), t2 = (-b + s) / (2 * a);
			if (t1 >= 0 && t1 <= 1)
			{
				t = t1;
				return true;
			}
			if (t2 >= 0 && t2 <= 1)
			{
				t = t2;
				return true;
			}
			if (t1 < 0 && t2 > 1)
			{
				t = 0;
				return true;
			} // starts inside
			return false;
		}

		bool hasClearance(Map& map, V2 p, float radius)
		{
			if (!inside(map, p.x, p.y) || !passable(map, (int)std::floor(p.x), (int)std::floor(p.y)))
				return false;
			CoordFrame cf;
			cf.position = at(p);
			map.collisionClip(cf, radius);
			return std::fabs(cf.position[0] - p.x) < 0.001f && std::fabs(cf.position[1] - p.y) < 0.001f;
		}

		bool segmentHasClearance(Map& map, V2 from, V2 to, float radius)
		{
			const float dx = to.x - from.x, dy = to.y - from.y, l = len(dx, dy);
			if (l < 0.001f)
				return hasClearance(map, from, radius);
			const float px = -dy / l * radius, py = dx / l * radius;
			for (float side : {-1.0f, 0.0f, 1.0f})
			{
				CVector3f p1(from.x + px * side, from.y + py * side, 0.25f), p2(to.x + px * side, to.y + py * side, 0.25f);
				if (segmentHitT(p1, p2, rayEnd(map, p1, p2)) < 0.98f)
					return false;
			}
			return true;
		}

		bool bfsNextTile(const Map& map, int sx, int sy, int ex, int ey, int& nx, int& ny)
		{
			if (!passable(map, sx, sy))
				return false;
			if (sx == ex && sy == ey)
			{
				nx = ex;
				ny = ey;
				return true;
			}
			const int w = map.size[0], h = map.size[1];
			std::vector<int> parent(w * h, -1);
			std::deque<int> queue;
			const int start = sy * w + sx, end = ey * w + ex;
			parent[start] = start;
			queue.push_back(start);
			const int dirs[4][2] = {{0, 1}, {0, -1}, {1, 0}, {-1, 0}};
			while (!queue.empty())
			{
				const int cur = queue.front();
				queue.pop_front();
				if (cur == end)
				{
					int node = end;
					while (parent[node] != start)
						node = parent[node];
					nx = node % w;
					ny = node / w;
					return true;
				}
				for (const auto& d : dirs)
				{
					const int x = cur % w + d[0], y = cur / w + d[1];
					if (!passable(map, x, y) || parent[y * w + x] >= 0)
						continue;
					parent[y * w + x] = cur;
					queue.push_back(y * w + x);
				}
			}
			return false;
		}

		V2 unstuckHeading(Map& map, V2 from, V2 preferred)
		{
			const float pl = std::max(1e-4f, len(preferred.x - from.x, preferred.y - from.y));
			const float ux = (preferred.x - from.x) / pl, uy = (preferred.y - from.y) / pl;
			V2 best = preferred;
			float bestScore = -1;
			for (int i = 0; i < 8; ++i)
			{
				const float a = i * PI_F / 4, cx = std::cos(a), cy = std::sin(a);
				const V2 test{from.x + cx * 0.6f, from.y + cy * 0.6f};
				if (!passable(map, (int)std::floor(test.x), (int)std::floor(test.y)))
					continue;
				if (!segmentHasClearance(map, from, test, 0.30f))
					continue;
				const float score = cx * ux + cy * uy;
				if (score > bestScore)
				{
					best = {from.x + cx * 3, from.y + cy * 3};
					bestScore = score;
				}
			}
			return best;
		}

		// --- Brain helpers (bots.rs) ---

		bool canDamage(int gameType, int shooterTeam, int targetTeam)
		{
			if (gameType == GAME_TYPE_DM || gameType == GAME_TYPE_SND)
				return true;
			return shooterTeam != targetTeam && (shooterTeam == PLAYER_TEAM_BLUE || shooterTeam == PLAYER_TEAM_RED);
		}

		const std::vector<CVector3f>& enemySpawnPoints(int team, const Map& map)
		{
			const std::vector<CVector3f>& points = team == PLAYER_TEAM_BLUE ? map.red_spawns : map.blue_spawns;
			return points.empty() ? map.dm_spawns : points;
		}

		// The enemy spawn farthest from this team's live players: BV2 spawns players away from their opponents
		bool predictedEnemySpawn(int team, const World& w, V2& out)
		{
			float best = -1;
			for (const CVector3f& s : enemySpawnPoints(team, *w.map))
			{
				float nearest = 1e9f;
				for (int i = 0; i < MAX_PLAYER; ++i)
					if (w.alive[i] && w.team[i] == team)
						nearest = std::min(nearest, dist(s, w.pos[i]));
				if (nearest > best)
				{
					best = nearest;
					out = xy(s);
				}
			}
			return best >= 0;
		}

		float spawnDanger(V2 p, const std::vector<CVector3f>& spawns, float pressure)
		{
			float d = 0;
			for (const CVector3f& s : spawns)
				d += pressure / std::max(0.5f, dist(p, s));
			return d;
		}

		bool flagVisible(const Map& map, const CVector3f& p)
		{
			return inside(map, p[0], p[1]) && passable(map, (int)std::floor(p[0]), (int)std::floor(p[1]));
		}

		bool defenderPressure(int team, const World& w)
		{
			const int own = ownFlag(team), enemy = 1 - own;
			if (w.flagState[own] == -1)
				return true;
			const CVector3f& pod = w.flagPod[own];
			const int carrier = w.flagState[enemy];
			if (carrier >= 0 && carrier < MAX_PLAYER && w.alive[carrier] && dist(w.pos[carrier], pod) <= tuning.defendPressureRange)
				return true;
			const int enemyTeam = team == PLAYER_TEAM_BLUE ? PLAYER_TEAM_RED : PLAYER_TEAM_BLUE;
			for (int i = 0; i < MAX_PLAYER; ++i)
				if (w.alive[i] && w.team[i] == enemyTeam && dist(w.pos[i], pod) <= tuning.defendPressureRange * 0.85f)
					return true;
			return false;
		}

		// The team's shared plan: about a third defend, the carrier gets escorts, a missing flag gets recoverers or
		// interceptors, the rest attack. Every bot reads the same plan, so they don't all chase one objective.
		void assignCtfRoles(int team, const World& w, int* roles)
		{
			for (int i = 0; i < MAX_PLAYER; ++i)
				roles[i] = NO_ROLE;
			std::vector<int> roster, live;
			for (int i = 0; i < MAX_PLAYER; ++i)
				if (w.isBot[i] && w.team[i] == team)
				{
					roster.push_back(i);
					if (w.alive[i])
						live.push_back(i);
				}
			if (live.empty())
				return;
			const int own = ownFlag(team), enemy = 1 - own;
			const int n = (int)roster.size();
			const int defenders = n <= 1 ? 0 : std::min(std::max((n + 2) / 3, 1), std::min(3, n - 1));
			auto natural = [&](int slot)
			{ return std::find(roster.begin(), roster.begin() + defenders, slot) != roster.begin() + defenders; };
			bool anyDefender = false;
			for (int s : live)
				if (natural(s))
				{
					roles[s] = DEFENDER;
					anyDefender = true;
				}
			if (defenders > 0 && !anyDefender)
				roles[live[0]] = DEFENDER;
			const int carrier = w.flagState[enemy];
			if (carrier >= 0)
			{
				if (std::find(live.begin(), live.end(), carrier) != live.end())
					roles[carrier] = CARRIER;
				int escorts = w.flagState[own] == -1 ? 1 : tuning.carrierEscorts;
				for (int s : live)
					if (escorts > 0 && s != carrier && !natural(s))
					{
						roles[s] = ESCORTER;
						--escorts;
					}
			}
			if (w.flagState[own] == -1 || w.flagState[own] >= 0)
			{
				const int role = w.flagState[own] == -1 ? RECOVERER : INTERCEPTOR;
				int count = w.flagState[own] == -1 ? tuning.missingFlagRecoverers : 2;
				for (int s : live)
					if (count > 0 && !natural(s) && roles[s] == NO_ROLE)
					{
						roles[s] = role;
						--count;
					}
			}
			if (w.defendPressure[own])
				for (int s : live)
					if (roles[s] == ATTACKER || roles[s] == NO_ROLE)
					{
						roles[s] = DEFENDER;
						break;
					}
			for (int s : live)
				if (roles[s] == NO_ROLE)
					roles[s] = ATTACKER;
		}

		V2 pressureSpawnGoal(const std::vector<CVector3f>& spawns, const CVector3f& carrier, const CVector3f& pod)
		{
			const V2 mid{(carrier[0] + pod[0]) * 0.5f, (carrier[1] + pod[1]) * 0.5f};
			V2 best = xy(pod);
			float bestD = 1e9f;
			for (const CVector3f& s : spawns)
				if (dist(mid, s) < bestD)
				{
					bestD = dist(mid, s);
					best = xy(s);
				}
			return best;
		}

		V2 saferCarrierGoal(const CVector3f& carrier, const CVector3f& pod, const std::vector<CVector3f>& spawns, float pressure)
		{
			const float dx = pod[0] - carrier[0], dy = pod[1] - carrier[1], l = std::max(0.01f, len(dx, dy));
			if (l <= tuning.carrierDirectCapture)
				return xy(pod);
			const V2 candidates[3] = {xy(pod),
									  {carrier[0] + dx * 0.5f - dy / l * 2, carrier[1] + dy * 0.5f + dx / l * 2},
									  {carrier[0] + dx * 0.5f + dy / l * 2, carrier[1] + dy * 0.5f - dx / l * 2}};
			V2 best = candidates[0];
			float bestScore = 1e9f;
			for (const V2& c : candidates)
			{
				const float score = dist(c, carrier) + spawnDanger(c, spawns, pressure) * 4;
				if (score < bestScore)
				{
					bestScore = score;
					best = c;
				}
			}
			return best;
		}

		int desiredState(int slot, int team, float life, bool combat, const World& w, int role, bool& urgent)
		{
			float u[STATE_COUNT] = {};
			u[WANDER] = w.ctf ? 0 : tuning.utilityWander;
			u[ATTACK] = combat ? tuning.utilityAttack * (0.5f + clampf(life, 0, 1)) : 0;
			u[ATTACK_OBJECTIVE] = w.ctf && (role == ATTACKER || role == DEFENDER) ? tuning.utilityObjective : 0;
			u[RETREAT] = std::max(0.0f, 0.55f - life) * tuning.utilityRetreatHealth;
			if (w.ctf)
			{
				const int enemy = 1 - ownFlag(team);
				if (role == CARRIER || w.flagState[enemy] == slot)
					u[CAPTURE] = tuning.utilityCapture;
				if (role == RECOVERER || role == INTERCEPTOR)
					u[RECOVER] = tuning.utilityRecover;
				if (role == ESCORTER)
					u[ESCORT] = tuning.utilityEscort;
			}
			int best = 0;
			for (int i = 1; i < STATE_COUNT; ++i)
				if (u[i] > u[best])
					best = i;
			urgent = best == CAPTURE || best == RECOVER || (best == RETREAT && life < 0.3f);
			return best;
		}

		void updateState(Brain& b, int desired, bool urgent, float dt)
		{
			b.committedFor += dt;
			if (desired == b.state)
				return;
			if (urgent || b.committedFor >= tuning.minCommitment)
			{
				b.state = desired;
				b.committedFor = 0;
				++b.stateChanges;
			}
		}

		void updateRole(Brain& b, int desired, bool urgent, float dt)
		{
			b.roleCommittedFor += dt;
			if (desired == b.role)
				return;
			if (urgent || b.roleCommittedFor >= tuning.minCommitment)
			{
				b.role = desired;
				b.roleCommittedFor = 0;
			}
		}

		float weaponUtility(int weaponID, float d, bool reloading)
		{
			d = std::max(0.0f, d);
			float range;
			switch (weaponID)
			{
				case WEAPON_SHOTGUN:
					range = clampf(1 - d / 5, 0.05f, 1);
					break;
				case WEAPON_SNIPER:
					range = clampf(d / 10, 0.15f, 1);
					break;
				case WEAPON_BAZOOKA:
					range = clampf(1 - std::fabs(d - 7) / 8, 0.1f, 1);
					break;
				case WEAPON_FLAME_THROWER:
					range = clampf(1 - d / 3, 0.05f, 1);
					break;
				default:
					range = clampf(1 - std::fabs(d - 5) / 14, 0.25f, 1);
			}
			return range * (reloading ? 0.2f : 1.0f);
		}

		bool findCoverGoal(Map& map, const CVector3f& from, const CVector3f& threat, V2& out)
		{
			const int ox = (int)std::floor(from[0]), oy = (int)std::floor(from[1]);
			float bestScore = 1e9f;
			bool found = false;
			for (int r = 1; r <= 4 && !found; ++r)
				for (int y = oy - r; y <= oy + r; ++y)
					for (int x = ox - r; x <= ox + r; ++x)
					{
						if (x != ox - r && x != ox + r && y != oy - r && y != oy + r)
							continue;
						if (!passable(map, x, y))
							continue;
						const CVector3f c(x + 0.5f, y + 0.5f, 0.25f);
						const CVector3f wall = rayEnd(map, CVector3f(threat[0], threat[1], 0.25f), c);
						const float threatD = dist(c, threat);
						if (dist(wall, threat) + 0.1f >= threatD)
							continue; // visible from the threat
						const float score = dist(c, from) - threatD * 0.15f;
						if (score < bestScore)
						{
							bestScore = score;
							out = xy(c);
							found = true;
						}
					}
			return found;
		}

		V2 flankGoal(int slot, const CVector3f& from, const CVector3f& target, int weaponID)
		{
			const float dx = target[0] - from[0], dy = target[1] - from[1], d = std::max(0.01f, len(dx, dy));
			const float nx = dx / d, ny = dy / d;
			const float side = ((slot / 2) & 1) == 0 ? 1.0f : -1.0f; // alternate within a team
			float standoff = 2.5f, lateral = (1.1f + (slot % 3) * 0.35f) * side;
			if (weaponID == WEAPON_FLAME_THROWER)
			{
				standoff = 1.5f;
				lateral = 0.35f * side;
			}
			else if (weaponID == WEAPON_BAZOOKA)
				standoff = 5.0f;
			return {target[0] - nx * standoff - ny * lateral, target[1] - ny * standoff + nx * lateral};
		}

		V2 humanAim(Brain& b, V2 current, V2 desired, float dt, int personality)
		{
			if (!b.aimSet)
			{
				b.aimPoint = current;
				b.aimSet = true;
			}
			const float responsiveness = 5.5f + (personality % 5) * 0.45f;
			const float maxSpeed = 12.0f + (personality % 3) * 1.25f;
			const float maxAccel = 32.0f + (personality % 4) * 3.0f;
			float* point[2] = {&b.aimPoint.x, &b.aimPoint.y};
			float* velocity[2] = {&b.aimVelocity.x, &b.aimVelocity.y};
			const float want[2] = {desired.x, desired.y};
			for (int a = 0; a < 2; ++a)
			{
				const float wanted = clampf((want[a] - *point[a]) * responsiveness, -maxSpeed, maxSpeed);
				*velocity[a] += clampf(wanted - *velocity[a], -maxAccel * dt, maxAccel * dt);
				*point[a] += *velocity[a] * dt;
			}
			return b.aimPoint;
		}

		V2 recoveryHeading(Map& map, const CVector3f& from, V2 preferred, V2 forward, float side)
		{
			const V2 f = xy(from);
			const float fl = len(forward.x, forward.y);
			if (fl <= 0.001f)
				return unstuckHeading(map, f, preferred);
			const float sign = side >= 0 ? 1.0f : -1.0f;
			const float fx = forward.x / fl, fy = forward.y / fl, nx = -fy * sign, ny = fx * sign;
			const V2 sidestep{f.x + nx * 1.5f, f.y + ny * 1.5f}, sidestep2{f.x - nx * 1.5f, f.y - ny * 1.5f};
			const V2 forwardStep{f.x + fx * 1.5f, f.y + fy * 1.5f};
			const V2 reverseStep{f.x - fx * tuning.escapeStep, f.y - fy * tuning.escapeStep};
			const CVector3f p1(f.x, f.y, 0.25f), p2(f.x + fx * 0.6f, f.y + fy * 0.6f, 0.25f);
			const bool wallAhead = segmentHitT(p1, p2, rayEnd(map, p1, p2)) < 0.98f;
			const V2 order[2][4] = {{forwardStep, sidestep, sidestep2, reverseStep}, {sidestep, reverseStep, sidestep2, forwardStep}};
			for (const V2& c : order[wallAhead])
			{
				const V2 h = unstuckHeading(map, f, c);
				if (dist(h, f) > 0.03f)
					return h;
			}
			return unstuckHeading(map, f, preferred);
		}

		void resolveStuck(Brain& b, bool carrying)
		{
			++b.recoveryTries;
			if (carrying)
			{
				b.recoveryTimer = 0.45f;
				b.recoveryDir = -b.recoveryDir;
				return;
			}
			b.recoveryTimer = 0.30f;
			if (b.recoveryTries > 3)
			{
				b.recoveryDir = -b.recoveryDir;
				b.recoveryTries = 1;
			}
		}

		bool consumeBurstShot(Brain& b, int slot)
		{
			if (b.burstPause > 0)
				return false;
			if (b.burstLeft == 0)
				b.burstLeft = 3 + slot % 4;
			if (--b.burstLeft == 0)
				b.burstPause = 0.18f + (slot % 5) * 0.035f;
			return true;
		}

		float arrivalRadius(int role, int state)
		{
			// Flag take and return use a 0.25 radius: stop inside it
			return role == DEFENDER || state == ATTACK_OBJECTIVE || state == CAPTURE || state == RECOVER ? 0.12f : 0.40f;
		}

		V2 defenderGoal(const World& w, int team, int slot, float angle, Map& map)
		{
			const int own = ownFlag(team);
			const CVector3f& pod = w.flagPod[own];
			const V2 approach = w.hasPredictedSpawn[own] ? w.predictedEnemySpawn[own] : xy(w.flagPod[1 - own]);
			const float lx = approach.x - pod[0], ly = approach.y - pod[1], l = std::max(0.01f, len(lx, ly));
			const V2 forward{lx / l, ly / l}, lateral{-forward.y, forward.x};
			// Both shoulders of the approach, no centre post: that is the carrier's scoring lane
			const int rank = slot / 2;
			const float side = (rank & 1) == 0 ? 1.0f : -1.0f;
			const float lane = side * (1.35f + (rank / 2) * 0.3f), advance = 1.25f + (rank % 2) * 0.35f;
			const float phase = angle * 0.85f + rank * 1.7f, pf = std::cos(phase) * 0.48f, pl = std::sin(phase) * 0.68f;
			return resolveGoal(map, {pod[0] + forward.x * (advance + pf) + lateral.x * (lane + pl),
									 pod[1] + forward.y * (advance + pf) + lateral.y * (lane + pl)});
		}

		V2 guardSafe(const CVector3f& point, const CVector3f& fallback, const Map& map)
		{
			return std::isfinite(point[0]) && std::isfinite(point[1]) && flagVisible(map, point) ? xy(point) : xy(fallback);
		}

		V2 separation(int slot, const CVector3f& p, const World& w)
		{
			V2 f;
			for (int o = 0; o < MAX_PLAYER; ++o)
			{
				if (o == slot || !w.alive[o])
					continue;
				const float dx = p[0] - w.pos[o][0], dy = p[1] - w.pos[o][1], d = len(dx, dy);
				if (d >= tuning.separationRadius)
					continue;
				const float strength = (tuning.separationRadius - d) / tuning.separationRadius;
				if (d > 0.001f)
				{
					f.x += dx / d * strength;
					f.y += dy / d * strength;
				}
				else
				{
					const float a = (slot * 37 + o * 17) * PI_F / 180;
					f.x += std::cos(a) * strength;
					f.y += std::sin(a) * strength;
				}
			}
			return f;
		}

		V2 wanderGoal(float angle, const CVector3f& p)
		{
			return {p[0] + std::cos(angle) * 6, p[1] + std::sin(angle) * 6};
		}

		V2 escortPathGoal(int slot, const CVector3f& carrier, const CVector3f& pod)
		{
			const float dx = pod[0] - carrier[0], dy = pod[1] - carrier[1], d = std::max(0.01f, len(dx, dy));
			const float nx = dx / d, ny = dy / d;
			const int formation = slot % 4;
			const float offset = 0.9f + (formation / 2) * 0.45f, sign = formation % 2 == 0 ? 1.0f : -1.0f;
			const float lead = std::min(std::min(d * 0.7f, 2.8f), std::max(0.0f, d - 0.55f));
			return {carrier[0] + nx * lead - ny * sign * offset, carrier[1] + ny * lead + nx * sign * offset};
		}

		// CTF priority: capture > return own flag > take enemy flag > chase the carrier
		V2 ctfGoal(int slot, int team, const World& w, const std::vector<CVector3f>& spawns, bool pressure)
		{
			const int own = ownFlag(team), enemy = 1 - own;
			const CVector3f &ownPod = w.flagPod[own], &enemyPod = w.flagPod[enemy];
			if (w.flagState[enemy] == slot)
				return guardSafe(ownPod, enemyPod, *w.map);
			if (pressure)
				return pressureSpawnGoal(spawns, w.flagPos[enemy], ownPod);
			if (w.flagState[own] == -2 && w.flagState[enemy] >= 0)
				return escortPathGoal(slot, w.flagPos[enemy], ownPod);
			if (w.flagState[own] == -1)
				return guardSafe(w.flagPos[own], ownPod, *w.map);
			if (w.flagState[enemy] == -1)
				return guardSafe(w.flagPos[enemy], enemyPod, *w.map);
			if (w.flagState[enemy] == -2)
				return guardSafe(enemyPod, ownPod, *w.map);
			if (w.flagState[own] >= 0)
				return guardSafe(w.flagPos[own], enemyPod, *w.map);
			return xy(enemyPod);
		}
	}

	const char* stateName(int state)
	{
		static const char* names[] = {"wander", "attack", "attack_objective", "capture", "escort", "recover", "retreat"};
		return state >= 0 && state < STATE_COUNT ? names[state] : "?";
	}

	const char* roleName(int role)
	{
		static const char* names[] = {"attacker", "defender", "escort", "recoverer", "interceptor", "carrier"};
		return role >= 0 && role < ROLE_COUNT ? names[role] : "?";
	}

	bool passable(const Map& map, int x, int y)
	{
		return x >= 0 && y >= 0 && x < map.size[0] && y < map.size[1] && map.cells[y * map.size[0] + x].passable;
	}

	bool canSee(Map& map, const CVector3f& from, const CVector3f& target, float range)
	{
		const float d = dist(from, target);
		if (d <= 0.001f || d > range)
			return false;
		const CVector3f a(from[0], from[1], 0.25f), b(target[0], target[1], 0.25f);
		return dist(rayEnd(map, a, b), a) + 1e-3f >= d;
	}

	// A goal on a wall moves to the nearest tile centre a player fits on
	V2 resolveGoal(Map& map, V2 goal)
	{
		if (hasClearance(map, goal, 0.30f))
			return goal;
		const int gx = (int)std::floor(goal.x), gy = (int)std::floor(goal.y);
		const int maxR = std::max(map.size[0], map.size[1]);
		for (int r = 0; r <= maxR; ++r)
		{
			float bestD = 1e9f;
			V2 best;
			for (int y = gy - r; y <= gy + r; ++y)
				for (int x = gx - r; x <= gx + r; ++x)
				{
					if (r > 0 && x != gx - r && x != gx + r && y != gy - r && y != gy + r)
						continue;
					const V2 p{x + 0.5f, y + 0.5f};
					if (!hasClearance(map, p, 0.30f))
						continue;
					if (dist(p, goal) < bestD)
					{
						bestD = dist(p, goal);
						best = p;
					}
				}
			if (bestD < 1e9f)
				return best;
		}
		return goal;
	}

	// Straight to the goal when the way is clear, else to the next tile of a breadth-first route
	V2 navigateToward(Map& map, V2 from, V2 goal)
	{
		goal = resolveGoal(map, goal);
		if (segmentHasClearance(map, from, goal, 0.30f))
			return goal;
		int nx, ny;
		if (bfsNextTile(map, (int)std::floor(from.x), (int)std::floor(from.y), (int)std::floor(goal.x), (int)std::floor(goal.y), nx, ny))
			return {nx + 0.5f, ny + 0.5f};
		return unstuckHeading(map, from, goal);
	}

	World buildWorld(Game& game)
	{
		World w;
		w.game = &game;
		w.map = game.map;
		w.gameType = game.gameType;
		for (int i = 0; i < MAX_PLAYER; ++i)
		{
			w.team[i] = PLAYER_TEAM_SPECTATOR;
			w.shotAge[i] = 1e9f;
			const Player* p = game.players[i];
			if (!p)
				continue;
			w.isBot[i] = p->isBot;
			w.team[i] = p->teamID;
			w.alive[i] = p->status == PLAYER_STATUS_ALIVE;
			w.pos[i] = p->currentCF.position;
			w.vel[i] = p->currentCF.vel;
			w.shotAge[i] = p->mfElapsedSinceLastShot;
		}
		if (!w.map)
			return w;
		w.ctf = game.gameType == GAME_TYPE_CTF && w.map->flagPodPos[0] != w.map->flagPodPos[1];
		for (int f = 0; f < 2; ++f)
		{
			w.flagState[f] = w.map->flagState[f];
			w.flagPos[f] = w.map->flagPos[f];
			w.flagPod[f] = w.map->flagPodPos[f];
		}
		for (int t = 0; t < 2; ++t)
		{
			for (int i = 0; i < MAX_PLAYER; ++i)
				w.roles[t][i] = NO_ROLE;
			const int team = t == 0 ? PLAYER_TEAM_BLUE : PLAYER_TEAM_RED;
			w.hasPredictedSpawn[t] = predictedEnemySpawn(team, w, w.predictedEnemySpawn[t]);
			for (int i = 0; i < MAX_PLAYER; ++i)
			{
				const Player* p = game.players[i];
				if (p && p->teamID != team && (p->teamID == PLAYER_TEAM_BLUE || p->teamID == PLAYER_TEAM_RED) &&
					p->status == PLAYER_STATUS_DEAD && p->timeToSpawn > 0)
					w.enemyRecentlyDead[t] = true;
			}
			if (w.ctf)
			{
				w.defendPressure[t] = defenderPressure(team, w);
				assignCtfRoles(team, w, w.roles[t]);
			}
		}
		return w;
	}

	Action think(Brain& b, const World& w, int slot, const CVector3f& myPos, const Player& self, float dt)
	{
		Map& map = *w.map;
		Action act;
		const int myTeam = self.teamID;
		const int own = ownFlag(myTeam), enemyFlag = 1 - own;
		const int weaponID = self.weapon ? self.weapon->weaponID : WEAPON_SMG;
		b.fireCooldown -= dt;
		b.grenadeWait -= dt;
		b.molotovWait -= dt;
		b.meleeWait -= dt;
		b.burstPause = std::max(0.0f, b.burstPause - dt);
		b.goalRefresh -= dt;
		b.wanderAngle += dt * (1.1f + slot * 0.013f);

		// Role from the team plan
		const int assigned = w.ctf && w.roles[own][slot] != NO_ROLE ? w.roles[own][slot] : ATTACKER;
		const bool coordinated = assigned == CARRIER || assigned == ESCORTER || assigned == RECOVERER || assigned == INTERCEPTOR ||
								 b.role == CARRIER || b.role == ESCORTER || b.role == RECOVERER || b.role == INTERCEPTOR;
		updateRole(b, assigned, coordinated, dt);
		b.spawnPressure = w.enemyRecentlyDead[own] ? 1.0f : std::max(0.0f, b.spawnPressure - dt * 0.35f);
		const bool carrying = w.ctf && w.flagState[enemyFlag] == slot;
		const bool escorting = b.role == ESCORTER && w.ctf && w.flagState[own] == -2 && w.flagState[enemyFlag] >= 0 && w.flagState[enemyFlag] != slot;
		const bool pressure = w.ctf && w.defendPressure[own];

		// Perception: the nearest visible enemy (keeping the current one unless another is much closer), and shots heard
		int visible = -1, heard = -1;
		float visibleD = 0, visibleScore = 1e9f, heardD = 1e9f;
		for (int o = 0; o < MAX_PLAYER; ++o)
		{
			if (o == slot || !w.alive[o] || !canDamage(w.gameType, myTeam, w.team[o]))
				continue;
			const float d = dist(w.pos[o], myPos);
			if (d > tuning.aimRange)
				continue;
			if (d <= tuning.hearingRange && w.shotAge[o] <= tuning.audibleShotAge && d < heardD)
			{
				heard = o;
				heardD = d;
			}
			if (!canSee(map, myPos, w.pos[o], tuning.aimRange))
				continue;
			const float score = d * (b.perceivedTarget == o ? 0.72f : 1.0f);
			if (score < visibleScore)
			{
				visibleScore = score;
				visible = o;
				visibleD = d;
			}
		}
		b.perceptionElapsed += dt;
		if (b.perceptionElapsed >= tuning.perceptionInterval)
		{
			const float pdt = b.perceptionElapsed;
			b.perceptionElapsed = 0;
			if (visible >= 0)
			{
				if (b.perceivedTarget == visible)
					b.targetVisibleFor += pdt;
				else
				{
					b.perceivedTarget = visible;
					b.targetVisibleFor = 0;
				}
				b.lastKnownEnemy = xy(w.pos[visible]);
				b.enemyMemoryLeft = tuning.enemyMemory;
			}
			else if (heard >= 0)
			{
				b.perceivedTarget = heard;
				b.lastKnownEnemy = xy(w.pos[heard]);
				b.enemyMemoryLeft = tuning.enemyMemory;
				b.targetVisibleFor = 0;
			}
			else
			{
				b.targetVisibleFor = 0;
				b.enemyMemoryLeft = std::max(0.0f, b.enemyMemoryLeft - pdt);
				if (b.enemyMemoryLeft == 0)
					b.perceivedTarget = -1;
			}
		}
		// Shoot only at an enemy visible this frame and acquired
		const int combat = visible >= 0 && b.perceivedTarget == visible && b.targetVisibleFor >= tuning.acquireTime ? visible : -1;
		// Outside CTF, hunt the nearest enemy even unseen, so teams don't stay in separate rooms
		int hunt = -1;
		float huntD = 1e9f;
		for (int o = 0; o < MAX_PLAYER; ++o)
			if (o != slot && w.alive[o] && canDamage(w.gameType, myTeam, w.team[o]) && dist(w.pos[o], myPos) < huntD)
			{
				huntD = dist(w.pos[o], myPos);
				hunt = o;
			}

		bool urgent;
		const int desired = desiredState(slot, myTeam, self.life, combat >= 0, w, b.role, urgent);
		updateState(b, desired, urgent, dt);

		// Goal
		const bool staleGoal = b.goalStaleFor > tuning.goalStaleTime && b.stuckTime > 0;
		if (b.goalRefresh <= 0 || staleGoal || carrying || escorting || b.role == DEFENDER)
		{
			const std::vector<CVector3f>& spawns = enemySpawnPoints(myTeam, map);
			if (b.state == RETREAT)
			{
				if (combat >= 0)
				{
					const CVector3f& t = w.pos[combat];
					if (!findCoverGoal(map, myPos, t, b.goal))
						b.goal = {myPos[0] * 2 - t[0], myPos[1] * 2 - t[1]};
					// Cornered (the escape lands on this spot): fight back instead of standing still. Not in bots.rs,
					// whose maps rarely corner a bot; the walled placeholder arenas do.
					if (dist(resolveGoal(map, b.goal), xy(myPos)) < 1.0f)
						b.goal = flankGoal(slot, myPos, t, weaponID);
				}
				else
					b.goal = wanderGoal(b.wanderAngle + PI_F, myPos);
			}
			else if (b.state == ATTACK)
			{
				if (b.role == DEFENDER && w.ctf)
				{
					const V2 post = defenderGoal(w, myTeam, slot, b.wanderAngle, map);
					b.goal = post;
					if (combat >= 0 && dist(w.pos[combat], w.flagPod[own]) <= tuning.defendPressureRange)
					{
						// The defender nearest the intruder responds; the others hold their posts
						int responder = -1;
						float best = 1e9f;
						for (int o = 0; o < MAX_PLAYER; ++o)
							if (w.roles[own][o] == DEFENDER && w.alive[o] && dist(w.pos[o], w.pos[combat]) < best)
							{
								best = dist(w.pos[o], w.pos[combat]);
								responder = o;
							}
						if (responder == slot)
							b.goal = flankGoal(slot, myPos, w.pos[combat], weaponID);
					}
				}
				else if (combat >= 0)
					b.goal = flankGoal(slot, myPos, w.pos[combat], weaponID);
				else
					b.goal = b.lastKnownEnemy;
			}
			else if (b.state == RECOVER)
			{
				const int carrier = w.flagState[own];
				if (carrier >= 0 && carrier < MAX_PLAYER && w.alive[carrier])
					b.goal = {w.pos[carrier][0] + w.vel[carrier][0] * 1.25f, w.pos[carrier][1] + w.vel[carrier][1] * 1.25f};
				else if (w.ctf)
					b.goal = xy(w.flagPos[own]);
			}
			else if (w.ctf)
			{
				switch (b.role)
				{
					case CARRIER:
						b.goal = saferCarrierGoal(myPos, w.flagPod[own], spawns, std::max(0.25f, b.spawnPressure));
						break;
					case DEFENDER:
						b.goal = defenderGoal(w, myTeam, slot, b.wanderAngle, map);
						break;
					case ESCORTER:
						if (w.flagState[enemyFlag] >= 0)
							b.goal = escortPathGoal(slot, w.flagPos[enemyFlag], w.flagPod[own]);
						else if (w.flagState[own] == -1)
							b.goal = guardSafe(w.flagPos[own], w.flagPod[own], map);
						else
							b.goal = ctfGoal(slot, myTeam, w, spawns, pressure);
						break;
					case ATTACKER:
						b.goal = xy(w.flagPod[enemyFlag]);
						break;
					default:
						b.goal = ctfGoal(slot, myTeam, w, spawns, pressure);
				}
			}
			else if (combat >= 0)
				b.goal = xy(w.pos[combat]);
			else if (b.enemyMemoryLeft > 0)
				b.goal = b.lastKnownEnemy;
			else if (hunt >= 0)
				b.goal = xy(w.pos[hunt]);
			else
				b.goal = wanderGoal(b.wanderAngle, myPos);
			b.goalRefresh = tuning.goalRefresh;
			b.goalStaleFor = 0;
		}
		else
			b.goalStaleFor += dt;

		// Navigation: keep a waypoint until it is reached, the goal moves, or progress stops
		const V2 navGoal = resolveGoal(map, b.goal);
		const V2 me = xy(myPos);
		b.navWaypointTime = std::max(0.0f, b.navWaypointTime - dt);
		const bool goalChanged = !b.hasNavGoal || dist(navGoal, b.navGoal) > (b.role == DEFENDER ? 0.02f : 0.75f);
		const bool reached = dist(b.navWaypoint, me) < 0.30f;
		const int tile[2] = {(int)std::floor(me.x), (int)std::floor(me.y)};
		const bool leftTile = tile[0] != b.navFromTile[0] || tile[1] != b.navFromTile[1];
		if (goalChanged || reached || leftTile || b.navWaypointTime <= 0)
		{
			if (goalChanged || reached || leftTile)
				b.stuckTime = 0;
			b.navGoal = navGoal;
			b.hasNavGoal = true;
			b.navFromTile[0] = tile[0];
			b.navFromTile[1] = tile[1];
			b.navWaypoint = navigateToward(map, me, navGoal);
			b.navWaypointTime = tuning.navWaypointCommit;
			b.navBestDistance = dist(b.navWaypoint, me);
			b.navNoProgressTime = 0;
		}
		const float waypointD = dist(b.navWaypoint, me);
		const bool atGoal = dist(b.navGoal, me) < arrivalRadius(b.role, b.state);
		const bool progress = atGoal || waypointD + tuning.navProgressEpsilon < b.navBestDistance;
		if (progress)
		{
			b.navBestDistance = waypointD;
			b.navNoProgressTime = 0;
		}
		else
			b.navNoProgressTime += dt;
		const bool navStuck = b.navNoProgressTime >= tuning.navNoProgressTime;
		act.lookAt = navGoal;

		// Combat
		if (combat >= 0)
		{
			const CVector3f& t = w.pos[combat];
			const float d = dist(t, myPos);
			// A slow, stable error: no perfect aim and no frame-to-frame jitter
			const float error = std::sin(b.wanderAngle) * tuning.aimError * (0.75f + (slot % 7) * 0.08f) * d;
			const float inv = 1 / std::max(0.01f, d);
			const V2 imperfect{t[0] - (t[1] - myPos[1]) * inv * error, t[1] + (t[0] - myPos[0]) * inv * error};
			const V2 aim = humanAim(b, xy(self.currentCF.mousePosOnMap), imperfect, dt, slot);
			const bool reloading = self.weapon && self.mfElapsedSinceLastShot < self.weapon->fireDelay;
			const float ready = weaponUtility(weaponID, d, reloading);
			const unsigned sequence = b.shotsFired + slot;
			const bool weaponReady = self.weapon && self.mfElapsedSinceLastShot >= self.weapon->fireDelay;
			if (d < 1.0f && b.meleeWait <= 0 && self.meleeWeapon)
			{
				act.fire = Action::MELEE;
				b.meleeWait = std::max(1.0f, self.meleeWeapon->fireDelay);
			}
			else if (d >= 1.5f && d <= 5.0f && self.nbMolotovLeft > 0 && b.molotovWait <= 0 && gameVar.sv_enableMolotov && sequence % 6 == 1)
			{
				act.fire = Action::MOLOTOV;
				b.molotovWait = 1.0f;
			}
			else if (d >= 2.0f && d <= 6.5f && self.nbGrenadeLeft > 0 && b.grenadeWait <= 0 && sequence % 4 == 0)
			{
				act.fire = Action::GRENADE;
				b.grenadeWait = 1.0f;
			}
			else if (b.fireCooldown <= 0 && weaponReady && ready > 0.05f && (weaponID == WEAPON_FLAME_THROWER || consumeBurstShot(b, slot)))
			{
				// The line to the target must be clear of walls up to the target
				const CVector3f p1(myPos[0], myPos[1], 0.25f);
				const CVector3f p2(p1[0] + (t[0] - p1[0]) * inv * 40, p1[1] + (t[1] - p1[1]) * inv * 40, 0.25f);
				float hitT;
				if (segmentCircleT(p1, p2, t, PLAYER_RADIUS, hitT) && hitT <= segmentHitT(p1, p2, rayEnd(map, p1, p2)) + 1e-3f)
				{
					act.fire = weaponID == WEAPON_BAZOOKA ? Action::ROCKET : Action::SHOOT;
					b.fireCooldown = 0.11f;
					++b.shotsFired;
				}
				else
					++b.occludedShots;
			}
			act.fireAt = aim;
			act.lookAt = aim;
		}

		// Steering, with recovery when stuck
		V2 steer = atGoal ? me : waypointD < 0.15f ? navigateToward(map, me, navGoal)
												   : b.navWaypoint;
		const V2 forward{navGoal.x - me.x, navGoal.y - me.y};
		if (b.recoveryTimer > 0)
		{
			b.recoveryTimer -= dt;
			steer = recoveryHeading(map, myPos, navGoal, forward, b.recoveryDir);
		}
		if (navStuck && b.recoveryTimer <= 0)
		{
			resolveStuck(b, carrying);
			b.goalRefresh = 0;
			b.goalStaleFor = 0;
			b.navWaypointTime = 0;
			b.navNoProgressTime = 0;
			b.navBestDistance = 1e9f;
			++b.stuckEvents;
			steer = recoveryHeading(map, myPos, navGoal, forward, b.recoveryDir);
		}
		if (b.stuckTime > tuning.stuckTime && !atGoal)
		{
			resolveStuck(b, carrying);
			b.goalRefresh = 0;
			steer = recoveryHeading(map, myPos, navGoal, forward, b.recoveryDir);
			b.stuckTime = 0;
			++b.stuckEvents;
		}

		float dx = steer.x - me.x, dy = steer.y - me.y;
		const V2 sep = separation(slot, myPos, w);
		float sepWeight = tuning.separationWeight;
		if ((b.state == ATTACK_OBJECTIVE || b.state == RECOVER) && dist(me, navGoal) <= 1.0f)
			sepWeight = 0; // reach the flag
		if (carrying)
			sepWeight = dist(me, w.flagPod[own]) <= tuning.carrierDirectCapture ? 0 : 1.8f;
		dx += sep.x * sepWeight;
		dy += sep.y * sepWeight;
		if (combat >= 0 && !carrying && !escorting)
		{
			// Circle the target in alternating arcs instead of charging in a line
			const float speed = weaponID == WEAPON_FLAME_THROWER ? 0.2f : 0.75f;
			const float orbit = (((int)(b.wanderAngle / 3)) & 1) == 0 ? speed : -speed;
			const float base = std::max(0.05f, len(dx, dy));
			const float fx = dx, fy = dy;
			dx += -fy / base * orbit;
			dy += fx / base * orbit;
		}
		const float l = std::max(0.05f, len(dx, dy));
		act.velocity = {dx / l * tuning.speed, dy / l * tuning.speed};
		if (atGoal && combat < 0 && sep.x == 0 && sep.y == 0)
			act.velocity = {0, 0};
		return act;
	}
}

void Bot::send(Server& server, int typeID, const void* data, int size)
{
	server.fromBot(slot, typeID, data, size);
}

void Bot::update(Server& server, const bots::World& world, float delay)
{
	Game& game = *server.game;
	Player* p = game.players[slot];
	if (!p || !game.map)
		return;
	if (p->teamID != PLAYER_TEAM_BLUE && p->teamID != PLAYER_TEAM_RED)
	{
		// Moved out (allwatch, rejected): ask to play again once a second, so it never strands a slot
		alive = false;
		spawnWait -= delay;
		if (spawnWait <= 0)
		{
			spawnWait = 1.0f;
			net_clsv_svcl_team_request request = {};
			request.playerID = (int8_t)slot;
			request.teamRequested = (int8_t)team;
			send(server, NET_CLSV_SVCL_TEAM_REQUEST, &request, sizeof(request));
		}
		return;
	}
	team = p->teamID; // an auto-balance move sticks

	if (p->status != PLAYER_STATUS_ALIVE)
	{
		alive = false;
		spawnWait -= delay;
		if (p->status == PLAYER_STATUS_DEAD && p->timeToSpawn <= 0 && spawnWait <= 0 && game.roundState == GAME_PLAYING)
		{
			spawnWait = 1.0f;
			net_clsv_spawn_request spawn = {};
			spawn.playerID = (int8_t)slot;
			// The roster index picks the main weapon (bots.rs): the next enabled one
			const bool enabled[8] = {gameVar.sv_enableSMG, gameVar.sv_enableShotgun, gameVar.sv_enableSniper, gameVar.sv_enableDualMachineGun,
									 gameVar.sv_enableChainGun, gameVar.sv_enableBazooka, gameVar.sv_enablePhotonRifle, gameVar.sv_enableFlameThrower};
			int weapon = WEAPON_SMG;
			for (int i = 0; i < 8; ++i)
				if (enabled[(rosterIndex + i) % 8])
				{
					weapon = (rosterIndex + i) % 8;
					break;
				}
			spawn.weaponID = (int8_t)weapon;
			spawn.meleeID = WEAPON_KNIVES;
			const int skin = rosterIndex % 23 + 1; // skin01..skin23
			memcpy(spawn.skin, "skin", 4);
			spawn.skin[4] = (char)('0' + skin / 10);
			spawn.skin[5] = (char)('0' + skin % 10);
			const uint8_t i = (uint8_t)rosterIndex;
			const uint8_t red[3] = {(uint8_t)(200 + i * 17), 80, 120}, green[3] = {80, (uint8_t)(200 + i * 23), 100}, blue[3] = {100, 80, (uint8_t)(200 + i * 29)};
			memcpy(spawn.redDecal, red, 3);
			memcpy(spawn.greenDecal, green, 3);
			memcpy(spawn.blueDecal, blue, 3);
			send(server, NET_CLSV_SPAWN_REQUEST, &spawn, sizeof(spawn));
		}
		return;
	}
	if (game.roundState != GAME_PLAYING)
		return;

	if (!alive)
	{
		// Just spawned: take the server's spawn point and start a fresh plan
		alive = true;
		cf = p->currentCF;
		cf.vel.set(0, 0, 0);
		cf.mousePosOnMap = cf.position + CVector3f(1, 0, 0);
		lastCF = cf;
		const bots::Brain old = brain;
		brain = bots::Brain();
		brain.shotsFired = old.shotsFired;
		brain.occludedShots = old.occludedShots;
		brain.stuckEvents = old.stuckEvents;
		brain.stateChanges = old.stateChanges;
		brain.wanderAngle = old.wanderAngle;
	}

	const bots::Action act = bots::think(brain, world, slot, cf.position, *p, delay);

	// Move like a client: velocity, then the map's collision
	lastCF = cf;
	const CVector3f before = cf.position;
	cf.vel.set(act.velocity.x, act.velocity.y, 0);
	cf.position += cf.vel * delay;
	cf.position[2] = 0.25f;
	game.map->performCollision(lastCF, cf, 0.25f);
	game.map->collisionClip(cf, 0.25f);
	cf.mousePosOnMap.set(act.lookAt.x, act.lookAt.y, 0.25f);
	const float moved = distance(CVector3f(before[0], before[1], 0), CVector3f(cf.position[0], cf.position[1], 0));
	if (moved < bots::tuning.stuckThreshold)
		brain.stuckTime += delay;
	else
	{
		brain.recoveryTries = 0;
		brain.stuckTime = 0;
	}
	++frameID;

	if (++sendFrame >= std::max(1, gameVar.sv_minSendInterval))
	{
		sendFrame = 0;
		net_clsv_svcl_player_coord_frame frame = {};
		frame.playerID = (int8_t)slot;
		frame.frameID = (int32_t)frameID;
		for (int a = 0; a < 3; ++a)
		{
			frame.position[a] = (int16_t)(cf.position[a] * 100);
			frame.vel[a] = (char)(cf.vel[a] * 10);
			frame.mousePos[a] = (int16_t)(cf.mousePosOnMap[a] * 100);
		}
		frame.babonetID = (int32_t)p->babonetID;
		frame.camPosZ = 7; // the default camera height
		send(server, NET_CLSV_SVCL_PLAYER_COORD_FRAME, &frame, sizeof(frame));
	}

	if (act.fire == bots::Action::NONE)
		return;
	p = game.players[slot]; // a message may have changed the slot
	if (!p || p->status != PLAYER_STATUS_ALIVE)
		return;
	const CVector3f from = p->currentCF.position;
	CVector3f dir(act.fireAt.x - from[0], act.fireAt.y - from[1], 0);
	if (dir.length() < 0.001f)
		return;
	normalize(dir);
	if (act.fire == bots::Action::SHOOT && p->weapon)
	{
		net_clsv_player_shoot shoot = {};
		shoot.playerID = (int8_t)slot;
		shoot.weaponID = (int8_t)p->weapon->weaponID;
		for (int a = 0; a < 3; ++a)
		{
			shoot.p1[a] = (int16_t)(from[a] * 100);
			shoot.p2[a] = (int16_t)(dir[a] * 100);
		}
		send(server, NET_CLSV_PLAYER_SHOOT, &shoot, sizeof(shoot));
	}
	else if (act.fire == bots::Action::MELEE)
	{
		net_clsv_svcl_player_shoot_melee melee = {};
		melee.playerID = (int8_t)slot;
		send(server, NET_CLSV_SVCL_PLAYER_SHOOT_MELEE, &melee, sizeof(melee));
	}
	else
	{
		net_clsv_svcl_player_projectile projectile = {};
		projectile.playerID = (int8_t)slot;
		projectile.projectileType = act.fire == bots::Action::ROCKET ? PROJECTILE_ROCKET : act.fire == bots::Action::GRENADE ? PROJECTILE_GRENADE
																															 : PROJECTILE_COCKTAIL_MOLOTOV;
		projectile.weaponID = act.fire == bots::Action::ROCKET ? WEAPON_BAZOOKA : act.fire == bots::Action::GRENADE ? WEAPON_GRENADE
																													: WEAPON_COCKTAIL_MOLOTOV;
		for (int a = 0; a < 3; ++a)
		{
			projectile.position[a] = (int16_t)(from[a] * 100);
			projectile.vel[a] = (char)(dir[a] * 10);
		}
		send(server, NET_CLSV_SVCL_PLAYER_PROJECTILE, &projectile, sizeof(projectile));
	}
}
