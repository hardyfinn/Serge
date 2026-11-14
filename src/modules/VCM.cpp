#include "../plugin.hpp"
#include "../dsp/Shapers.hpp"
#include "../generated/Layout.hpp"

struct VCM : Module {
	enum ParamId { VC1_PARAM, HILO_PARAM, FOLD_PARAM, VC3_PARAM, PARAMS_LEN };
	enum InputId { IN1_INPUT, VC1_INPUT, IN2A_INPUT, IN2B_INPUT, VC2_INPUT, IN3A_INPUT, IN3B_INPUT, VC3_INPUT, INPUTS_LEN };
	enum OutputId { OUT1_OUTPUT, OUT2_OUTPUT, OUT3_OUTPUT, SQUARED_OUTPUT, OUTPUTS_LEN };
	enum LightId { LIGHTS_LEN };

	OsState os;
	serge::Upsampler up[3];
	serge::Decimator dec[4];
	serge::Smoother sm[3];

	VCM() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(VC1_PARAM, 0.f, 1.f, 1.f, "1: VC (level)", "%", 0.f, 100.f);
		configSwitch(HILO_PARAM, 0.f, 1.f, 1.f, "1: HI / LO", {"HI (squared up)", "LO (linear VCA)"});
		configParam(FOLD_PARAM, 0.f, 1.f, 0.f, "2: FOLD", "%", 0.f, 100.f);
		configParam(VC3_PARAM, 0.f, 1.f, 0.f, "3: VC (rectifier stages)", "%", 0.f, 100.f);
		configInput(IN1_INPUT, "1: IN");
		configInput(VC1_INPUT, "1: VC");
		configInput(IN2A_INPUT, "2: IN 1");
		configInput(IN2B_INPUT, "2: IN 2");
		configInput(VC2_INPUT, "2: VC");
		configInput(IN3A_INPUT, "3: IN 1");
		configInput(IN3B_INPUT, "3: IN 2");
		configInput(VC3_INPUT, "3: VC");
		configOutput(OUT1_OUTPUT, "1: OUT");
		configOutput(OUT2_OUTPUT, "2: OUT");
		configOutput(OUT3_OUTPUT, "3: OUT");
		configOutput(SQUARED_OUTPUT, "3: SQUARED");
		os.setRate(48000.f);
	}

	void onSampleRateChange(const SampleRateChangeEvent& e) override {
		os.setRate(e.sampleRate);
		for (auto& u : up) u.reset();
		for (auto& d : dec) d.reset();
	}

	void process(const ProcessArgs& args) override {
		int F = os.factor;
		float x[serge::OS_MAX], y[serge::OS_MAX], z[serge::OS_MAX];
		float dt = args.sampleTime;
		// 1: VCA / squarer
		if (outputs[OUT1_OUTPUT].isConnected()) {
			float g = sm[0].process(clamp(params[VC1_PARAM].getValue() + inputs[VC1_INPUT].getVoltage() / 5.f, 0.f, 1.f), dt);
			bool hi = params[HILO_PARAM].getValue() < 0.5f;
			up[0].process(os.kernel, inputs[IN1_INPUT].getVoltage(), x);
			for (int k = 0; k < F; k++) y[k] = serge::vcmTop(x[k], g, hi);
			outputs[OUT1_OUTPUT].setVoltage(dec[0].process(os.kernel, y));
		}
		// 2: folder
		if (outputs[OUT2_OUTPUT].isConnected()) {
			float c = sm[1].process(clamp(params[FOLD_PARAM].getValue() + inputs[VC2_INPUT].getVoltage() / 5.f, 0.f, 1.f), dt);
			up[1].process(os.kernel, inputs[IN2A_INPUT].getVoltage() + inputs[IN2B_INPUT].getVoltage(), x);
			for (int k = 0; k < F; k++) y[k] = serge::vcmMiddle(x[k], c);
			outputs[OUT2_OUTPUT].setVoltage(dec[1].process(os.kernel, y));
		}
		// 3: rectifiers
		if (outputs[OUT3_OUTPUT].isConnected() || outputs[SQUARED_OUTPUT].isConnected()) {
			float c = sm[2].process(clamp(params[VC3_PARAM].getValue() + inputs[VC3_INPUT].getVoltage() / 5.f, 0.f, 1.f), dt);
			up[2].process(os.kernel, inputs[IN3A_INPUT].getVoltage() + inputs[IN3B_INPUT].getVoltage(), x);
			for (int k = 0; k < F; k++) serge::vcmBottom(x[k], c, y[k], z[k]);
			outputs[OUT3_OUTPUT].setVoltage(dec[2].process(os.kernel, y));
			outputs[SQUARED_OUTPUT].setVoltage(clamp(dec[3].process(os.kernel, z), 0.f, 5.f));
		}
	}
};

struct VCMWidget : ModuleWidget {
	VCMWidget(VCM* m) { sergeScaffold(this, m, "VCM"); placeVCM(this, m); }
};

Model* modelVCM = createModel<VCM, VCMWidget>("VCM");
