#include "check.hpp"
#include "../src/dsp/Nto.hpp"
#include "../src/dsp/Shapers.hpp"
#include "../src/dsp/Oversample.hpp"
#include "../src/dsp/Filters.hpp"
#include "../src/dsp/Noise.hpp"
#include "../src/dsp/Utility.hpp"
#include <vector>
#include <complex>
using namespace serge;

// Goertzel-style amplitude of frequency f in signal x at rate fs (Hann window).
static double amp(const std::vector<float>& x, double f, double fs) {
	std::complex<double> acc = 0; double wsum = 0;
	for (size_t i = 0; i < x.size(); i++) {
		double w = 0.5 - 0.5 * std::cos(2 * M_PI * i / (x.size() - 1));
		acc += w * (double)x[i] * std::exp(std::complex<double>(0, -2 * M_PI * f * i / fs));
		wsum += w;
	}
	return 2 * std::abs(acc) / wsum;
}

// Run an NTO at base rate fs with oversampling, returning decimated SAW.
static std::vector<float> runNto(float fs, float fexp, int n, int which = 0, float w = 0) {
	OsKernel k; int F = oversampleFactor(fs); k.design(F);
	Decimator dec; Nto o; std::vector<float> out;
	float dtOs = 1.f / (fs * F); float buf[OS_MAX];
	for (int i = 0; i < n; i++) {
		for (int p = 0; p < F; p++) {
			o.step(dtOs, fexp, 0, 0, 0, w);
			buf[p] = which == 0 ? o.saw : which == 1 ? o.tri : which == 2 ? o.sine : o.var;
		}
		out.push_back(dec.process(k, buf));
	}
	return out;
}

static double zeroCrossFreq(const std::vector<float>& x, double fs, float level) {
	int first = -1, last = -1, count = 0;
	for (size_t i = 1; i < x.size(); i++) if (x[i - 1] < level && x[i] >= level) {
		if (first < 0) first = (int)i; else { last = (int)i; count++; }
	}
	return count / ((last - first) / fs);
}

TEST("NTO pitch exact (cycle count) from 16 Hz to 8 kHz") {
	for (float f : {16.f, 110.f, 1000.f, 8000.f}) {
		auto x = runNto(48000, f, 48000 * 4, 1);
		double meas = zeroCrossFreq(x, 48000, 2.5f);
		double cents = 1200 * std::log2(meas / f);
		std::printf("    %g Hz -> %g Hz (%g cents)\n", f, meas, cents);
		CHECK(std::fabs(cents) < 1.0);
	}
}

TEST("NTO saw aliases below -70 dB in the audio band at 2.1 kHz / 48 kHz") {
	auto x = runNto(48000, 2100.f, 48000);
	double fund = amp(x, 2100, 48000);
	// strongest alias: look at a few non-harmonic bins
	double worst = 0;
	for (int h = 12; h <= 40; h++) {
		double fa = std::fabs(std::fmod(2100.0 * h + 24000, 48000) - 24000);
		bool harmonic = std::fabs(std::fmod(fa, 2100.0)) < 1 || std::fabs(std::fmod(fa, 2100.0) - 2100) < 1;
		if (!harmonic && fa > 50 && fa < 20000) worst = std::max(worst, amp(x, fa, 48000));
	}
	double db = 20 * std::log10(worst / fund);
	std::printf("    worst alias %g dB\n", db);
	CHECK(db < -70);
}

TEST("NTO sine is clean, VAR at w=0 equals sine") {
	auto s = runNto(48000, 500.f, 48000, 2);
	double f1 = amp(s, 500, 48000), f3 = amp(s, 1500, 48000);
	std::printf("    sine %g V, 3rd %g dB\n", f1, 20 * std::log10(f3 / f1));
	CHECK(f1 > 2.3 && f1 < 2.6);
	auto v = runNto(48000, 500.f, 48000, 3, 0);
	double d = 0; for (size_t i = 0; i < v.size(); i++) d = std::max(d, (double)std::fabs(v[i] - s[i]));
	CHECK(d < 1e-4);
}

TEST("NTO sync locks slave to master at 1x, 2x, 0.5x") {
	float fs = 48000; float dtOs = 1 / (fs * 4);
	for (float ratio : {1.f, 2.f, 0.5f}) {
		Nto master, slave; double t = 0; int masterEdges = 0, slaveWraps = 0;
		float prevPh = 0;
		float mf = 220, sf = mf * ratio * 0.97f;   // slave slightly flat: soft sync pulls it up
		float prevSaw = 0;
		for (int i = 0; i < 48000 * 4; i++) {
			master.step(dtOs, mf, 0, 0, 0, 0);
			float pulse = master.phase < 0.5f ? 5.f : 0.f;
			if (prevSaw >= 0.5f && master.phase < 0.5f) masterEdges++;
			prevSaw = master.phase;
			slave.step(dtOs, sf, 0, 0, pulse, 0);
			if (slave.phase < prevPh) slaveWraps++;
			prevPh = slave.phase;
			t += dtOs;
		}
		double measured = (double)slaveWraps / masterEdges;
		std::printf("    ratio %g -> %g\n", ratio, measured);
		CHECK_NEAR(measured, ratio, 0.02);
	}
}

