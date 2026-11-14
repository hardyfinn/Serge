// Dual Universal Slope Generator: one section.
#pragma once
#include "Slew.hpp"

namespace serge {

struct DsgSection {
	enum VcMode { VC_RISE = 0, VC_BOTH = 1, VC_FALL = 2 };

	SlewCore slew;
	SchmittEdge trig;
	bool latch = false;
	bool falling = false;
	float out = 0.f;
	float end = PULSE_HI;

	// 0 to 1 knob -> V/s. k=0: 20 s per 5 V. k=1: 100 us per 5 V.
	static float rateKnob(float k) { return 0.25f * std::exp2(17.6f * k); }

	void process(float dt, float in, float trigIn, float vcIn, float voct,
	             float riseKnob, float fallKnob, float vcKnob, int vcMode) {
		float vc = vcKnob * vcIn;
		float rise = rateKnob(riseKnob) * std::exp2(voct + (vcMode != VC_FALL ? vc : 0.f));
		float fall = rateKnob(fallKnob) * std::exp2(voct + (vcMode != VC_RISE ? vc : 0.f));
		rise = clampf(rise, 0.005f, 150000.f);
		fall = clampf(fall, 0.005f, 150000.f);
		if (!finite(rise)) rise = 1.f;
		if (!finite(fall)) fall = 1.f;

		bool busy = latch || falling;
		if (trig.process(trigIn) && !busy) latch = true;

		float t = dt;
		if (latch) {
			float y0 = slew.y;
			float target = std::max(in, 6.f);
			slew.process(target, rise, fall, t);
			if (slew.y >= 5.f) {
				// Turn around inside the sample so the cycling pitch is exact.
				latch = false;
				if (y0 < 5.f && slew.y > 5.f && G_rateLimited(target - y0, rise)) {
					float tHit = (5.f - y0) / rise;
					float rem = clampf(t - tHit, 0.f, t);
					slew.y = 5.f;
					t = rem;
				}
				else {
					t = 0.f;
				}
				if (t > 0.f) slew.process(in, rise, fall, t);
			}
		}
		else {
			slew.process(in, rise, fall, t);
		}

		float target = latch ? std::max(in, 6.f) : in;
		float d = slew.y - target;
		if (!falling && d > 0.020f) falling = true;
		else if (falling && d < 0.005f) falling = false;

		out = slew.y;
		end = falling ? 0.f : PULSE_HI;
		if (!finite(out)) { slew.y = 0.f; out = 0.f; latch = falling = false; }
	}

	static bool G_rateLimited(float e, float rate) { return SlewCore::G * e > rate; }
};

} // namespace serge
