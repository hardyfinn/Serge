#include "../plugin.hpp"
#include "../dsp/Dsg.hpp"
#include "../generated/Layout.hpp"

struct DSG : Module {
	enum ParamId { RISE_A_PARAM, FALL_A_PARAM, VC_A_PARAM, MODE_A_PARAM, RISE_B_PARAM, FALL_B_PARAM, VC_B_PARAM, MODE_B_PARAM, PARAMS_LEN };
	enum InputId { IN_A_INPUT, TRIG_A_INPUT, VC_A_INPUT, VOCT_A_INPUT, IN_B_INPUT, TRIG_B_INPUT, VC_B_INPUT, VOCT_B_INPUT, INPUTS_LEN };
	enum OutputId { OUT_A_OUTPUT, END_A_OUTPUT, OUT_B_OUTPUT, END_B_OUTPUT, OUTPUTS_LEN };
	enum LightId { LEVEL_A_LIGHT, LEVEL_B_LIGHT, LIGHTS_LEN };

	serge::DsgSection sec[2];

	DSG() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		const char* n[2] = {"A", "B"};
		for (int s = 0; s < 2; s++) {
			int p = s * 4, i = s * 4, o = s * 2;
			// time for a 5 V slope: 20 s * (2^-17.6)^k
			configParam(RISE_A_PARAM + p, 0.f, 1.f, 0.5f, string::f("Rise %s (time for 5 V)", n[s]), " s", std::exp2(-17.6f), 20.f);
			configParam(FALL_A_PARAM + p, 0.f, 1.f, 0.5f, string::f("Fall %s (time for 5 V)", n[s]), " s", std::exp2(-17.6f), 20.f);
			configParam(VC_A_PARAM + p, -1.f, 1.f, 0.f, string::f("VC %s (VC IN amount, oct/V)", n[s]));
			configSwitch(MODE_A_PARAM + p, 0.f, 2.f, 1.f, string::f("VC IN controls %s", n[s]), {"Rise", "Both", "Fall"});
			configInput(IN_A_INPUT + i, string::f("IN %s (signal to follow)", n[s]));
			configInput(TRIG_A_INPUT + i, string::f("TRIG %s", n[s]));
			configInput(VC_A_INPUT + i, string::f("VC IN %s", n[s]));
			configInput(VOCT_A_INPUT + i, string::f("1V/OCT %s", n[s]));
			configOutput(OUT_A_OUTPUT + o, string::f("OUT %s", n[s]));
			configOutput(END_A_OUTPUT + o, string::f("END %s (low during the fall)", n[s]));
			configLight(LEVEL_A_LIGHT + s, string::f("Level %s", n[s]));
		}
	}

	void process(const ProcessArgs& args) override {
		for (int s = 0; s < 2; s++) {
			int p = s * 4, i = s * 4, o = s * 2;
			sec[s].process(args.sampleTime,
			               inputs[IN_A_INPUT + i].getVoltage(), inputs[TRIG_A_INPUT + i].getVoltage(),
			               inputs[VC_A_INPUT + i].getVoltage(), inputs[VOCT_A_INPUT + i].getVoltage(),
			               params[RISE_A_PARAM + p].getValue(), params[FALL_A_PARAM + p].getValue(),
			               params[VC_A_PARAM + p].getValue(), (int) std::round(params[MODE_A_PARAM + p].getValue()));
			outputs[OUT_A_OUTPUT + o].setVoltage(sec[s].out);
			outputs[END_A_OUTPUT + o].setVoltage(sec[s].end);
			lights[LEVEL_A_LIGHT + s].setBrightnessSmooth(clamp(sec[s].out / 5.f, 0.f, 1.f), args.sampleTime);
		}
	}
};

struct DSGWidget : ModuleWidget {
	DSGWidget(DSG* m) { sergeScaffold(this, m, "DSG"); placeDSG(this, m); }
};

Model* modelDSG = createModel<DSG, DSGWidget>("DSG");