TEST("VCM middle: clean at c=0, adds odd harmonics as c rises") {
	std::vector<float> a, b;
	for (int i = 0; i < 48000; i++) {
		float s = 2.5f * std::sin(2 * M_PI * 100 * i / 48000.0);
		a.push_back(vcmMiddle(s, 0)); b.push_back(vcmMiddle(s, 0.6f));
	}
	double a1 = amp(a, 100, 48000), a3 = amp(a, 300, 48000);
	double b1 = amp(b, 100, 48000), b3 = amp(b, 300, 48000), b2 = amp(b, 200, 48000);
	std::printf("    c=0: 3rd %g dB; c=0.6: 3rd %g dB, 2nd %g dB\n", 20 * log10(a3 / a1), 20 * log10(b3 / b1), 20 * log10(b2 / b1 + 1e-12));
	CHECK(a3 / a1 < 0.01);
	CHECK(b3 / b1 > 0.1);
	CHECK(b2 / b1 < 0.001);
}

TEST("VCM bottom doubles a triangle per stage") {
	for (int stage = 1; stage <= 3; stage++) {
		std::vector<float> y;
		for (int i = 0; i < 48000; i++) {
			double ph = std::fmod(100.0 * i / 48000, 1.0);
			float tri = 2.5f * (float)(1 - 4 * std::fabs(ph - 0.5));
			float o, sq; vcmBottom(tri, stage / 3.f, o, sq); y.push_back(o);
		}
		double f = 100 * (1 << stage);
		double main = amp(y, f, 48000), base = amp(y, 100, 48000);
		std::printf("    stage %d: %g Hz amp %g, 100 Hz amp %g\n", stage, f, main, base);
		CHECK(main > 1.5 && base < 0.01);
	}
}

TEST("TWS turns a saw into a sine") {
	std::vector<float> y;
	for (int i = 0; i < 48000; i++) {
		double ph = std::fmod(100.0 * i / 48000, 1.0);
		y.push_back(tws((float)(5 * ph - 2.5)));
	}
	double h1 = amp(y, 100, 48000), thd = 0;
	for (int h = 2; h <= 9; h++) thd += std::pow(amp(y, 100 * h, 48000), 2);
	std::printf("    THD %g %%\n", 100 * std::sqrt(thd) / h1);
	CHECK(std::sqrt(thd) / h1 < 0.05);
}

TEST("RING: m=0 dry, m=1 suppresses carrier and signal") {
	CHECK_NEAR(ring(1.3f, 2.f, 0, 0.5f), 1.3, 1e-6);
	std::vector<float> y;
	for (int i = 0; i < 48000; i++) {
		float s = 2.f * std::sin(2 * M_PI * 300 * i / 48000.0), c = 1.f * std::sin(2 * M_PI * 1000 * i / 48000.0);
		y.push_back(ring(s, c, 1, 0.5f));
	}
	CHECK(amp(y, 300, 48000) < 0.01);
	CHECK(amp(y, 1300, 48000) > 0.3);
}

TEST("VCFQ: unity LP passband, BP rings with Q-dependent decay") {
	float fs = 48000 * 4, dt = 1 / fs;
	Vcfq f; f.setup(440, fs, std::log(100.f) / std::log(200.f), dt);
	// passband
	for (int i = 0; i < 4 * (int)fs; i++) f.step(0.5f, 0);
	CHECK_NEAR(f.low(), 0.5, 0.01);
	f.svf.reset();
	f.trigger(0); f.trigger(5);
	std::vector<float> bp;
	for (int i = 0; i < (int)fs; i++) { f.step(0, 0); bp.push_back(f.band()); }
	// envelope: peak per period
	int per = (int)(fs / 440);
	std::vector<float> env;
	for (size_t i = 0; i + per < bp.size(); i += per) { float m = 0; for (int j = 0; j < per; j++) m = std::max(m, std::fabs(bp[i + j])); env.push_back(m); }
	// time from 1.0 V to 1/e V
	int i1 = -1, i2 = -1;
	for (size_t i = 0; i < env.size(); i++) { if (i1 < 0 && env[i] < 1.0f) i1 = i; if (i2 < 0 && env[i] < 1.0f / M_E) i2 = i; }
	double tau = (i2 - i1) * per / fs, expect = 100 / (M_PI * 440);
	std::printf("    peak %g V, tau %g s (expect %g)\n", env[0], tau, expect);
	CHECK(env[0] > 1.0);
	CHECK_NEAR(tau, expect, expect * 0.15);
}

