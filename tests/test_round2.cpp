// Round two: BDR, PDIV, FRS, WAD.
#include "check.hpp"
#include "../src/dsp/Switching.hpp"
#include "../src/dsp/Frs.hpp"
#include "../src/dsp/Wad.hpp"
#include <vector>
#include <complex>
using namespace serge;

static double amp2(const std::vector<float>& x, double f, double fs, size_t from = 0) {
	std::complex<double> acc = 0; double wsum = 0; size_t n = x.size() - from;
	for (size_t i = 0; i < n; i++) {
		double w = 0.5 - 0.5 * std::cos(2 * M_PI * i / (n - 1));
		acc += w * (double) x[from + i] * std::exp(std::complex<double>(0, -2 * M_PI * f * i / fs));
		wsum += w;
	}
	return 2 * std::abs(acc) / wsum;
}
static double db(double r) { return 20 * std::log10(std::max(r, 1e-12)); }
static double rms(const std::vector<float>& x, size_t a, size_t b) {
	double s = 0; for (size_t i = a; i < b; i++) s += (double) x[i] * x[i]; return std::sqrt(s / (b - a));
}
static double zcFreq(const std::vector<float>& x, double fs, size_t from) {
	int first = -1, last = -1, count = 0;
	for (size_t i = from + 1; i < x.size(); i++) if (x[i - 1] < 0 && x[i] >= 0) {
		if (first < 0) first = (int) i; else count++;
		last = (int) i;
	}
	if (count < 1) return 0;
	return count * fs / (last - first);
}

// ------------------------------------------------------------------ BDR ----
TEST("BDR: routes by SELECT, both directions") {
	BdrSection s;
	for (int i = 0; i < 10; i++) s.process(0.f, 1.5f, 0.7f, -0.9f);
	CHECK_NEAR(s.a1Out, 1.5f, 1e-6); CHECK_NEAR(s.a2Out, 0.f, 1e-6); CHECK_NEAR(s.bOut, 0.7f, 1e-6);
	for (int i = 0; i < 10; i++) s.process(5.f, 1.5f, 0.7f, -0.9f);
	CHECK_NEAR(s.a1Out, 0.f, 1e-6); CHECK_NEAR(s.a2Out, 1.5f, 1e-6); CHECK_NEAR(s.bOut, -0.9f, 1e-6);
	// hysteresis: 2.2 V keeps the side last selected
	for (int i = 0; i < 10; i++) s.process(2.2f, 1.5f, 0.7f, -0.9f);
	CHECK_NEAR(s.a2Out, 1.5f, 1e-6);
	for (int i = 0; i < 10; i++) s.process(0.f, 1.5f, 0.7f, -0.9f);
	for (int i = 0; i < 10; i++) s.process(2.2f, 1.5f, 0.7f, -0.9f);
	CHECK_NEAR(s.a1Out, 1.5f, 1e-6);
	// clamp
	for (int i = 0; i < 10; i++) s.process(0.f, 10.f, -10.f, 0.f);
	CHECK_NEAR(s.a1Out, 7.f, 1e-6); CHECK_NEAR(s.bOut, -7.f, 1e-6);
	// unpatched SELECT reads 0: A1
	BdrSection u; for (int i = 0; i < 4; i++) u.process(0.f, 2.f, 0.f, 0.f);
	CHECK_NEAR(u.a1Out, 2.f, 1e-6);
}

