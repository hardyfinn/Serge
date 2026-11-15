#include "../plugin.hpp"
#include "../dsp/Switching.hpp"
#include "../generated/Layout.hpp"

// BDR, Bi-Directional Router ---------------------------------------------------
struct BDR : Module {
	enum ParamId { PARAMS_LEN };
	enum InputId { SELECT1_INPUT, BIN1_INPUT, A1IN1_INPUT, A2IN1_INPUT,
	               SELECT2_INPUT, BIN2_INPUT, A1IN2_INPUT, A2IN2_INPUT,
	               SELECT3_INPUT, BIN3_INPUT, A1IN3_INPUT, A2IN3_INPUT, INPUTS_LEN };
	enum OutputId { A1OUT1_OUTPUT, A2OUT1_OUTPUT, BOUT1_OUTPUT,
	                A1OUT2_OUTPUT, A2OUT2_OUTPUT, BOUT2_OUTPUT,
	                A1OUT3_OUTPUT, A2OUT3_OUTPUT, BOUT3_OUTPUT, OUTPUTS_LEN };
	enum LightId { LIGHTS_LEN };

	serge::BdrSection sec[3];

	BDR() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		for (int s = 0; s < 3; s++) {
			int n = s + 1;
			configInput(SELECT1_INPUT + 4 * s, string::f("%d: SELECT (low A1, high A2)", n));
			configInput(BIN1_INPUT + 4 * s, string::f("%d: B IN (1 to 2)", n));
			configInput(A1IN1_INPUT + 4 * s, string::f("%d: A1 IN (2 to 1)", n));
			configInput(A2IN1_INPUT + 4 * s, string::f("%d: A2 IN (2 to 1)", n));
			configOutput(A1OUT1_OUTPUT + 3 * s, string::f("%d: A1 OUT (1 to 2)", n));
			configOutput(A2OUT1_OUTPUT + 3 * s, string::f("%d: A2 OUT (1 to 2)", n));
			configOutput(BOUT1_OUTPUT + 3 * s, string::f("%d: B OUT (2 to 1)", n));
		}
	}

	void onReset() override { for (auto& s : sec) s.reset(); }

	void process(const ProcessArgs&) override {
		for (int s = 0; s < 3; s++) {
			sec[s].process(inputs[SELECT1_INPUT + 4 * s].getVoltage(), inputs[BIN1_INPUT + 4 * s].getVoltage(),
			               inputs[A1IN1_INPUT + 4 * s].getVoltage(), inputs[A2IN1_INPUT + 4 * s].getVoltage());
			outputs[A1OUT1_OUTPUT + 3 * s].setVoltage(sec[s].a1Out);
			outputs[A2OUT1_OUTPUT + 3 * s].setVoltage(sec[s].a2Out);
			outputs[BOUT1_OUTPUT + 3 * s].setVoltage(sec[s].bOut);
		}
	}
};

struct BDRWidget : ModuleWidget {
	BDRWidget(BDR* m) { sergeScaffold(this, m, "BDR"); placeBDR(this, m); }
};

Model* modelBDR = createModel<BDR, BDRWidget>("BDR");

// PDIV, Pulse Divider ----------------------------------------------------------
struct PDIV : Module {
	enum ParamId { PARAMS_LEN };
	enum InputId { IN_INPUT, INPUTS_LEN };
	enum OutputId { DIV2_OUTPUT, DIV3_OUTPUT, DIV4_OUTPUT, DIV5_OUTPUT, DIV6_OUTPUT, DIV7_OUTPUT, DIV8_OUTPUT, OUTPUTS_LEN };
	enum LightId { LIGHTS_LEN };

	serge::PulseDivider div;

	PDIV() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configInput(IN_INPUT, "IN (counts each rise through +2.5 V)");
		for (int n = 2; n <= 8; n++) configOutput(DIV2_OUTPUT + n - 2, string::f("Divide by %d", n));
	}

	void onReset() override { div.reset(); }

	void process(const ProcessArgs&) override {
		div.process(inputs[IN_INPUT].getVoltage());
		for (int n = 2; n <= 8; n++) outputs[DIV2_OUTPUT + n - 2].setVoltage(div.out[n]);
	}
};

struct PDIVWidget : ModuleWidget {
	PDIVWidget(PDIV* m) { sergeScaffold(this, m, "PDIV"); placePDIV(this, m); }
};

Model* modelPDIV = createModel<PDIV, PDIVWidget>("PDIV");
