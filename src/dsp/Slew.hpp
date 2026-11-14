// The slew core shared by the DSG and the SSG: a high-gain follower whose upward and
// downward speeds are limited separately (error amp driving current sources into a cap).
#pragma once
#include "Common.hpp"

namespace serge {

struct SlewCore {
	static constexpr float G = 400000.f;  // follower bandwidth about 64 kHz (see DECISIONS.md)
	float y = 0.f;

	// Advance by dt seconds toward target. Rates in V/s.
	float process(float target, float riseRate, float fallRate, float dt) {
		float e = target - y;
		float want = G * e;
		if (want > riseRate) {
			y += std::min(riseRate * dt, e);
		}
		else if (want < -fallRate) {
			y -= std::min(fallRate * dt, -e);
		}
		else {
			y += e * (1.f - std::exp(-G * dt));
		}
		y = clampf(y, -RAIL, RAIL);
		if (!finite(y)) y = 0.f;
		return y;
	}
};

} // namespace serge