TEST("VCFQ: BAND patched back to IN sustains oscillation") {
	float fs = 192000, dt = 1 / fs;
	Vcfq f; f.setup(1000, fs, 0.0f, dt);
	float fb = 0.01f; float mx = 0;
	for (int i = 0; i < (int)fs; i++) { f.step(2.f * fb, 0); fb = f.band(); if (i > fs / 2) mx = std::max(mx, std::fabs(fb)); }
	std::printf("    oscillation amplitude %g V\n", mx);
	CHECK(mx > 1.0 && mx < RAIL);
}

static double svfResponseDb(Vcfs& f, float fs, float freq, int out) {
	f.svf.reset(); std::vector<float> y;
	int n = (int)fs; 
	for (int i = 0; i < n; i++) { f.step(0.1f * (float)std::sin(2 * M_PI * freq * i / fs)); if (i > n / 2) y.push_back(out == 0 ? f.low() : f.band()); }
	return 20 * std::log10(amp(y, freq, fs) / 0.1);
}

TEST("VCFS slope: Butterworth at centre, 6 dB/oct at min, peak at max") {
	float fs = 192000;
	Vcfs f; f.setup(1000, fs, 0.5f);
	double c = svfResponseDb(f, fs, 1000, 0), c4 = svfResponseDb(f, fs, 4000, 0);
	std::printf("    centre: %g dB at fc, %g dB at 4 fc\n", c, c4);
	CHECK_NEAR(c, -3.01, 0.5);
	CHECK_NEAR(c4, -24.1, 1.0);
	f.setup(1000, fs, 0.0f);
	// find -3 dB point then measure the next two octaves
	double fc3 = 0; for (double fr = 5; fr < 1000; fr *= 1.02) if (svfResponseDb(f, fs, fr, 0) < -3) { fc3 = fr; break; }
	double a = svfResponseDb(f, fs, fc3 * 2, 0), b = svfResponseDb(f, fs, fc3 * 4, 0);
	std::printf("    min: -3 dB at %g Hz, slope over next octave %g dB\n", fc3, b - a);
	CHECK_NEAR(b - a, -6, 1.5);
	f.setup(1000, fs, 1.0f);
	double pk = svfResponseDb(f, fs, 1000, 0);
	std::printf("    max: %g dB at fc\n", pk);
	CHECK(pk > 15 && pk < 21);
}

TEST("RESEQ: flat when centred, +12 dB per band at 3 o'clock, combs") {
	float fs = 48000 * 4;
	ResEq eq; float knobs[10] = {}; eq.setup(knobs, fs);
	for (float fr : {50.f, 400.f, 3000.f}) {
		std::vector<float> y; for (int i = 0; i < (int)fs; i++) { eq.step(0.4f * (float)std::sin(2 * M_PI * fr * i / fs)); if (i > fs / 2) y.push_back(eq.out); }
		double db = 20 * log10(amp(y, fr, fs) / 0.1);
		CHECK_NEAR(db, 0, 0.1);
	}
	knobs[4] = 0.5f; eq.setup(knobs, fs);
	std::vector<float> y; for (int i = 0; i < (int)fs; i++) { eq.step(0.04f * (float)std::sin(2 * M_PI * 411 * i / fs)); if (i > fs / 2) y.push_back(eq.out); }
	double db = 20 * log10(amp(y, 411, fs) / 0.01);
	std::printf("    411 Hz band at +0.5: %g dB\n", db);
	CHECK_NEAR(db, 12, 0.5);
}

TEST("Noise levels") {
	NoiseSource n; n.rng.seed(3); n.setSampleRate(48000);
	double sw = 0, sp = 0, mean = 0; int N = 480000;
	for (int i = 0; i < N; i++) { n.process(); sw += n.white * n.white; sp += n.pink * n.pink; mean += n.sh; }
	std::printf("    white %g, pink %g, sh mean %g\n", std::sqrt(sw / N), std::sqrt(sp / N), mean / N);
	CHECK_NEAR(std::sqrt(sw / N), 0.8, 0.05);
	CHECK_NEAR(std::sqrt(sp / N), 0.8, 0.08);
	CHECK_NEAR(mean / N, 2.5, 0.15);
}

TEST("VCA2 law") {
	CHECK_NEAR(vca2Gain(5), 1, 1e-5);
	CHECK_NEAR(20 * log10(vca2Gain(4)), -12, 1e-3);
	CHECK(vca2Gain(0.3f) < 1e-4);
	CHECK_NEAR(vca2Gain(9), 2, 1e-6);
}

TEST("Oversampler passes a tone at unity") {
	OsKernel k; k.design(4); Upsampler u; Decimator d; std::vector<float> y; float buf[4];
	for (int i = 0; i < 48000; i++) { u.process(k, (float)std::sin(2 * M_PI * 1000 * i / 48000.0), buf); y.push_back(d.process(k, buf)); }
	CHECK_NEAR(amp(y, 1000, 48000), 1, 0.01);
}
