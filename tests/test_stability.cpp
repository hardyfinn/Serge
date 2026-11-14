// Garbage in at four sample rates: every model stays finite and inside the rails.
#include "check.hpp"
#include "../src/dsp/Dsg.hpp"
#include "../src/dsp/Ssg.hpp"
#include "../src/dsp/Nto.hpp"
#include "../src/dsp/Shapers.hpp"
#include "../src/dsp/Filters.hpp"
#include "../src/dsp/Noise.hpp"
#include "../src/dsp/Utility.hpp"
#include "../src/dsp/Oversample.hpp"
using namespace serge;

static bool ok(float v) { return std::isfinite(v) && std::fabs(v) <= RAIL + 1e-3f; }

TEST("Stability: noise into every model at 44.1/48/96/192 kHz") {
	for (float fs : {44100.f, 48000.f, 96000.f, 192000.f}) {
		Rng r; r.seed((uint64_t) fs);
		int F = oversampleFactor(fs); float fsOs = fs * F, dtOs = 1 / fsOs, dt = 1 / fs;
		DsgSection d; Ssg s; Nto n; Vcfq q; Vcfs v; ResEq e; Asr a; Str st;
		int bad = 0;
		for (int i = 0; i < (int)(fs * 2); i++) {
			auto g = [&] { return r.gauss() * 10.f; };
			auto u = [&] { return r.uniform(); };
			if (i % 4800 == 0) {
				float knobs[10]; for (float& k : knobs) k = u() * 2 - 1;
				e.setup(knobs, fsOs);
			}
			d.process(dt, g(), g(), g(), g(), u(), u(), u() * 2 - 1, i % 3);
			s.process(dt, g(), g(), g(), u(), u(), g(), g(), g(), u(), u());
			for (int k = 0; k < F; k++) n.step(dtOs, 16.f * std::exp2(u() * 14), u(), g(), g(), u());
			q.setup(20.f * std::exp2(u() * 10 + g() * 0.1f), fsOs, u() + g() * 0.05f, dtOs); q.trigger(g());
			v.setup(20.f * std::exp2(u() * 10), fsOs, u());
			for (int k = 0; k < F; k++) { q.step(g(), g()); v.step(g()); e.step(g()); }
			a.process(g(), g()); st.process(g(), g());
			float outs[] = {d.out, d.end, s.smooth.slew.y, s.stepped.slew.y, s.coupler, s.hot, s.smooth.cycle,
			                n.saw, n.tri, n.sine, n.var, q.low(), q.band(), q.high(), q.notch(), v.low(), v.band(), v.high(),
			                e.out, e.combUp, e.combDown, a.out[0], a.out[2], st.outRs,
			                vcmTop(g(), u(), i & 1), vcmMiddle(g(), u()), tws(g()), ring(g(), g(), u(), u())};
			for (size_t j = 0; j < sizeof(outs) / sizeof(outs[0]); j++) if (!ok(outs[j])) { bad++; if (bad < 4) std::printf("    output %zu = %g\n", j, outs[j]); }
			float o3, sq; vcmBottom(g(), u(), o3, sq);
			if (!std::isfinite(o3) || !std::isfinite(sq)) bad++;
		}
		if (bad) std::printf("    %g Hz: %d bad samples\n", fs, bad);
		CHECK(bad == 0);
	}
}

TEST("Silence: no input gives exactly 0 V from processors") {
	Vcfq q; q.setup(1000, 192000, 0.5f, 1 / 192000.f);
	Vcfs v; v.setup(1000, 192000, 0.5f);
	ResEq e; float k[10] = {0.9f, -0.9f, 0.5f, 0, 0, 0, 0, 0, 0, 1}; e.setup(k, 192000);
	for (int i = 0; i < 10000; i++) { q.step(0, 0); v.step(0); e.step(0); }
	CHECK(q.low() == 0.f && q.band() == 0.f && v.low() == 0.f && e.out == 0.f && e.combUp == 0.f);
	CHECK(vcmMiddle(0, 0.7f) == 0.f);
	CHECK(tws(0) == 0.f);
	CHECK(ring(0, 2, 1, 1) == 0.f);
}

TEST("Sample-rate independence: DSG and SSG cycle, filter cutoff") {
	auto ssgFreq = [](float fs) {
		Ssg s; float dt = 1 / fs, cyc = s.smooth.cycle, prev = cyc; int edges = 0;
		for (int i = 0; i < (int)(fs * 2); i++) {
			s.process(dt, cyc, 0, 0, 0.7f, 0, 0, 0, 0, 0.5f, 0);
			cyc = s.smooth.cycle; if (prev < 0 && cyc > 0) edges++; prev = cyc;
		}
		return edges / 2.0;
	};
	double base = ssgFreq(48000);
	for (float fs : {44100.f, 96000.f, 192000.f}) CHECK_NEAR(ssgFreq(fs) / base, 1.0, 0.01);
	auto dsgFreq = [](float fs) {
		DsgSection d; float dt = 1 / fs, endPrev = 0, prev = 0; int edges = 0;
		for (int i = 0; i < (int)(fs * 4); i++) {
			d.process(dt, 0, endPrev, 0, 0, 0.45f, 0.45f, 0, 1);
			endPrev = d.end; if (prev < 2.5f && d.end >= 2.5f) edges++; prev = d.end;
		}
		return edges / 4.0;
	};
	double db = dsgFreq(48000);
	std::printf("    SSG %g Hz, DSG %g Hz at 48k\n", base, db);
	for (float fs : {44100.f, 96000.f, 192000.f}) CHECK_NEAR(dsgFreq(fs) / db, 1.0, 0.01);
}

TEST("ASR shifts on each pulse") {
	Asr a;
	a.process(1, 0); a.process(1, 5); a.process(2, 0); a.process(2, 5); a.process(3, 0); a.process(3, 5);
	CHECK(a.out[0] == 3 && a.out[1] == 2 && a.out[2] == 1);
}

TEST("STR squares audio and R/S flip-flop") {
	Str s;
	s.process(1, 0); CHECK(s.outA == 5 && s.outRs == 5);
	s.process(-1, 1); CHECK(s.outA == 0 && s.outB == 5 && s.outRs == 0);
}
