#include "../plugin.hpp"
#include "../dsp/Frs.hpp"
#include "../generated/Layout.hpp"

struct FRS : Module {
	enum ParamId { FREQ_PARAM, VCFREQ_PARAM, UP_PARAM, DOWN_PARAM, PARAMS_LEN };
	enum InputId { IN_INPUT, FREQCV_INPUT, VOCT_INPUT, VCUP_INPUT, VCDOWN_INPUT, INPUTS_LEN };
	enum OutputId { UP_OUTPUT, DOWN_OUTPUT, COMP_OUTPUT, OSC0_OUTPUT, OSC90_OUTPUT, OUTPUTS_LEN };
	enum LightId { LIGHTS_LEN };

	serge::Frs frs;

	FRS() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(FREQ_PARAM, 0.f, 1.f, 0.5f, "FREQ (shift)", " Hz", std::exp2(16.61f), 0.05f);
		configParam(VCFREQ_PARAM, -1.f, 1.f, 0.f, "VC FREQ (FREQ CV amount, oct/V)");
		configParam(UP_PARAM, 0.f, 1.f, 1.f, "UP level", "%", 0.f, 100.f);
		configParam(DOWN_PARAM, 0.f, 1.f, 1.f, "DOWN level", "%", 0.f, 100.f);
		configInput(IN_INPUT, "IN");
		configInput(FREQCV_INPUT, "FREQ CV");
		configInput(VOCT_INPUT, "1V/OCT");
		configInput(VCUP_INPUT, "VC UP (0 to 5 V, scaled by UP)");
		configInput(VCDOWN_INPUT, "VC DOWN (0 to 5 V, scaled by DOWN)");
		configOutput(UP_OUTPUT, "UP (shifted up)");
		configOutput(DOWN_OUTPUT, "DOWN (shifted down)");
		configOutput(COMP_OUTPUT, "COMP (compressed input)");
		configOutput(OSC0_OUTPUT, "0° (oscillator sine)");
		configOutput(OSC90_OUTPUT, "90° (oscillator sine, a quarter cycle ahead)");
		frs.setRate(48000.f);
	}

	void onReset() override { frs.reset(); }
	void onSampleRateChange(const SampleRateChangeEvent& e) override { frs.setRate(e.sampleRate); }

	void process(const ProcessArgs&) override {
		frs.process(inputs[IN_INPUT].getVoltage(),
		            params[FREQ_PARAM].getValue(), params[VCFREQ_PARAM].getValue(),
		            inputs[FREQCV_INPUT].getVoltage(), inputs[VOCT_INPUT].getVoltage(),
		            params[UP_PARAM].getValue(), params[DOWN_PARAM].getValue(),
		            inputs[VCUP_INPUT].isConnected(), inputs[VCUP_INPUT].getVoltage(),
		            inputs[VCDOWN_INPUT].isConnected(), inputs[VCDOWN_INPUT].getVoltage(),
		            outputs[UP_OUTPUT].isConnected(), outputs[DOWN_OUTPUT].isConnected());
		outputs[UP_OUTPUT].setVoltage(frs.outUp);
		outputs[DOWN_OUTPUT].setVoltage(frs.outDown);
		outputs[COMP_OUTPUT].setVoltage(frs.comp);
		outputs[OSC0_OUTPUT].setVoltage(frs.osc0);
		outputs[OSC90_OUTPUT].setVoltage(frs.osc90);
	}
};

struct FRSWidget : ModuleWidget {
	FRSWidget(FRS* m) { sergeScaffold(this, m, "FRS"); placeFRS(this, m); }
};

Model* modelFRS = createModel<FRS, FRSWidget>("FRS");
