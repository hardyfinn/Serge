// Variable Q VCF, Variable Slope VCF and Resonant Equalizer models.
#pragma once
#include "Svf.hpp"

namespace serge {

inline float expFreq(float baseHz, float volts) {
	float f = baseHz * std::exp2(volts);
	return finite(f) ? f : baseHz;
}

// --- VCFQ ---------------------------------------------------------------------
struct Vcfq {
	Svf svf;
	float ping = 0.f;
	SchmittEdge trig;
	float pingDecay = 0.f;   // per oversampled step
	float k = 1.f;

	static float qFromNorm(float q) { return std::pow(200.f, clampf(q, 0.f, 1.f)); }   // 1 to 200

	// Call once per base sample.
	void setup(float fc, float fsOs, float qNorm, float dtOs) {
		float Q = qFromNorm(qNorm);
		svf.set(fc, fsOs, Q);
		k = 1.f / Q;
		pingDecay = std::exp(-dtOs / 0.0003f);
	}
	void trigger(float trigIn) { if (trig.process(trigIn)) ping = 5.f; }
	// One oversampled step.
	void step(float in, float agcIn) {
		float x = in + agcIn * std::min(1.f, 2.f * k) + ping;
		ping *= pingDecay;
		if (ping < 1e-6f) ping = 0.f;
		svf.process(x, true);
	}
	float low() const { return railClip(svf.lp); }
	float band() const { return railClip(svf.bp); }
	float high() const { return railClip(svf.hp); }
	float notch() const { return railClip(svf.lp + svf.hp); }
};

// --- VCFS ---------------------------------------------------------------------
struct Vcfs {
	Svf svf;
	float k = 1.f;
	static float qFromSlope(float s) {
		s = clampf(s, 0.f, 1.f);
		if (s <= 0.5f) return 0.05f * std::pow(0.707f / 0.05f, 2.f * s);
		return 0.707f * std::pow(8.f / 0.707f, 2.f * s - 1.f);
	}
	void setup(float fc, float fsOs, float slope) {
		float Q = qFromSlope(slope);
		svf.set(fc, fsOs, Q);
		k = 1.f / Q;
	}
	void step(float in) { svf.process(in, true); }
	float low() const { return railClip(svf.lp); }
	float band() const { return railClip(svf.bp * k); }
	float high() const { return railClip(svf.hp); }
};

// --- Resonant Equalizer -----------------------------------------------------------
struct ResEq {
	static constexpr int N = 10;
	static constexpr float FREQ[N] = {29.f, 61.f, 115.f, 218.f, 411.f, 777.f, 1500.f, 2800.f, 5200.f, 11000.f};
	Svf band[N];
	float A[N] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
	float kk[N] = {};
	float out = 0.f, combUp = 0.f, combDown = 0.f;

	// p in [-1, 1]; returns gain in dB and Q.
	static void knobLaw(float p, float& gainDb, float& Q) {
		p = clampf(p, -1.f, 1.f);
		if (p > 0.5f) {
			gainDb = 12.f + 24.f * (p - 0.5f);
			Q = 1.5f * std::pow(40.f / 1.5f, (p - 0.5f) * 2.f);
		}
		else if (p < -0.5f) {
			gainDb = -12.f - 36.f * (-p - 0.5f);
			Q = 1.5f * std::pow(4.f / 1.5f, (-p - 0.5f) * 2.f);
		}
		else {
			gainDb = 24.f * p;
			Q = 1.5f;
		}
	}
	void setup(const float* knobs, float fsOs) {
		for (int i = 0; i < N; i++) {
			float db, Q;
			knobLaw(knobs[i], db, Q);
			band[i].set(FREQ[i], fsOs, Q);
			kk[i] = 1.f / Q;
			A[i] = std::pow(10.f, db / 20.f);
		}
	}
	// x is the already-scaled input (INPUT gain * 4 * IN).
	void step(float x) {
		float sum = 0.f, up = 0.f, down = 0.f;
		for (int i = 0; i < N; i++) {
			band[i].process(x, false);
			float b = band[i].bp * kk[i];   // unity peak gain
			sum += (A[i] - 1.f) * b;
			if (i & 1) up += A[i] * b;      // 61, 218, 777, 2.8k, 11k
			else down += A[i] * b;          // 29, 115, 411, 1.5k, 5.2k
		}
		out = railClip((x + sum) * 0.25f);
		combUp = railClip(up * 0.25f);
		combDown = railClip(down * 0.25f);
	}
};

} // namespace serge