TEST("BDR: audio-rate chop is band-limited") {
	float fs = 48000, f = 2100;
	auto run = [&](bool corrected) {
		BdrSection s; std::vector<float> y;
		double ph = 0.37;
		for (int i = 0; i < 48000; i++) {
			ph += f / fs; if (ph >= 1) ph -= 1;
			// a large sine: crosses the thresholds steeply but smoothly, so interpolation finds the instant
			float sel = 2.25f + 10.f * (float) std::sin(2 * M_PI * ph);
			s.process(sel, 0.f, 2.5f, -2.5f);
			y.push_back(corrected ? s.bOut : s.nb);
		}
		return y;
	};
	auto yc = run(true), yn = run(false);
	// alias energy: everything except the odd harmonics of 2.1 kHz below 20 kHz
	auto aliasPower = [&](const std::vector<float>& y) {
		double p = 0;
		for (double fr = 150; fr < 20000; fr += 97) {
			bool nearHarm = false;
			for (int k = 1; k * f < 24000; k++) if (std::fabs(fr - k * f) < 150) nearHarm = true;
			if (!nearHarm) { double a = amp2(y, fr, fs, 4800); p += a * a; }
		}
		return p;
	};
	double pc = aliasPower(yc), pn = aliasPower(yn);
	std::printf("    alias reduction %.1f dB\n", 10 * std::log10(pn / pc));
	CHECK(10 * std::log10(pn / pc) > 30.0);
	// fundamental still there
	CHECK_NEAR(amp2(yc, f, fs, 4800), 4 / M_PI * 2.5, 0.3);
}

TEST("BDR: sections are independent") {
	BdrSection s[3];
	for (int i = 0; i < 5; i++) { s[0].process(5, 1, 0, 0); s[1].process(0, 1, 0, 0); s[2].process(5, 1, 0, 0); }
	CHECK(s[0].a2Out == 1.f && s[1].a1Out == 1.f && s[2].a2Out == 1.f);
}

// ----------------------------------------------------------------- PDIV ----
TEST("PDIV: counts 2 to 8") {
	PulseDivider d;
	int edges[9] = {}, first[9] = {}; float prev[9] = {};
	int highs[9] = {};
	for (int p = 1; p <= 840; p++) {
		for (int k = 0; k < 10; k++) {
			d.process(k < 5 ? 5.f : 0.f);
			for (int n = 2; n <= 8; n++) {
				if (d.out[n] > 2.5f && prev[n] < 2.5f) { edges[n]++; if (!first[n]) first[n] = p; }
				if (d.out[n] > 2.5f) highs[n]++;
				prev[n] = d.out[n];
				CHECK(d.out[n] == 0.f || d.out[n] == 5.f);
			}
		}
	}
	for (int n = 2; n <= 8; n++) { CHECK(edges[n] == 840 / n); CHECK(first[n] == n); }
	CHECK(highs[2] == 840 * 10 / 2);
	CHECK(highs[5] == 840 * 10 / 5);
	for (int n = 2; n <= 8; n++) CHECK(d.out[n] == 5.f);   // all high together on pulse 840
}

TEST("PDIV: audio-rate sub-harmonics, clean counting") {
	PulseDivider d; float fs = 48000; std::vector<float> o2, o8;
	for (int i = 0; i < 48000; i++) {
		d.process(2.5f * (float) std::sin(2 * M_PI * 800 * i / fs) + 2.5f);   // 0..5 V sine
		o2.push_back(d.out[2] - 2.5f); o8.push_back(d.out[8] - 2.5f);
	}
	CHECK_NEAR(zcFreq(o2, fs, 1000), 400, 2);
	CHECK_NEAR(zcFreq(o8, fs, 1000), 100, 1);
	// a slow ramp crossing 2.5 V once counts once; hovering between 2.0 and 2.5 does not count again
	PulseDivider r;
	for (int i = 0; i <= 1000; i++) r.process(i * 0.003f);
	for (int i = 0; i < 1000; i++) r.process(2.1f + 0.35f * (float) std::sin(i * 0.1));
	CHECK(r.count[2] == 1 && r.count[8] == 1);
}

// ------------------------------------------------------------------ FRS ----
static float freqKnobFor(float hz) { return (float) (std::log2(hz / 0.05) / 16.61); }

static void runFrs(float fs, std::vector<float> (*sig)(float, int), float shift,
                   std::vector<float>& up, std::vector<float>& down, int n) {
	Frs f; f.setRate(fs);
	std::vector<float> x = sig(fs, n);
	float k = freqKnobFor(shift);
	for (int i = 0; i < n; i++) {
		f.process(x[i], k, 0, 0, 0, 1, 1, false, 0, false, 0);
		up.push_back(f.outUp); down.push_back(f.outDown);
	}
}

