// Small control and utility circuits: ASR, COM, STR, PRC, XFAD, 2VCA, MIX, DCSM, Peak & Trough.
#pragma once
#include "Common.hpp"

namespace serge {

struct Asr {
	SchmittEdge pulse;
	float out[3] = {};
	void process(float in, float pulseIn) {
		if (pulse.process(pulseIn)) {
			out[2] = out[1];
			out[1] = out[0];
			out[0] = clampf(in, -RAIL, RAIL);
		}
	}
};

struct Comparator {
	bool high = false;
	float process(float plus, float minus, float threshold) {
		float d = plus - (minus + threshold);
		if (high && d < -0.005f) high = false;
		else if (!high && d > 0.005f) high = true;
		return high ? PULSE_HI : 0.f;
	}
};

struct Schmitt {
	bool high = false;
	float process(float in) {
		if (!high && in > 0.6f) high = true;
		else if (high && in < -0.6f) high = false;
		return high ? PULSE_HI : 0.f;
	}
};

struct Str {
	Schmitt a, b;
	SchmittEdge setEdge, resetEdge;
	bool rs = false;
	float outA = 0.f, outB = 0.f, outRs = 0.f;
	void process(float inA, float inB) {
		outA = a.process(inA);
		outB = b.process(inB);
		if (setEdge.process(outA)) rs = true;
		if (resetEdge.process(outB)) rs = false;
		outRs = rs ? PULSE_HI : 0.f;
	}
};

inline float processor(const float* in, const float* k, float offset) {
	return railClip(in[0] * k[0] + in[1] * k[1] + in[2] * k[2] + offset);
}

inline float crossfade(float in1, float in2, float x, float g) {
	x = clampf(x, 0.f, 1.f);
	return g * (std::cos(0.5f * PI * x) * in1 + std::sin(0.5f * PI * x) * in2);
}

// Exponential VCA: 12 dB per volt, unity at 5 V, closes below 0.5 V, never more than x2.
inline float vca2Gain(float v) {
	float db = 12.f * (v - 5.f);
	if (v < 0.5f) db -= 200.f * (0.5f - v);
	if (db < -160.f) return 0.f;
	return std::min(std::pow(10.f, db / 20.f), 2.f);
}

inline void panLaw(float pan, float& l, float& r) {
	pan = clampf(pan, 0.f, 1.f);
	l = std::cos(0.5f * PI * pan);
	r = std::sin(0.5f * PI * pan);
}

} // namespace serge
