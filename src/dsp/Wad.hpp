// WAD, Wilson Analog Delay: a 2048-stage bucket brigade (two SAD1024s) with taps
// after 1024 stages (A) and 2048 stages (B), a voltage-controlled clock, user-set
// input and output filters, switched feedback, a flange output and a 1 V/oct output.
// Plain C++11, no Rack includes.
#pragma once
#include "Common.hpp"
#include "Blep.hpp"
#include "Svf.hpp"

namespace serge {

struct Wad {
	// The line holds one sample per clock tick: 1024 samples for 2048 stages.
	float buf[1024] = {};
	int w = 0;
	double phase = 0.0;
	float heldA = 0.f, heldB = 0.f;
	Blep blepA, blepB;
	Svf inF, outFA, outFB;
	float xPrev = 0.f;
	float lastA = 0.f, lastB = 0.f;
	Smoother sDelay, sFilter;
	Rng rng;
	bool noise = true;     // the line's own hiss (about 1 mV); tests can turn it off

	// outputs
	float a = 0.f, b = 0.f, flange = 0.f, voct = 0.f;
	float fClk = 0.f;

	// The line's soft limit, 6*tanh(v/6), as a rational fit (within 2% of tanh, exact at small levels).
	static float bbdSat(float v) {
		float x = v * (1.f / 6.f);
		if (x > 3.f) return 6.f;
		if (x < -3.f) return -6.f;
		float x2 = x * x;
		return 6.f * x * (27.f + x2) / (27.f + 9.f * x2);
	}

	// source: 0 = A, 1 = B, 2 = AUX
	void process(float fs, float delayKnob, float vcDelay, float vDelayCv,
	             float filterKnob, float vcFilter, float vFilterCv,
	             float in1Knob, float vIn1, float in2Knob, float vIn2,
	             float in3Knob, int source, float vAux, float flangeKnob) {
		float dt = 1.f / fs;
		// 1. clock
		float dk = sDelay.process(delayKnob, dt);
		float V = clampf(12.32f * (1.f - dk) + vcDelay * vDelayCv, 0.f, 12.32f);
		fClk = 200.f * std::exp2(V);
		voct = V - 6.f;

		// 2. filters share one cutoff
		float fk = sFilter.process(filterKnob, dt);
		float fc = clampf(20.f * std::exp2(10.f * fk + vcFilter * vFilterCv), 5.f, std::min(20000.f, 0.45f * fs));
		inF.set(fc, fs, 0.7071f);
		outFA.g = inF.g; outFA.k = inF.k; outFA.a1 = inF.a1; outFA.a2 = inF.a2; outFA.a3 = inF.a3;
		outFB.g = inF.g; outFB.k = inF.k; outFB.a1 = inF.a1; outFB.a2 = inF.a2; outFB.a3 = inF.a3;

		// 3. input mix
		float dry = in1Knob * vIn1 + in2Knob * vIn2;
		float fb = source == 0 ? lastA : source == 1 ? lastB : vAux;
		inF.process(dry + in3Knob * fb, false);
		float x = inF.lp;

		// 4. bucket brigade: one write per clock tick
		double step = (double) fClk / fs;
		phase += step;
		while (phase >= 1.0) {
			phase -= 1.0;
			float frac = (float) (phase / step);       // how long before the end of this sample the tick fell
			float xin = x - frac * (x - xPrev);
			float newB = buf[w];
			float newA = buf[(w + 512) & 1023];
			float v = bbdSat(xin);
			if (noise) v += 0.003464f * (rng.uniform() - 0.5f);   // 1 mV RMS
			buf[w] = v;
			w = (w + 1) & 1023;
			blepA.add(newA - heldA, frac);  heldA = newA;
			blepB.add(newB - heldB, frac);  heldB = newB;
		}
		xPrev = x;

		// 5. outputs
		outFA.process(blepA.process(heldA), false);
		outFB.process(blepB.process(heldB), false);
		a = railClip(outFA.lp);
		b = railClip(outFB.lp * 0.891f);
		flange = railClip(dry + flangeKnob * a);
		lastA = a; lastB = b;

		if (!finite(a) || !finite(b) || !finite(flange) || !finite(x)) reset();
	}

	void reset() {
		for (int i = 0; i < 1024; i++) buf[i] = 0.f;
		w = 0; phase = 0.0; heldA = heldB = 0.f;
		blepA = Blep(); blepB = Blep();
		inF.reset(); outFA.reset(); outFB.reset();
		xPrev = lastA = lastB = 0.f;
		a = b = flange = 0.f;
	}
};

} // namespace serge