static float gFreq = 1000;
static std::vector<float> sine1(float fs, int n) {
	std::vector<float> x(n);
	for (int i = 0; i < n; i++) x[i] = 2.5f * (float) std::sin(2 * M_PI * gFreq * i / fs);
	return x;
}

TEST("FRS: shifts 1 kHz by 100 Hz, sidebands and carrier rejected") {
	float fs = 48000; gFreq = 1000;
	std::vector<float> up, dn; runFrs(fs, sine1, 100, up, dn, 48000);
	CHECK_NEAR(Frs::shiftHz(freqKnobFor(100), 0, 0, 0), 100, 0.05);
	double u = amp2(up, 1100, fs, 9600);
	CHECK_NEAR(db(u / 2.5), 0, 0.1);
	CHECK(db(amp2(up, 900, fs, 9600) / 2.5) < -50);
	CHECK(db(amp2(up, 1000, fs, 9600) / 2.5) < -80);
	CHECK(db(amp2(up, 100, fs, 9600) / 2.5) < -80);
	CHECK_NEAR(db(amp2(dn, 900, fs, 9600) / 2.5), 0, 0.1);
	CHECK(db(amp2(dn, 1100, fs, 9600) / 2.5) < -50);
}

TEST("FRS: opposite sideband rejected across the band at four rates") {
	for (float fs : {44100.f, 48000.f, 96000.f, 192000.f}) {
		double worst = -200;
		for (float fin : {30.f, 100.f, 3000.f, 10000.f, 16000.f}) {
			gFreq = fin;
			std::vector<float> up, dn; runFrs(fs, sine1, 7, up, dn, (int) fs * 2);
			double main = amp2(up, fin + 7, fs, (size_t) fs / 2);
			double other = amp2(up, fin - 7, fs, (size_t) fs / 2);
			worst = std::max(worst, db(other / main));
		}
		std::printf("    %g Hz: worst opposite sideband %.1f dB\n", fs, worst);
		CHECK(worst < -45);
	}
}

static std::vector<float> chord(float fs, int n) {
	std::vector<float> x(n);
	for (int i = 0; i < n; i++) { double t = i / fs; x[i] = (float) (0.8 * (std::sin(2 * M_PI * 200 * t) + std::sin(2 * M_PI * 400 * t) + std::sin(2 * M_PI * 600 * t))); }
	return x;
}

TEST("FRS: not a transposer, through zero, no aliasing") {
	float fs = 48000;
	std::vector<float> up, dn; runFrs(fs, chord, 50, up, dn, 48000);
	for (float f : {250.f, 450.f, 650.f}) CHECK_NEAR(amp2(up, f, fs, 9600), 0.8, 0.02);
	CHECK(amp2(up, 300, fs, 9600) < 0.01);
	gFreq = 300; up.clear(); dn.clear(); runFrs(fs, sine1, 500, up, dn, 48000);
	CHECK_NEAR(amp2(dn, 200, fs, 9600), 2.5, 0.05);
	// 18 kHz shifted up 8 kHz is 26 kHz: above the host Nyquist rate, so it must be removed, not folded to 22 kHz.
	gFreq = 18000; up.clear(); dn.clear(); runFrs(fs, sine1, 8000, up, dn, 48000);
	double worst = 0, worstF = 0;
	for (double f = 100; f < 20000; f += 50) {
		if (std::fabs(f - 10000) < 200) continue;   // the opposite sideband, checked separately
		double a = amp2(up, f, fs, 9600); if (a > worst) { worst = a; worstF = f; }
	}
	std::printf("    worst alias below 20 kHz %.1f dB at %.0f Hz; opposite sideband %.1f dB\n", db(worst / 2.5), worstF, db(amp2(up, 10000, fs, 9600) / 2.5));
	CHECK(db(worst / 2.5) < -70);
	CHECK(db(amp2(up, 10000, fs, 9600) / 2.5) < -45);
	CHECK(db(amp2(up, 22000, fs, 9600) / 2.5) < -60);
}

