// Shared constants and helpers for the Serge models. Plain C++17, no Rack includes.
#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace serge {

constexpr float PI = 3.14159265358979f;
constexpr float RAIL = 10.5f;       // op-amp output limit on +/-12 V supplies
constexpr float PULSE_HI = 5.f;     // Serge pulse level

inline float clampf(float x, float lo, float hi) { return std::min(std::max(x, lo), hi); }

// Linear to +/-9.5 V, soft knee up to the +/-10.5 V rail.
inline float railClip(float x) {
	if (x > 9.5f) return 9.5f + std::tanh(x - 9.5f);
	if (x < -9.5f) return -9.5f + std::tanh(x + 9.5f);
	return x;
}

// Every model ends with this: a NaN or infinity resets the state instead of sticking.
inline bool finite(float x) { return std::isfinite(x); }

// Rising-edge detector for Serge pulse inputs: fires when the input rises through +2.5 V,
// re-arms below +2.0 V.
struct SchmittEdge {
	bool high = false;
	bool process(float v) {
		if (!high && v >= 2.5f) { high = true; return true; }
		if (high && v < 2.0f) high = false;
		return false;
	}
	void reset() { high = false; }
};

// One-pole smoother for knob values (5 ms).
struct Smoother {
	float y = 0.f;
	bool init = false;
	float process(float x, float dt) {
		if (!init) { y = x; init = true; return y; }
		y += (x - y) * (1.f - std::exp(-dt / 0.005f));
		return y;
	}
};

// Small, fast PRNG (xoshiro128+). Seeded by the caller.
struct Rng {
	uint32_t s[4] = {0x9e3779b9u, 0x243f6a88u, 0xb7e15162u, 0x12345678u};
	static uint32_t rotl(uint32_t x, int k) { return (x << k) | (x >> (32 - k)); }
	void seed(uint64_t v) {
		// splitmix64 to fill the state
		for (int i = 0; i < 4; i++) {
			v += 0x9e3779b97f4a7c15ull;
			uint64_t z = v;
			z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
			z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
			z ^= z >> 31;
			s[i] = (uint32_t) z | 1u;
		}
	}
	uint32_t next() {
		uint32_t result = s[0] + s[3];
		uint32_t t = s[1] << 9;
		s[2] ^= s[0]; s[3] ^= s[1]; s[1] ^= s[2]; s[0] ^= s[3];
		s[2] ^= t; s[3] = rotl(s[3], 11);
		return result;
	}
	float uniform() { return (next() >> 8) * (1.f / 16777216.f); }   // [0, 1)
	// Approximately Gaussian, unit variance (sum of 4 uniforms).
	float gauss() {
		float a = uniform() + uniform() + uniform() + uniform() - 2.f;
		return a * 1.7320508f;   // var of sum of 4 U(0,1) = 1/3
	}
};

} // namespace serge
