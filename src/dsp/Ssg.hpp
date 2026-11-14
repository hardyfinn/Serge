// Smooth & Stepped Generator.
#pragma once
#include "Slew.hpp"

namespace serge {

struct SsgSection {
	SlewCore slew;
	float cycle = RAIL;   // power-up state
	static float rate(float knob, float vcKnob, float vcIn) {
		float r = 0.5f * std::exp2(12.f * knob) * std::exp2(vcKnob * vcIn);
		if (!finite(r)) r = 1.f;
		return clampf(r, 0.01f, 100000.f);
	}
	void updateCycle() {
		if (slew.y > 2.5f) cycle = -RAIL;
		else if (slew.y < -2.5f) cycle = RAIL;
	}
};

struct Ssg {
	SsgSection smooth, stepped;
	SchmittEdge sample;
	float timer = 0.f;   // one-shot remaining time, seconds
	static constexpr float ONE_SHOT = 0.001f;

	float coupler = 0.f, hot = -RAIL;

	void process(float dt,
	             float smIn, float smVcIn, float smHold, float smRate, float smVc,
	             float stIn, float stVcIn, float stSample, float stRate, float stVc) {
		// Smooth: tracks unless HOLD is high (plain level test).
		float r1 = SsgSection::rate(smRate, smVc, smVcIn);
		if (smHold < 2.5f) smooth.slew.process(smIn, r1, r1, dt);
		smooth.updateCycle();

		// Stepped: tracks only during a 1 ms one-shot fired by SAMPLE rising edges.
		float r2 = SsgSection::rate(stRate, stVc, stVcIn);
		if (sample.process(stSample)) timer = ONE_SHOT;
		if (timer > 0.f) {
			float step = std::min(timer, dt);
			stepped.slew.process(stIn, r2, r2, step);
			timer -= dt;
		}
		stepped.updateCycle();

		bool hi = stepped.slew.y > smooth.slew.y;
		coupler = hi ? PULSE_HI : 0.f;
		hot = hi ? RAIL : -RAIL;
	}
};

} // namespace serge