TEST("FRS: oscillator, VCAs, COMP, no squelch") {
	for (float fs : {44100.f, 48000.f, 96000.f, 192000.f}) {
		Frs f; f.setRate(fs); std::vector<float> s0, s90;
		float k = freqKnobFor(440);
		for (int i = 0; i < (int) fs; i++) { f.process(0, k, 0, 0, 0, 1, 1, false, 0, false, 0); s0.push_back(f.osc0); s90.push_back(f.osc90); }
		CHECK_NEAR(zcFreq(s0, fs, 100) / 440, 1, 0.001);
		CHECK_NEAR(amp2(s0, 440, fs), 2.5, 0.02);
		// 90 leads 0 by a quarter cycle
		std::complex<double> a0 = 0, a90 = 0;
		for (size_t i = 0; i < s0.size(); i++) { auto e = std::exp(std::complex<double>(0, -2 * M_PI * 440 * i / fs)); a0 += (double) s0[i] * e; a90 += (double) s90[i] * e; }
		double d = std::arg(a90 / a0) * 180 / M_PI;
		CHECK_NEAR(d, 90, 0.5);
	}
	CHECK_NEAR(Frs::shiftHz(freqKnobFor(100), 0, 0, 1), 200, 0.1);
	// VCAs
	float fs = 48000; gFreq = 1000;
	{
		Frs f; f.setRate(fs); std::vector<float> u, d; auto x = sine1(fs, 24000);
		for (int i = 0; i < 24000; i++) { f.process(x[i], freqKnobFor(100), 0, 0, 0, 0, 1, false, 0, true, 2.5f); u.push_back(f.outUp); d.push_back(f.outDown); }
		CHECK(rms(u, 1000, 24000) == 0.0);
		CHECK_NEAR(amp2(d, 900, fs, 4800), 1.25, 0.02);
	}
	// COMP 2:1
	for (float a : {0.25f, 2.5f}) {
		Frs f; f.setRate(fs); float pk = 0;
		for (int i = 0; i < 48000; i++) { f.process(a * (float) std::sin(2 * M_PI * 200 * i / fs), 0.5f, 0, 0, 0, 1, 1, false, 0, false, 0); if (i > 24000) pk = std::max(pk, std::fabs(f.comp)); }
		float want = a == 2.5f ? 2.5f : 0.79f;
		CHECK_NEAR(pk / want, 1, 0.05);
	}
	// no squelch: 5 mV in, 5 mV out
	{
		Frs f; f.setRate(fs); std::vector<float> u;
		for (int i = 0; i < 48000; i++) { f.process(0.005f * (float) std::sin(2 * M_PI * 1000 * i / fs), freqKnobFor(100), 0, 0, 0, 1, 1, false, 0, false, 0); u.push_back(f.outUp); }
		CHECK_NEAR(amp2(u, 1100, fs, 9600), 0.005, 0.0002);
	}
}

// ------------------------------------------------------------------ WAD ----
static float delayKnobForV(float V) { return 1.f - V / 12.32f; }

struct WadRun {
	Wad w; float fs;
	float delay, vcDelay = 0, delayCv = 0, filter = 1, in3 = 0, flangeK = 1; int src = 0;
	explicit WadRun(float fs_, float V) : fs(fs_), delay(delayKnobForV(V)) { w.noise = false; }
	void step(float in1, float aux = 0) { w.process(fs, delay, vcDelay, delayCv, filter, 0, 0, 1, in1, 0, 0, in3, src, aux, flangeK); }
};

static double stepDelay(float fs, float V, bool tapB) {
	WadRun r(fs, V); int n = (int) (fs * 2.5);
	for (int i = 0; i < n; i++) {
		r.step(i >= 100 ? 1.f : 0.f);
		float y = tapB ? r.w.b / 0.891f : r.w.a;
		if (y > 0.5f) return (i - 100) / fs;
	}
	return -1;
}

