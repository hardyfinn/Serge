// BDR (Bi-Directional Router) and PDIV (Pulse Divider). Plain C++11, no Rack includes.
#pragma once
#include "Common.hpp"
#include "Blep.hpp"

namespace serge {

// One section of the Bi-Directional Router: a CMOS switch steered by SELECT.
// Rack ports are one-way, so the section carries both directions:
//   1 TO 2: B IN goes to A1 OUT or A2 OUT.
//   2 TO 1: A1 IN or A2 IN goes to B OUT.
struct BdrSection {
	bool selA2 = false;      // false: A1 selected (rest), true: A2
	float prevSelect = 0.f;
	float a1Out = 0.f, a2Out = 0.f, bOut = 0.f;           // band-limited outputs (one sample late)
	float n1 = 0.f, n2 = 0.f, nb = 0.f;                    // last naive outputs
	Blep blep1, blep2, blepB;

	static float pass(float v) { return clampf(v, -7.f, 7.f); }   // CMOS supply limit

	void process(float vSelect, float vBin, float vA1in, float vA2in) {
		bool was = selA2;
		float thr = 0.f;
		if (!selA2 && vSelect > 2.5f) { selA2 = true; thr = 2.5f; }
		else if (selA2 && vSelect < 2.0f) { selA2 = false; thr = 2.0f; }

		float b = pass(vBin);
		float x1 = selA2 ? 0.f : b;
		float x2 = selA2 ? b : 0.f;
		float xb = pass(selA2 ? vA2in : vA1in);

		if (selA2 != was) {
			// Where in this sample SELECT crossed its threshold, by linear interpolation.
			float d = vSelect - prevSelect;
			float t = (std::fabs(d) > 1e-9f) ? clampf((thr - prevSelect) / d, 0.f, 1.f) : 1.f;
			float a = 1.f - t;     // how long before the end of the sample the switch happened
			// Jump each output makes at the switching instant (from its old route to its new route).
			float old1 = was ? 0.f : b, old2 = was ? b : 0.f;
			float oldB = pass(was ? vA2in : vA1in);
			blep1.add(x1 - old1, a);
			blep2.add(x2 - old2, a);
			blepB.add(xb - oldB, a);
		}
		prevSelect = vSelect;
		n1 = x1; n2 = x2; nb = xb;
		// The band-limiting correction can overshoot by up to half a jump; keep it inside the rails.
		a1Out = railClip(blep1.process(x1));
		a2Out = railClip(blep2.process(x2));
		bOut = railClip(blepB.process(xb));
		if (!finite(a1Out) || !finite(a2Out) || !finite(bOut)) reset();
	}
	void reset() {
		selA2 = false; prevSelect = 0.f;
		a1Out = a2Out = bOut = n1 = n2 = nb = 0.f;
		blep1 = Blep(); blep2 = Blep(); blepB = Blep();
	}
};

// Pulse Divider: outputs on every 2nd .. 8th input pulse.
struct PulseDivider {
	SchmittEdge edge;
	int count[9] = {};
	float out[9] = {};   // out[2] .. out[8], 0 or 5 V

	void process(float vIn) {
		if (edge.process(vIn)) {
			for (int n = 2; n <= 8; n++) {
				count[n] = (count[n] + 1) % n;
				out[n] = (count[n] == 0) ? PULSE_HI : 0.f;
			}
		}
	}
	void reset() {
		edge.reset();
		for (int n = 0; n < 9; n++) { count[n] = 0; out[n] = 0.f; }
	}
};

} // namespace serge
