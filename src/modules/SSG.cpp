#include "../plugin.hpp"
#include "../dsp/Ssg.hpp"
#include "../generated/Layout.hpp"

struct SSG : Module {
	enum ParamId { RATE_SM_PARAM, VC_SM_PARAM, RATE_ST_PARAM, VC_ST_PARAM, PARAMS_LEN };
	enum InputId { IN_SM_INPUT, VC_SM_INPUT, HOLD_INPUT, IN_ST_INPUT, VC_ST_INPUT, SAMPLE_INPUT, INPUTS_LEN };
	enum OutputId { CYCLE_SM_OUTPUT, OUT_SM_OUTPUT, CYCLE_ST_OUTPUT, OUT_ST_OUTPUT, COUPLER_OUTPUT, HOT_OUTPUT, OUTPUTS_LEN };
	enum LightId { LEVEL_SM_LIGHT, LEVEL_ST_LIGHT, LIGHTS_LEN };

	serge::Ssg ssg;

	SSG() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(RATE_SM_PARAM, 0.f, 1.f, 0.5f, "Smooth RATE", " V/s", 4096.f, 0.5f);
		configParam(VC_SM_PARAM, 0.f, 1.f, 0.5f, "Smooth VC (VC IN amount, oct/V)");
		configParam(RATE_ST_PARAM, 0.f, 1.f, 0.5f, "Stepped RATE", " V/s", 4096.f, 0.5f);
		configParam(VC_ST_PARAM, 0.f, 1.f, 0.5f, "Stepped VC (VC IN amount, oct/V)");
		configInput(IN_SM_INPUT, "Smooth IN");
		configInput(VC_SM_INPUT, "Smooth VC IN");
		configInput(HOLD_INPUT, "Smooth HOLD (high freezes the output)");
		configInput(IN_ST_INPUT, "Stepped IN");
		configInput(VC_ST_INPUT, "Stepped VC IN");
		configInput(SAMPLE_INPUT, "Stepped SAMPLE");
		configOutput(CYCLE_SM_OUTPUT, "Smooth CYCLE");
		configOutput(OUT_SM_OUTPUT, "Smooth OUT");
		configOutput(CYCLE_ST_OUTPUT, "Stepped CYCLE");
		configOutput(OUT_ST_OUTPUT, "Stepped OUT");
		configOutput(COUPLER_OUTPUT, "COUPLER (high when stepped > smooth)");
		configOutput(HOT_OUTPUT, "HOT COUPLER");
		configLight(LEVEL_SM_LIGHT, "Smooth level");
		configLight(LEVEL_ST_LIGHT, "Stepped level");
	}

	void process(const ProcessArgs& args) override {
		ssg.process(args.sampleTime,
		            inputs[IN_SM_INPUT].getVoltage(), inputs[VC_SM_INPUT].getVoltage(), inputs[HOLD_INPUT].getVoltage(),
		            params[RATE_SM_PARAM].getValue(), params[VC_SM_PARAM].getValue(),
		            inputs[IN_ST_INPUT].getVoltage(), inputs[VC_ST_INPUT].getVoltage(), inputs[SAMPLE_INPUT].getVoltage(),
		            params[RATE_ST_PARAM].getValue(), params[VC_ST_PARAM].getValue());
		outputs[CYCLE_SM_OUTPUT].setVoltage(ssg.smooth.cycle);
		outputs[OUT_SM_OUTPUT].setVoltage(ssg.smooth.slew.y);
		outputs[CYCLE_ST_OUTPUT].setVoltage(ssg.stepped.cycle);
		outputs[OUT_ST_OUTPUT].setVoltage(ssg.stepped.slew.y);
		outputs[COUPLER_OUTPUT].setVoltage(ssg.coupler);
		outputs[HOT_OUTPUT].setVoltage(ssg.hot);
		lights[LEVEL_SM_LIGHT].setBrightnessSmooth(clamp(std::fabs(ssg.smooth.slew.y) / 5.f, 0.f, 1.f), args.sampleTime);
		lights[LEVEL_ST_LIGHT].setBrightnessSmooth(clamp(std::fabs(ssg.stepped.slew.y) / 5.f, 0.f, 1.f), args.sampleTime);
	}
};

struct SSGWidget : ModuleWidget {
	SSGWidget(SSG* m) { sergeScaffold(this, m, "SSG"); placeSSG(this, m); }
};

Model* modelSSG = createModel<SSG, SSGWidget>("SSG");
