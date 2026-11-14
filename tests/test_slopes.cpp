#include "check.hpp"
#include <algorithm>
#include "../src/dsp/Dsg.hpp"
#include "../src/dsp/Ssg.hpp"
using namespace serge;

// Run a DSG section self-patched END -> TRIG (one sample cable delay) and measure frequency.
static double dsgCycleFreq(float fs, float riseK, float fallK, float voct, double seconds = 2.0) {
	DsgSection d;
	float dt = 1.f / fs;
	float endPrev = 0.f;
	int n = (int)(seconds * fs);
	int edges = 0; double firstT = -1, lastT = 0;
	float prevEnd = 0.f;
	for (int i = 0; i < n; i++) {
		d.process(dt, 0.f, endPrev, 0.f, voct, riseK, fallK, 0.f, DsgSection::VC_BOTH);
		endPrev = d.end;
		if (i > fs * 0.2 && prevEnd < 2.5f && d.end >= 2.5f) {
			if (firstT < 0) firstT = i / (double)fs; else { lastT = i / (double)fs; edges++; }
		}
		prevEnd = d.end;
	}
	return edges / (lastT - firstT);
}

TEST("DSG trigger gives one 0..5 V envelope; retrigger ignored") {
	DsgSection d; float fs = 48000, dt = 1 / fs;
	float peak = 0; int cycles = 0; bool wasFalling = false;
	for (int i = 0; i < 48000; i++) {
		float trig = (i == 10 || i == 200) ? 5.f : 0.f;   // second trigger mid-cycle
		d.process(dt, 0, trig, 0, 0, 0.5f, 0.5f, 0, 1);
		peak = std::max(peak, d.out);
		if (wasFalling && !d.falling) cycles++;
		wasFalling = d.falling;
	}
	CHECK_NEAR(peak, 5.0, 0.05);
	CHECK(cycles == 1);
	CHECK(d.out < 0.01f);
}

TEST("DSG gate into IN: rise, hold, fall, and re-rise mid-fall") {
	DsgSection d; float fs = 48000, dt = 1 / fs;
	for (int i = 0; i < 48000; i++) d.process(dt, 3.f, 0, 0, 0, 0.6f, 0.4f, 0, 1);
	CHECK_NEAR(d.out, 3.0, 0.01);
	float mid = 0;
	for (int i = 0; i < 2000; i++) d.process(dt, 0.f, 0, 0, 0, 0.6f, 0.4f, 0, 1);
	mid = d.out;
	CHECK(mid > 0.1f && mid < 2.9f);
	d.process(dt, 3.f, 0, 0, 0, 0.6f, 0.4f, 0, 1);
	CHECK(d.out > mid);
}

TEST("DSG cycling at full knobs is about 5 kHz") {
	double f = dsgCycleFreq(48000, 1, 1, 0);
	std::printf("    cycle freq %g Hz\n", f);
	CHECK(f > 3800 && f < 5200);
}

TEST("DSG 1V/OCT tracking within 3% below 1 kHz") {
	// pick knobs giving about 62 Hz, then go up 4 octaves
	double base = dsgCycleFreq(48000, 0.55f, 0.55f, 0, 4.0);
	std::printf("    base %g Hz\n", base);
	for (int o = 1; o <= 4; o++) {
		double f = dsgCycleFreq(48000, 0.55f, 0.55f, (float)o, 4.0);
		double ratio = f / base / std::pow(2.0, o);
		std::printf("    +%d oct: %g Hz ratio %g\n", o, f, ratio);
		if (f < 1000) CHECK_NEAR(ratio, 1.0, 0.03);
	}
}

TEST("DSG sub-harmonic division") {
	DsgSection d; float fs = 48000, dt = 1 / fs;
	// clock at 100 Hz; rise+fall slightly longer than one period (10 ms) -> every second clock
	float k = std::log2(5.f / 0.0055f / 0.25f) / 17.6f;   // 5.5 ms per slope
	int cycles = 0; bool wasF = false;
	for (int i = 0; i < 48000; i++) {
		float clk = (i % 480) < 24 ? 5.f : 0.f;
		d.process(dt, 0, clk, 0, 0, k, k, 0, 1);
		if (wasF && !d.falling) cycles++;
		wasF = d.falling;
	}
	std::printf("    cycles in 1 s: %d\n", cycles);
	CHECK(cycles >= 48 && cycles <= 51);
}

