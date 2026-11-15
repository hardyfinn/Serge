#include "../plugin.hpp"
#include "../dsp/Wad.hpp"
#include "../generated/Layout.hpp"

struct WAD : Module {
	enum ParamId { DELAY_PARAM, VCDELAY_PARAM, FILTER_PARAM, VCFILTER_PARAM, IN1_PARAM, IN2_PARAM, IN3_PARAM, FLANGE_PARAM, SOURCE_PARAM, PARAMS_LEN };
	enum InputId { IN1_INPUT, IN2_INPUT, AUX_INPUT, DELAYCV_INPUT, FILTERCV_INPUT, INPUTS_LEN };
	enum OutputId { A_OUTPUT, B_OUTPUT, FLANGE_OUTPUT, VOCT_OUTPUT, OUTPUTS_LEN };
	enum LightId { LIGHTS_LEN };

	serge::Wad wad;

	WAD() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(DELAY_PARAM, 0.f, 1.f, 0.5f, "DELAY (clockwise is longer)");
		configParam(VCDELAY_PARAM, -1.f, 1.f, 0.f, "VC DELAY (DELAY CV amount, oct/V)");
		configParam(FILTER_PARAM, 0.f, 1.f, 1.f, "FILTER (input and output filter cutoff)", " Hz", 1024.f, 20.f);
		configParam(VCFILTER_PARAM, -1.f, 1.f, 0.f, "VC FILTER (FILTER CV amount, oct/V)");
		configParam(IN1_PARAM, 0.f, 1.f, 1.f, "IN 1 level", "%", 0.f, 100.f);
		configParam(IN2_PARAM, 0.f, 1.f, 0.f, "IN 2 level", "%", 0.f, 100.f);
		configParam(IN3_PARAM, -1.f, 1.f, 0.f, "IN 3 (feedback, scales and inverts)", "%", 0.f, 100.f);
		configParam(FLANGE_PARAM, 0.f, 1.f, 1.f, "FLANGE depth", "%", 0.f, 100.f);
		configSwitch(SOURCE_PARAM, 0.f, 2.f, 0.f, "IN 3 source", {"A", "B", "AUX"});
		configInput(IN1_INPUT, "IN 1");
		configInput(IN2_INPUT, "IN 2 (audio or control voltage)");
		configInput(AUX_INPUT, "AUX (feedback return)");
		configInput(DELAYCV_INPUT, "DELAY CV");
		configInput(FILTERCV_INPUT, "FILTER CV");
		configOutput(A_OUTPUT, "A (delayed)");
		configOutput(B_OUTPUT, "B (twice the delay of A)");
		configOutput(FLANGE_OUTPUT, "FLANGE");
		configOutput(VOCT_OUTPUT, "1V/OCT (follows the delay clock, for filters)");
	}

	void onReset() override { wad.reset(); }
	void onSampleRateChange(const SampleRateChangeEvent&) override { wad.reset(); }

	void process(const ProcessArgs& args) override {
		wad.process(args.sampleRate,
		            params[DELAY_PARAM].getValue(), params[VCDELAY_PARAM].getValue(), inputs[DELAYCV_INPUT].getVoltage(),
		            params[FILTER_PARAM].getValue(), params[VCFILTER_PARAM].getValue(), inputs[FILTERCV_INPUT].getVoltage(),
		            params[IN1_PARAM].getValue(), inputs[IN1_INPUT].getVoltage(),
		            params[IN2_PARAM].getValue(), inputs[IN2_INPUT].getVoltage(),
		            params[IN3_PARAM].getValue(), (int) params[SOURCE_PARAM].getValue(), inputs[AUX_INPUT].getVoltage(),
		            params[FLANGE_PARAM].getValue());
		outputs[A_OUTPUT].setVoltage(wad.a);
		outputs[B_OUTPUT].setVoltage(wad.b);
		outputs[FLANGE_OUTPUT].setVoltage(wad.flange);
		outputs[VOCT_OUTPUT].setVoltage(wad.voct);
	}
};

struct WADWidget : ModuleWidget {
	WADWidget(WAD* m) { sergeScaffold(this, m, "WAD"); placeWAD(this, m); }
};

Model* modelWAD = createModel<WAD, WADWidget>("WAD");
