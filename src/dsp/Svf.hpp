// Zero-delay-feedback state-variable filter (Zavalishin TPT) with optional
// CA3080-style saturation on the integrator states.
#pragma once
#include "Common.hpp"

namespace serge {

// Integrator saturation (CA3080 stage). Linear up to 4 V so normal levels keep the
// filter's Q and gain exact; soft limit at 6 V so overload and self-oscillation stay bounded.
inline float otaSat(float v) {
	float a = std::fabs(v);
	if (a <= 4.f) return v;
	float s = 4.f + 2.f * std::tanh((a - 4.f) * 0.5f);
	return v < 0.f ? -s : s;
}

struct Svf {
	float ic1 = 0.f, ic2 = 0.f;
	float lp = 0.f, bp = 0.f, hp = 0.f;
	float g = 0.f, k = 1.f, a1 = 0.f, a2 = 0.f, a3 = 0.f;

	// fc in Hz, fs the rate this filter runs at, Q > 0.
	void set(float fc, float fs, float Q) {
		fc = clampf(fc, 1.f, 0.45f * fs);
		g = std::tan(PI * fc / fs);
		k = 1.f / std::max(Q, 0.01f);
		a1 = 1.f / (1.f + g * (g + k));
		a2 = g * a1;
		a3 = g * a2;
	}
	void process(float v0, bool nonlinear) {
		float v3 = v0 - ic2;
		float v1 = a1 * ic1 + a2 * v3;
		float v2 = ic2 + a2 * ic1 + a3 * v3;
		ic1 = 2.f * v1 - ic1;
		ic2 = 2.f * v2 - ic2;
		if (nonlinear) { ic1 = otaSat(ic1); ic2 = otaSat(ic2); }
		if (!finite(ic1) || !finite(ic2)) { ic1 = ic2 = 0.f; v1 = v2 = 0.f; }
		lp = v2; bp = v1; hp = v0 - k * v1 - v2;
	}
	void reset() { ic1 = ic2 = lp = bp = hp = 0.f; }
};

} // namespace serge
