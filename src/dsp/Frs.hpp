// FRS, Frequency Shifter: quadrature sine oscillator, 6 + 6 pole phase-difference
// network, two multipliers, output VCAs and a compressed copy of the input.
// Plain C++11, no Rack includes.
#pragma once
#include "Common.hpp"
#include "Oversample.hpp"

namespace serge {

// First-order all-pass with an analog pole at p Hz (bilinear, no prewarp).
struct Allpass1 {
	double g = 0.0, x1 = 0.0, y1 = 0.0;
	void set(double p, double fs) {
		double k = 3.14159265358979323846 * p / fs;
		g = (k - 1.0) / (k + 1.0);
	}
	double process(double x) {
		double y = g * x + x1 - g * y1;
		x1 = x; y1 = y;
		return y;
	}
	void reset() { x1 = y1 = 0.0; }
};

// Two chains whose outputs are 90 degrees apart from 20 Hz to 20 kHz (I leads Q).
struct PhaseNetwork {
	Allpass1 ci[6], cq[6];
	void set(double fs) {
		static const double pi[6] = {25.05, 112.62, 448.11, 1773.27, 7228.10, 55766.88};
		static const double pq[6] = {7.17, 55.27, 225.18, 890.98, 3546.26, 15953.69};
		for (int j = 0; j < 6; j++) { ci[j].set(pi[j], fs); cq[j].set(pq[j], fs); }
	}
	void process(double x, double& i, double& q) {
		i = x; q = x;
		for (int j = 0; j < 6; j++) { i = ci[j].process(i); q = cq[j].process(q); }
	}
	void reset() { for (int j = 0; j < 6; j++) { ci[j].reset(); cq[j].reset(); } }
};

struct Frs {
	OsKernel kernel;
	int factor = 1;
	float fs = 48000.f, fsOs = 48000.f;
	Upsampler up;
	Decimator decUp, decDown;
	PhaseNetwork net;
	double phase = 0.0;
	float env = 0.f;
	float gUp = 1.f, gDown = 1.f;              // smoothed knob values
	bool smoothInit = false;
	float kSmooth = 0.f, kAtt = 0.f, kRel = 0.f; // one-pole coefficients

	// outputs
	float outUp = 0.f, outDown = 0.f, comp = 0.f, osc0 = 0.f, osc90 = 0.f;

	Frs() { setRate(48000.f); }

	void setRate(float sampleRate) {
		fs = sampleRate;
		factor = oversampleFactor(sampleRate);
		fsOs = fs * factor;
		kernel.design(factor);
		net.set(fsOs);
		float dt = 1.f / fs;
		kSmooth = 1.f - std::exp(-dt / 0.005f);
		kAtt = 1.f - std::exp(-dt / 0.001f);
		kRel = 1.f - std::exp(-dt / 0.1f);
		reset();
	}

	static float shiftHz(float freqKnob, float vcFreq, float vFreqCv, float vVoct) {
		return clampf(0.05f * std::exp2(16.61f * freqKnob + vcFreq * vFreqCv + vVoct), 0.01f, 8000.f);
	}

	// upKnob, downKnob 0..1; vcUp/vcDown < 0 means unpatched.
	void process(float vIn, float freqKnob, float vcFreq, float vFreqCv, float vVoct,
	             float upKnob, float downKnob, bool vcUpPatched, float vVcUp, bool vcDownPatched, float vVcDown,
	             bool needUp = true, bool needDown = true) {
		float f = shiftHz(freqKnob, vcFreq, vFreqCv, vVoct);
		double inc = (double) f / fsOs;

		float xs[OS_MAX], us[OS_MAX], ds[OS_MAX];
		up.process(kernel, vIn, xs);
		// Quadrature oscillator: exact at the first sub-sample, then rotated.
		const double TWO_PI = 2.0 * 3.14159265358979323846;
		phase += inc;
		if (phase >= 1.0) phase -= 1.0;
		double s = std::sin(TWO_PI * phase), c = std::cos(TWO_PI * phase);
		double rs = 0.0, rc = 1.0;
		if (factor > 1) { rs = std::sin(TWO_PI * inc); rc = std::cos(TWO_PI * inc); }
		for (int p = 0; p < factor; p++) {
			if (p > 0) {
				double s2 = s * rc + c * rs;
				c = c * rc - s * rs;
				s = s2;
				phase += inc;
				if (phase >= 1.0) phase -= 1.0;
			}
			double I, Q;
			net.process(xs[p], I, Q);
			us[p] = (float) (I * c - Q * s);
			ds[p] = (float) (I * c + Q * s);
		}
		float u = needUp ? decUp.process(kernel, us) : 0.f;
		float d = needDown ? decDown.process(kernel, ds) : 0.f;

		if (!smoothInit) { gUp = upKnob; gDown = downKnob; smoothInit = true; }
		gUp += (upKnob - gUp) * kSmooth;
		gDown += (downKnob - gDown) * kSmooth;
		float gU = clampf(gUp * (vcUpPatched ? vVcUp / 5.f : 1.f), 0.f, 1.f);
		float gD = clampf(gDown * (vcDownPatched ? vVcDown / 5.f : 1.f), 0.f, 1.f);
		outUp = railClip(gU * u);
		outDown = railClip(gD * d);

		osc0 = 2.5f * (float) s;
		osc90 = 2.5f * (float) c;

		// COMP: 2:1 feed-forward compressor toward +/-2.5 V
		float a = std::fabs(vIn);
		env += (a - env) * (a > env ? kAtt : kRel);
		float gain = std::sqrt(2.5f / std::max(env, 0.025f));
		comp = railClip(vIn * gain);

		if (!finite(outUp) || !finite(outDown) || !finite(comp) || !finite(env)) reset();
	}

	void reset() {
		up.reset(); decUp.reset(); decDown.reset(); net.reset();
		env = 0.f; outUp = outDown = comp = 0.f;
		smoothInit = false;
	}
};

} // namespace serge