TEST("DSG portamento passes signal at full knobs, glides when slow") {
	DsgSection d; float fs = 48000, dt = 1 / fs;
	for (int i = 0; i < 100; i++) d.process(dt, 2.f, 0, 0, 0, 1, 1, 0, 1);
	CHECK_NEAR(d.out, 2.0, 0.001);
	d.process(dt, 0.f, 0, 0, 0, 0.3f, 0.3f, 0, 1);
	CHECK(d.out > 1.9f);
}

TEST("DSG survives garbage") {
	DsgSection d; Rng r; r.seed(1);
	for (int i = 0; i < 200000; i++) {
		d.process(1 / 48000.f, r.gauss() * 10, r.gauss() * 10, r.gauss() * 10, r.gauss() * 10, r.uniform(), r.uniform(), r.uniform() * 2 - 1, i % 3);
		CHECK(std::isfinite(d.out) && std::fabs(d.out) <= RAIL);
		if (failures()) break;
	}
}

TEST("SSG smooth self-cycles: ~200 Hz at full RATE, +-2.5 V triangle") {
	Ssg s; float fs = 48000, dt = 1 / fs;
	float cyc = s.smooth.cycle; int edges = 0; float mx = -9, mn = 9; float prev = cyc;
	for (int i = 0; i < 48000; i++) {
		s.process(dt, cyc, 0, 0, 1.f, 0, 0, 0, 0, 0.5f, 0);
		cyc = s.smooth.cycle;
		if (i > 4800) { mx = std::max(mx, s.smooth.slew.y); mn = std::min(mn, s.smooth.slew.y); if (prev < 0 && cyc > 0) edges++; }
		prev = cyc;
	}
	std::printf("    %d cycles in 0.9 s, %g..%g V\n", edges, mn, mx);
	CHECK(edges > 160 && edges < 200);
	CHECK_NEAR(mx, 2.5, 0.2);
	CHECK_NEAR(mn, -2.5, 0.2);
}

TEST("SSG hold freezes smooth output") {
	Ssg s; float dt = 1 / 48000.f;
	for (int i = 0; i < 4800; i++) s.process(dt, 3.f, 0, 0, 0.5f, 0, 0, 0, 0, 0.5f, 0);
	float a = s.smooth.slew.y;
	for (int i = 0; i < 4800; i++) s.process(dt, -3.f, 0, 5.f, 0.5f, 0, 0, 0, 0, 0.5f, 0);
	CHECK_NEAR(s.smooth.slew.y, a, 1e-6);
}

TEST("SSG stepped staircase: ~2 V steps at full RATE") {
	Ssg s; float dt = 1 / 48000.f;
	float cyc = s.stepped.cycle;
	std::vector<float> vals;
	for (int i = 0; i < 48000; i++) {
		float clk = (i % 480) < 24 ? 5.f : 0.f;
		s.process(dt, 0, 0, 0, 0.5f, 0, cyc, 0, clk, 1.f, 0);
		cyc = s.stepped.cycle;
		if (i % 480 == 300) vals.push_back(s.stepped.slew.y);
	}
	std::vector<float> d;
	for (size_t i = 5; i + 1 < vals.size(); i++) d.push_back(std::fabs(vals[i + 1] - vals[i]));
	std::sort(d.begin(), d.end());
	float step = d[d.size() / 2];
	std::printf("    step %g V\n", step);
	CHECK(step > 1.8f && step < 2.3f);
}

TEST("SSG coupler follows comparison") {
	Ssg s; float dt = 1 / 48000.f;
	for (int i = 0; i < 4800; i++) s.process(dt, -1, 0, 0, 1, 0, 0, 0, 0, 1, 0);
	CHECK(s.coupler == 5.f && s.hot == RAIL);
	for (int i = 0; i < 4800; i++) s.process(dt, 2, 0, 0, 1, 0, 0, 0, 0, 1, 0);
	CHECK(s.coupler == 0.f && s.hot == -RAIL);
}
