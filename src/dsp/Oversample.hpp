// Polyphase FIR up/down sampling for the nonlinear models. Factor 4 up to 50 kHz,
// 2 up to 100 kHz, otherwise 1. No user-facing controls.
#pragma once
#include "Common.hpp"

namespace serge {

constexpr int OS_MAX = 4;
constexpr int OS_TAPS_PER_PHASE = 32;
constexpr int OS_MAX_TAPS = OS_MAX * OS_TAPS_PER_PHASE;

inline int oversampleFactor(float sampleRate) {
	if (sampleRate <= 50000.f) return 4;
	if (sampleRate <= 100000.f) return 2;
	return 1;
}

struct OsKernel {
	int factor = 1;
	int taps = 1;
	float h[OS_MAX_TAPS] = {1.f};

	void design(int f) {
		factor = f;
		if (f == 1) { taps = 1; h[0] = 1.f; return; }
		taps = f * OS_TAPS_PER_PHASE;
		float fc = 0.42f / f;   // cutoff as a fraction of the oversampled rate (about 20 kHz at 48 kHz)
		float sum = 0.f;
		for (int i = 0; i < taps; i++) {
			float n = i - (taps - 1) * 0.5f;
			float x = 2.f * PI * fc * n;
			float sinc = (std::fabs(n) < 1e-6f) ? 2.f * fc : std::sin(x) / (PI * n);
			// Blackman window
			float w = 0.42f - 0.5f * std::cos(2.f * PI * i / (taps - 1)) + 0.08f * std::cos(4.f * PI * i / (taps - 1));
			h[i] = sinc * w;
			sum += h[i];
		}
		for (int i = 0; i < taps; i++) h[i] /= sum;
	}
};

struct Upsampler {
	float hist[OS_TAPS_PER_PHASE] = {};
	int pos = 0;
	// out[0..factor-1]
	void process(const OsKernel& k, float x, float* out) {
		if (k.factor == 1) { out[0] = x; return; }
		pos = (pos + OS_TAPS_PER_PHASE - 1) % OS_TAPS_PER_PHASE;
		hist[pos] = x;
		for (int p = 0; p < k.factor; p++) {
			float acc = 0.f;
			for (int j = 0; j < OS_TAPS_PER_PHASE; j++)
				acc += k.h[j * k.factor + p] * hist[(pos + j) % OS_TAPS_PER_PHASE];
			out[p] = acc * k.factor;
		}
	}
	void reset() { for (float& v : hist) v = 0.f; }
};

struct Decimator {
	float hist[OS_MAX_TAPS] = {};
	int pos = 0;
	float process(const OsKernel& k, const float* in) {
		if (k.factor == 1) return in[0];
		for (int p = 0; p < k.factor; p++) {
			pos = (pos + k.taps - 1) % k.taps;
			hist[pos] = in[p];
		}
		float acc = 0.f;
		for (int j = 0; j < k.taps; j++) acc += k.h[j] * hist[(pos + j) % k.taps];
		return acc;
	}
	void reset() { for (float& v : hist) v = 0.f; pos = 0; }
};

} // namespace serge
