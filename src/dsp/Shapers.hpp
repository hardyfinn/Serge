// Static waveshapers: Wave Multipliers, Triple Waveshaper, Ring Modulator.
#pragma once
#include "Common.hpp"

namespace serge {

// --- Wave Multipliers -------------------------------------------------------
inline float vcmTop(float in, float g, bool hi) {
	if (hi) return g * 2.5f * std::tanh(6.f * in / 2.5f);
	static const float n = 1.f / std::tanh(0.4f);
	return g * 2.5f * std::tanh(0.4f * in / 2.5f) * n;
}

inline float softplusW(float z, float w) {
	float u = z / w;
	if (u > 20.f) return z;
	if (u < -20.f) return 0.f;
	return w * std::log1p(std::exp(u));
}
// The diode knee in the folder has to be exactly zero for a clean sine, so subtract
// the value at |x| = 0 of each stage.
inline float vcmMiddle(float in, float c) {
	const float T = 0.8f, w = 0.03f;
	float x = in * (0.3f + 3.9f * c);
	float ax = std::fabs(x), s = x < 0.f ? -1.f : 1.f;
	float F = x;
	for (int k = 1; k <= 6; k++) {
		float th = (2 * k - 1) * T;
		float sd = s * (softplusW(ax - th, w) - softplusW(-th, w));
		F -= 2.f * ((k & 1) ? 1.f : -1.f) * sd;
	}
	return railClip(F * 2.5f / T);
}
inline void vcmBottom(float in, float c, float& out, float& squared) {
	float s = clampf(in / 2.5f, -4.f, 4.f);
	for (int k = 1; k <= 3; k++) {
		float m = clampf(3.f * c - (k - 1), 0.f, 1.f);
		s = (1.f - m) * s + m * (2.f * std::fabs(s) - 1.f);
	}
	out = 2.5f * s;
	squared = 2.5f + 2.5f * std::tanh(20.f * s);
}

// --- Triple Waveshaper ------------------------------------------------------
inline float tws(float x) {
	float u = clampf(x / 2.5f, -1.5f, 1.5f);
	return 2.5f * (std::sin(PI * u) + 0.03f * std::sin(3.f * PI * u));
}

// --- Ring Modulator ---------------------------------------------------------
inline float ring(float sig, float car, float m, float cl) {
	static const float n = 1.f / std::tanh(1.2f);
	float c = std::tanh(2.f * cl * car / 2.5f * 1.2f) * n;
	return sig * ((1.f - m) + m * c);
}

} // namespace serge
