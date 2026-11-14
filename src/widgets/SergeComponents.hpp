// Banana jacks, Davies-style knob, bat-handle toggles. SVGs come from scripts/make_panels.py.
#pragma once
#include "../plugin.hpp"

inline std::shared_ptr<window::Svg> sergeSvg(const std::string& name) {
	return window::Svg::load(asset::plugin(pluginInstance, "res/components/" + name + ".svg"));
}

struct SergeKnob : app::SvgKnob {
	SergeKnob() {
		minAngle = -0.75f * M_PI;
		maxAngle = 0.75f * M_PI;
		setSvg(sergeSvg("SergeKnob"));
		shadow->opacity = 0.15f;
	}
};

struct JackBlack : app::SvgPort { JackBlack() { setSvg(sergeSvg("JackBlack")); } };
struct JackBlue : app::SvgPort { JackBlue() { setSvg(sergeSvg("JackBlue")); } };
struct JackRed : app::SvgPort { JackRed() { setSvg(sergeSvg("JackRed")); } };
struct JackYellow : app::SvgPort { JackYellow() { setSvg(sergeSvg("JackYellow")); } };

struct SergeToggle2 : app::SvgSwitch {
	SergeToggle2() {
		addFrame(sergeSvg("Toggle2_0"));
		addFrame(sergeSvg("Toggle2_1"));
	}
};

struct SergeToggle3 : app::SvgSwitch {
	SergeToggle3() {
		addFrame(sergeSvg("Toggle3_0"));
		addFrame(sergeSvg("Toggle3_1"));
		addFrame(sergeSvg("Toggle3_2"));
	}
};

// Shared widget scaffold: panel, screws, generated placement.
template <class W>
inline void sergeScaffold(W* w, Module* m, const std::string& slug) {
	w->setModule(m);
	w->setPanel(createPanel(asset::plugin(pluginInstance, "res/" + slug + ".svg")));
	float width = w->box.size.x;
	w->addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
	w->addChild(createWidget<ScrewBlack>(Vec(width - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
	if (width > 6 * RACK_GRID_WIDTH) {
		w->addChild(createWidget<ScrewBlack>(Vec(width - 2 * RACK_GRID_WIDTH, 0)));
		w->addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
	}
}

// Oversampling state shared by the nonlinear modules.
#include "../dsp/Oversample.hpp"
struct OsState {
	serge::OsKernel kernel;
	int factor = 1;
	float fs = 48000.f;
	void setRate(float sampleRate) {
		fs = sampleRate;
		factor = serge::oversampleFactor(sampleRate);
		kernel.design(factor);
	}
	float fsOs() const { return fs * factor; }
	float dtOs() const { return 1.f / (fs * factor); }
};
