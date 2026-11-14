// New Timbral Oscillator core. Runs at the oversampled rate.
#pragma once
#include "Common.hpp"
#include "Blep.hpp"

namespace serge {

struct Nto {
	float phase = 0.f;
	float portV = 0.f;
	bool portInit = false;
	float prevSync = 0.f;
	bool syncHigh = false;
	Blep bSaw, bTri, bSin, bVar;
	float saw = 0.f, tri = 0.f, sine = 0.f, var = 0.f;   // output volts (1 sample latency)

	static float shapeSin(float tri01) {
		float x = 2.f * tri01 - 1.f;
		return std::sin(0.5f * PI * x) + 0.02f * std::sin(1.5f * PI * x);
	}
	struct Vals { float saw, tri, sin, var; };
	static Vals values(float ph, float w) {
		float tri = 1.f - std::fabs(2.f * ph - 1.f);
		float s = shapeSin(tri);
		return {5.f * ph, 5.f * tri, 2.5f * s, 2.5f * ((1.f - w) * s + w * (2.f * ph - 1.f))};
	}
	void addJump(const Vals& before, const Vals& after, float a) {
		bSaw.add(after.saw - before.saw, a);
		bTri.add(after.tri - before.tri, a);
		bSin.add(after.sin - before.sin, a);
		bVar.add(after.var - before.var, a);
	}

	// Portamento runs per base sample. Returns the lagged pitch voltage.
	float portamento(float vPort, float portKnob, float dt) {
		float tau = 2.f * portKnob * portKnob * portKnob;
		if (!portInit || tau < 1e-4f) { portV = vPort; portInit = true; return portV; }
		portV += (vPort - portV) * (1.f - std::exp(-dt / tau));
		if (!finite(portV)) portV = 0.f;
		return portV;
	}

	// One oversampled step. fexp in Hz, depth 0..1, linfm volts, sync volts (interpolated), w 0..1.
	void step(float dt, float fexp, float depth, float linfm, float sync, float w) {
		float f = fexp * std::max(0.f, 1.f + depth * 2.f * linfm / 2.5f);
		f = clampf(f, 0.f, 0.45f / dt);
		if (!finite(f)) f = 0.f;
		float inc = f * dt;

		float ph0 = phase;
		float ph = ph0 + inc;
		// wrap
		if (ph >= 1.f) {
			ph -= 1.f;
			float a = (inc > 0.f) ? clampf(ph / inc, 0.f, 0.999f) : 0.f;
			addJump(values(1.f, w), values(0.f, w), a);
		}
		// sync: rising through 2.5 V, sub-sample position by linear interpolation
		if (!syncHigh && sync >= 2.5f) {
			syncHigh = true;
			float denom = sync - prevSync;
			float frac = denom > 1e-9f ? clampf((2.5f - prevSync) / denom, 0.f, 1.f) : 1.f;
			// phase at the edge
			float phEdge = ph0 + inc * frac;
			if (phEdge >= 1.f) phEdge -= 1.f;
			if (phEdge >= 0.70f) {
				float a = clampf(1.f - frac, 0.f, 0.999f);
				addJump(values(phEdge, w), values(0.f, w), a);
				ph = inc * (1.f - frac);
			}
		}
		else if (syncHigh && sync < 2.0f) syncHigh = false;
		prevSync = sync;
		phase = ph;
		if (!finite(phase) || phase < 0.f || phase >= 1.f) phase = 0.f;

		Vals v = values(phase, w);
		saw = bSaw.process(v.saw);
		tri = bTri.process(v.tri);
		sine = bSin.process(v.sin);
		var = bVar.process(v.var);
	}
};

} // namespace serge