TEST("WAD: delay times, B twice A, sample-rate independence") {
	for (float V : {3.f, 6.f, 9.f}) {
		double fclk = 200 * std::exp2(V);
		double a = stepDelay(48000, V, false), b = stepDelay(48000, V, true);
		double tol = 1 / fclk + 3 / 48000.0;
		CHECK_NEAR(a, 512 / fclk, tol);
		CHECK_NEAR(b, 1024 / fclk, tol);
	}
	for (float V : {4.f, 5.f, 7.f, 8.f, 10.f}) {
		double a = stepDelay(48000, V, false), b = stepDelay(48000, V, true);
		CHECK_NEAR(b / a, 2.0, 0.02);
	}
	double ref = stepDelay(48000, 6, false);
	for (float fs : {44100.f, 96000.f, 192000.f}) CHECK_NEAR(stepDelay(fs, 6, false) / ref, 1, 0.005);
}

TEST("WAD: 1 V/oct control of the clock") {
	WadRun r(48000, 6); r.vcDelay = 1; r.delayCv = 0;
	r.step(0); float v0 = r.w.voct; double f0 = r.w.fClk;
	CHECK_NEAR(v0, 0, 1e-4); CHECK_NEAR(f0, 12800, 1);
	r.delayCv = 1; r.step(0);
	CHECK_NEAR(r.w.voct - v0, 1, 1e-4);
	CHECK_NEAR(r.w.fClk / f0, 2, 0.001);
	// and the delay halves
	WadRun s(48000, 6); s.vcDelay = 1; s.delayCv = 1;
	double d = -1;
	for (int i = 0; i < 48000 && d < 0; i++) { s.step(i >= 100 ? 1.f : 0.f); if (s.w.a > 0.5f) d = (i - 100) / 48000.0; }
	CHECK_NEAR(d / 0.020, 1, 0.005 + 3 / (48000 * 0.02));
}

TEST("WAD: clock aliasing present, filter removes it") {
	float fs = 48000;
	float V = (float) std::log2(4000.0 / 200);   // 4 kHz clock
	auto run = [&](float filt) {
		WadRun r(fs, V); r.filter = filt; std::vector<float> y;
		for (int i = 0; i < 96000; i++) { r.step((float) std::sin(2 * M_PI * 3000 * i / fs)); y.push_back(r.w.a); }
		return amp2(y, 1000, fs, 48000);
	};
	double open = run(1), closed = run((float) (std::log2(400.0 / 20) / 10));
	std::printf("    1 kHz alias open %.3f V, filtered %.1f dB\n", open, db(closed / open));
	CHECK(db(open / 1.0) > -6);
	CHECK(db(closed / open) < -40);
}

TEST("WAD: echoes decay by the feedback ratio and invert") {
	float fs = 48000;
	for (float k : {0.7f, -0.7f}) {
		WadRun r(fs, 6); r.in3 = k; std::vector<float> y;
		for (int i = 0; i < (int) (fs * 0.5); i++) {
			float x = (i < 240) ? (float) std::sin(2 * M_PI * 1000 * i / fs) : 0.f;
			r.step(x); y.push_back(r.w.a);
		}
		// echo n starts at n * 40 ms (plus a little)
		double prev = 0; float prevSign = 0;
		for (int n = 1; n <= 4; n++) {
			size_t a0 = (size_t) (n * 0.040 * fs), a1 = a0 + 400;
			double e = rms(y, a0, a1);
			// sign: correlate with the original burst
			double c = 0; for (int i = 0; i < 240; i++) c += y[a0 + i + 2] * std::sin(2 * M_PI * 1000 * i / fs);
			float sign = c > 0 ? 1.f : -1.f;
			if (n > 1) {
				double ratio = e / prev;
				CHECK(ratio > 0.6 && ratio < 0.72);
				if (k < 0) CHECK(sign == -prevSign); else CHECK(sign == prevSign);
			}
			prev = e; prevSign = sign;
		}
	}
}

