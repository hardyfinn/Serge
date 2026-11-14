// Noise Source: white, pink, S/H source.
#pragma once
#include "Common.hpp"

namespace serge {

struct NoiseSource {
	Rng rng;
	float b[7] = {};
	float lp = 0.f, lpA = 0.f, lpNorm = 1.f;
	float white = 0.f, pink = 0.f, sh = 2.5f;
	static constexpr float PINK_NORM = 0.3271f;   // measured: makes pink sigma 1 before scaling (tests/test_noise)

	void setSampleRate(float fs) {
		lpA = std::exp(-2.f * PI * 25.f / fs);
		lpNorm = std::sqrt((1.f + lpA) / (1.f - lpA));
	}
	void process() {
		float w = rng.gauss();
		white = clampf(0.8f * w, -2.5f, 2.5f);
		// Paul Kellet's refined pink filter
		b[0] = 0.99886f * b[0] + w * 0.0555179f;
		b[1] = 0.99332f * b[1] + w * 0.0750759f;
		b[2] = 0.96900f * b[2] + w * 0.1538520f;
		b[3] = 0.86650f * b[3] + w * 0.3104856f;
		b[4] = 0.55000f * b[4] + w * 0.5329522f;
		b[5] = -0.7616f * b[5] - w * 0.0168980f;
		float p = b[0] + b[1] + b[2] + b[3] + b[4] + b[5] + b[6] + w * 0.5362f;
		b[6] = w * 0.115926f;
		pink = clampf(0.8f * PINK_NORM * p, -2.5f, 2.5f);
		lp = lpA * lp + (1.f - lpA) * w;
		sh = clampf(2.5f + 1.6f * lpNorm * lp, 0.f, 5.f);
		for (float& v : b) if (!finite(v)) v = 0.f;
		if (!finite(lp)) lp = 0.f;
	}
};

} // namespace serge