TEST("WAD: Karplus-Strong at 220 Hz") {
	float fs = 48000;
	WadRun r(fs, (float) std::log2(112640.0 / 200)); r.in3 = 0.98f; std::vector<float> y;
	Rng g; g.seed(5);
	for (int i = 0; i < 48000; i++) { r.step(i < 48 ? g.gauss() : 0.f); y.push_back(r.w.a); }
	// pitch by autocorrelation (zero crossings would follow the bright upper partials)
	double best = -1e9; int lag = 0;
	for (int L = 150; L <= 300; L++) { double c = 0; for (int i = 12000; i < 36000; i++) c += (double) y[i] * y[i + L]; if (c > best) { best = c; lag = L; } }
	// refine with a parabola through the peak
	auto ac = [&](int L) { double c = 0; for (int i = 12000; i < 36000; i++) c += (double) y[i] * y[i + L]; return c; };
	double c0 = ac(lag - 1), c1 = ac(lag), c2 = ac(lag + 1);
	double f = fs / (lag + 0.5 * (c0 - c2) / (c0 - 2 * c1 + c2));
	std::printf("    rings at %.2f Hz\n", f);
	CHECK(f > 220 * 0.97 && f <= 220.5);
}

TEST("WAD: flange notches") {
	float fs = 48000;
	float V = (float) std::log2(512.0 / 0.001 / 200);   // A delay 1 ms
	auto level = [&](double f) {
		WadRun r(fs, V); std::vector<float> y;
		for (int i = 0; i < 9600; i++) { r.step((float) std::sin(2 * M_PI * f * i / fs)); y.push_back(r.w.flange); }
		return rms(y, 2400, 9600);
	};
	for (double target : {500.0, 1500.0, 2500.0}) {
		double best = 1e9, bestF = 0;
		for (double f = target * 0.93; f <= target * 1.07; f += target * 0.004) { double l = level(f); if (l < best) { best = l; bestF = f; } }
		double peak = level(target * 2);   // a peak between notches (1 kHz, 3 kHz, 5 kHz)
		std::printf("    notch %.0f Hz at %.1f Hz, %.1f dB\n", target, bestF, db(best / peak));
		CHECK(std::fabs(bestF / target - 1) < 0.05);
		CHECK(db(best / peak) < -20);
	}
}

TEST("WAD: silence, and noise floor when on") {
	WadRun r(48000, 6);
	for (int i = 0; i < 48000; i++) r.step(0);
	CHECK(r.w.a == 0.f && r.w.b == 0.f && r.w.flange == 0.f);
	r.w.noise = true; std::vector<float> y;
	for (int i = 0; i < 48000; i++) { r.step(0); y.push_back(r.w.a); }
	double n = rms(y, 4800, 48000);
	CHECK(n > 0.0002 && n < 0.003);
}

// ------------------------------------------------------------ stability ----
TEST("Round two stability: noise into everything at four rates") {
	for (float fs : {44100.f, 48000.f, 96000.f, 192000.f}) {
		Rng r; r.seed((uint64_t) fs + 7);
		BdrSection b; PulseDivider d; Frs f; f.setRate(fs); Wad w;
		int bad = 0;
		auto ok = [](float v) { return std::isfinite(v) && std::fabs(v) <= RAIL + 1e-3f; };
		float k[12]; int src = 0;
		for (int i = 0; i < (int) (fs * 2); i++) {
			auto g = [&] { return r.gauss() * 10.f; };
			if (i % 4800 == 0) { for (float& x : k) x = r.uniform(); src = (int) (r.uniform() * 3); k[6] = k[6] * 2 - 1; }
			b.process(g(), g(), g(), g());
			d.process(g());
			f.process(g(), k[0], k[1] * 2 - 1, g(), g(), k[2], k[3], true, g(), false, 0);
			w.process(fs, k[4], k[5] * 2 - 1, g(), k[7], k[8] * 2 - 1, g(), k[9], g(), k[10], g(), k[6] < 0 ? -1.f : 1.f, src, g(), k[11]);
			float outs[] = {b.a1Out, b.a2Out, b.bOut, f.outUp, f.outDown, f.comp, f.osc0, f.osc90, w.a, w.b, w.flange, w.voct};
			for (size_t j = 0; j < sizeof(outs) / sizeof(outs[0]); j++) if (!ok(outs[j])) { bad++; if (bad < 4) std::printf("    output %zu = %g\n", j, outs[j]); }
			for (int n = 2; n <= 8; n++) if (d.out[n] != 0.f && d.out[n] != 5.f) bad++;
		}
		if (bad) std::printf("    %g Hz: %d bad samples\n", fs, bad);
		CHECK(bad == 0);
	}
}
